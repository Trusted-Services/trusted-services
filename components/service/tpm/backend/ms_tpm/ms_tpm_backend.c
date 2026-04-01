// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 */

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "TpmBuildSwitches.h"
#include "BaseTypes.h"
#include "Platform_fp.h"
#include "ExecCommand_fp.h"
#include "Manufacture_fp.h"
#include "_TPM_Init_fp.h"
#include "_TPM_Hash_Start_fp.h"
#include "_TPM_Hash_Data_fp.h"
#include "_TPM_Hash_End_fp.h"
#include "TpmFail_fp.h"

#include <TpmProfile.h>
#include <GpMacros.h>
#include <TPMB.h>
#include <TpmAlgorithmDefines.h>
#include <Capabilities.h>
#include <TpmTypes.h>

#include "ms_tpm_backend.h"
#include "trace.h"

#include "components/common/event_log/event_log_parser.h"
#include "common/endian/be.h"
#include "common/utils/include/compiler.h"

// Size of the TPMS_AUTH_COMMAND structure is dynamic. If TPM2B size is 0, the
// length of the structure becomes sizeof(UINT16). Hence the minimum size is 9
// bytes.
#define AUTH_SIZE               9u

#define GET_BYTE(ndx, val) \
	(((val) >> ((ndx) * 8u)) & 0xffu)

// tcg.h can not be included as it has items conflicting with tcg-tpm headers.
// Duplicate the macro below here as a workaround.
#define EV_POST_CODE	0x00000001

/*
 * Hardcoded buffer of the startup command with "clear" argument
 *
 * tag: 0x8001 (TPM_ST_NO_SESSIONS),
 * commandSize: 0x0c,
 * commandCode: 0x0144 (TPM_CC_Startup),
 * startupType: 0x0 (TPM_SU_CLEAR)
 */
static const uint8_t tpm_startup_clear_cmd[] = { 0x80, 0x01,
					   0x00, 0x00, 0x00, 0x0c,
					   0x00, 0x00, 0x01, 0x44,
					   0x00, 0x00 };

/*
 * Hardcoded empty password auth command.
 */
static const uint8_t tpm_empty_pwd_aut_cmd[AUTH_SIZE] = {
	GET_BYTE(3, TPM_RS_PW), GET_BYTE(2, TPM_RS_PW),
	GET_BYTE(1, TPM_RS_PW), GET_BYTE(0, TPM_RS_PW), //session handle
	0, 0,	// nonce.t.size
	0,	// sessionAttributes
	0, 0	// hmac.t.size
};

/*
 * The structure below uses complex data types from TpmTypes.h which are not
 * packed. As a result, members before AuthSessionPcr can be referred with C
 * code directly. The address of further members must be calculated manually.
 */
#pragma pack(push, 1)
typedef struct {
	TPM_ST                tag;
	uint32_t              commandSize;
	TPM_CC                commandCode;
	TPMI_DH_PCR           PcrHandle;
	uint32_t              AuthorizationSize;
	TPMS_AUTH_COMMAND     AuthSessionPcr;
	TPML_DIGEST_VALUES    DigestValues;
} TPM2_PCR_EXTEND_COMMAND;
#pragma pack(pop)

/*
 * Read the response code returned by ExecuteCommand.
 */
static inline uint32_t tpm_get_response_code(uint8_t *buf)
{
	if (!buf)
		return 0x101; // TPM_RC_FAILURE

	return load_u32_be(buf, 6);
}

/*
 * Invoke startup command
 */
static uint32_t tpm_startup(void)
{
	uint8_t out_buf[128] = { 0 };
	uint8_t *out_ptr = out_buf;
	uint32_t out_size = sizeof(out_buf);

	ExecuteCommand(sizeof(tpm_startup_clear_cmd), (unsigned char *)tpm_startup_clear_cmd,
		       &out_size, &out_ptr);

	return tpm_get_response_code(out_ptr);
}

/*
 * Invoke a PCR_EXTEND command.
 */
