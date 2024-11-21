// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 */

#include <stdbool.h>
#include <stddef.h>

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

#include "ms_tpm_backend.h"
#include "trace.h"

static inline uint32_t tpm_get_response_code(uint8_t *buf)
{
	if (!buf)
		return 0x101; // TPM_RC_FAILURE

	return (uint32_t)((buf[6] << 24) + (buf[7] << 16) + (buf[8] << 8 ) + buf[9]);
}

/*
* Hardcoded buffer of the startup command with "clear" argument
*
* tag: 0x8001 (TPM_ST_NO_SESSIONS),
* commandSize: 0x0c,
* commandCode: 0x0144 (TPM_CC_Startup),
* startupType: 0x0 (TPM_SU_CLEAR)
*/
static uint8_t tpm_startup_clear_cmd[] = { 0x80, 0x01,
					   0x00, 0x00, 0x00, 0x0c,
					   0x00, 0x00, 0x01, 0x44,
					   0x00, 0x00 };


static uint32_t tpm_startup(void)
{
	uint8_t out_buf[128] = { 0 };
	uint8_t *out_ptr = out_buf;
	uint32_t out_size = sizeof(out_buf);

	ExecuteCommand(sizeof(tpm_startup_clear_cmd), tpm_startup_clear_cmd, &out_size, &out_ptr);

	return tpm_get_response_code(out_ptr);
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