static int ms_tpm_backend_extend_pcr(struct evl_event2_record *record)
{
	uint8_t out_buf[128] = { 0 };
	uint8_t *out_ptr = out_buf;
	uint32_t out_size = sizeof(out_buf);
	TPM2_PCR_EXTEND_COMMAND cmd = {0};
	uint8_t *p = (uint8_t *)&cmd;

	// The current code assumes all SHA digest types in the log are supported, and all PCR banks
	// are enabled. The code could use TPM2_GetCapability to check the available banks and then
	// use TPM2_PCR_Allocate to enable the required banks.

	store_u16_be(p, offsetof(TPM2_PCR_EXTEND_COMMAND, tag), TPM_ST_SESSIONS);
	store_u32_be(p, offsetof(TPM2_PCR_EXTEND_COMMAND, commandCode), TPM_CC_PCR_Extend);
	store_u32_be(p, offsetof(TPM2_PCR_EXTEND_COMMAND, PcrHandle), record->header.pcr_index);
	store_u32_be(p, offsetof(TPM2_PCR_EXTEND_COMMAND, AuthorizationSize), AUTH_SIZE);

	// Note: cmd.AuthSessionPcr is not packed. Use memcpy from an uint8_t array as a workaround.
	memcpy(&cmd.AuthSessionPcr, tpm_empty_pwd_aut_cmd, AUTH_SIZE);
	p += offsetof(TPM2_PCR_EXTEND_COMMAND, AuthSessionPcr) + AUTH_SIZE;

	store_u32_be(p, 0, record->header.n_digests);
	p += 4;

	uint8_t *limit = ((uint8_t *)&cmd) + sizeof(cmd);

	for (size_t x = 0; x < record->header.n_digests; x++) {
		size_t digest_size = tcg_event2_digest_size(record->header.digests[x].algorithm_id);

		if (digest_size == 0) {
			EMSG("Unsupported algorithm type %u",
			     record->header.digests[x].algorithm_id);
			return TPM_RC_FAILURE;
		}

		if (limit - p < 2 + digest_size) {
			EMSG("TPM command buffer overrun.");
			return TPM_RC_FAILURE;
		}

		store_u16_be(p, 0, record->header.digests[x].algorithm_id);
		p += 2;
		memcpy(p, record->header.digests[x].digest, digest_size);
		p += digest_size;
	}

	uint32_t req_size = (uint32_t)((uintptr_t)p - (uintptr_t)&cmd);

	store_u32_be((uint8_t *)&cmd, offsetof(TPM2_PCR_EXTEND_COMMAND, commandSize), req_size);
	ExecuteCommand(req_size, (unsigned char *)&cmd, &out_size, &out_ptr);
	return tpm_get_response_code(out_ptr);
}

bool ms_tpm_backend_replay_eventlog(const uint8_t *log_buffer, const size_t log_buffer_length)
{
	struct evl_context context = {0};
	struct evl_record_data record = {0};

	if (event_log_parser_init(&context, log_buffer, log_buffer_length)) {
		EMSG("Invalid event log.");
		return true;
	}

	while (!event_log_parser_is_done(&context)) {
		if (event_log_parser_get(&context, &record)) {
			EMSG("Invalid event log.");
			return 1;
		}

		if (record.data_type == rct_pcr_record &&
		    record.data.pcr_record.header.event_type == EV_POST_CODE) {
			if (ms_tpm_backend_extend_pcr(&record.data.pcr_record) != 0 ) {
					EMSG("Failed to replay event log to tpm PCR registers!");
					return 1;
			}
		}

		(void)event_log_parser_next(&context);
	}

	return false;
}

void ms_tpm_backend_execute_command(const uint8_t *req, size_t req_len, uint8_t **resp,
				    size_t *resp_len, size_t resp_max_size)
{
	/* Set response buffer to same as request, discard const */
	uint8_t *response_buf = (uint8_t *)req;

	/* ms_tpm expects the maximum response size as input in this variable */
	uint32_t response_len = resp_max_size;

	ExecuteCommand(req_len, (uint8_t *)req, &response_len, &response_buf);

	*resp = response_buf;
	*resp_len = response_len;
}

bool ms_tpm_backend_init(void)
{
	uint32_t tpm_result = 0;
	int rc = 0;

	_plat__SetNvAvail();
	rc = _plat__NVEnable(NULL);
	if (rc) {
		EMSG("NV enable error: %d", rc);
		return false;
	}

	/* The parameter indicates if it's the first time we call this function */
	rc = TPM_Manufacture(true);
	if (rc) {
		EMSG("TPM manufacture error: %d", rc);
		return false;
	}

	rc = _plat__Signal_PowerOn();
	if (rc) {
		EMSG("Power on signal error: %d", rc);
		return false;
	}

	_TPM_Init();
	IMSG("TPM init done");

	tpm_result = tpm_startup();
	if (tpm_result != 0) {
		EMSG("TPM startup failed with error: 0x%x", tpm_result);
		return false;
	}

	IMSG("TPM startup done");

	return true;
}
