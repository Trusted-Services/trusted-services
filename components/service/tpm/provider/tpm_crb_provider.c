// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 */

#include <stdint.h>
#include <string.h>
#include "rpc/tpm_crb_ffa/common/tpm_crb_ffa.h"
#include "tpm_crb_provider.h"
#include "trace.h"
#include "util.h"

#define CRB_DATA_BUF_SIZE (size_t)0x800

/* CRB register definitions based on TCG PC Client Platform TPM Profile Specification for TPM 2.0 */

/* TPM_LOC_STATE_x: Locality State Register */
#define LOC_STATE_TPM_ESTABLISHED		BIT32(0)
#define LOC_STATE_LOC_ASSIGNED			BIT32(1)
#define LOC_STATE_ACTIVE_LOCALITY_SHIFT		UINT32_C(2)
#define LOC_STATE_ACTIVE_LOCALITY_MASK		GENMASK_32(2, 0)
#define LOC_STATE_TPM_REG_VALID_STATUS		BIT32(7)

/* TPM_LOC_CTRL_x: Locality Control Register for Localities 0 - 3 */
#define LOC_CTRL_REQUEST_ACCESS			BIT32(0)
#define LOC_CTRL_RELINQUISH			BIT32(1)
#define LOC_CTRL_SEIZE				BIT32(2)
#define LOC_CTRL_RESET_ESTABLISHMENT_BIT	BIT32(3)

/* TPM_LOC_CTRL_4: Locality Control Register for Locality 4 */
#define LOC_CTRL_4_HASH_START			BIT32(0)
#define LOC_CTRL_4_HASH_DATA			BIT32(1)
#define LOC_CTRL_4_HASH_END			BIT32(2)
#define LOC_CTRL_4_RESET_ESTABLISHMENT_BIT	BIT32(3)

/* TPM_LOC_STS_x: Locality Status Register */
#define LOC_STS_GRANTED				BIT32(0)
#define LOC_STS_BEEN_SEIZED			BIT32(1)

/* TPM_CRB_CTRL_REQ_x: Control Area Request Register */
#define CRB_CTRL_REQ_CMD_READY			BIT32(0)
#define CRB_CTRL_REQ_GO_IDLE			BIT32(1)

/* TPM_CRB_CTRL_STS_x: Control Area Status Register */
#define CRB_CTRL_STS_TPM_STATUS			BIT32(0)
#define CRB_CTRL_STS_TPM_IDLE			BIT32(1)

/* TPM_CRB_CTRL_CANCEL_x: Control Area Status Register */
#define CRB_CTRL_CANCEL_COMMAND			BIT32(0)

/* TPM_CRB_CTRL_START_x: Control Area Status Register */
#define CRB_CTRL_START_COMMAND			BIT32(0)

struct loc_and_crb_ctrl {
	// Offset 0x00 - 0x03: Used to determine current state of locality of the TPM. This register
	// is aliased across all localities. Read-only.
	uint32_t loc_state;
	// Offset 0x04 - 0x07: Reserved
	uint8_t _res4[4];
	// Offset 0x08 - 0x0b: Used to gain control of the TPM by this locality.
	uint32_t loc_ctrl;
	// Offset 0x0c - 0x0f: Used to determine whether locality has been granted or Seized.
	// Read-only. This register SHALL NOT be aliased.
	uint32_t loc_status;
	// Offset 0x10 - 0x2f: Reserved
	uint8_t _res10[32];
	// Offset 0x30 - 0x37: Used to identify the Interface types supported by the TPM as well as
	// the Vendor ID, Device ID and Revision ID.
	uint64_t interface_id;
	// Offset 0x38 - 0x3f: Optional Register used in low memory environments prior to
	// CRB_DATA_BUFFER availability. This field is not implemented in hardware TPMs. This field
	// is only available in Locality 0.
	uint64_t ctrl_ext;
	// Offset 0x40 - 0x43: Register used to initiate transactions for the CRB interface. This
	// register may be aliased across localities.
	uint32_t ctrl_request;
	// Offset 0x44 - 0x47: Register used by the TPM to provide status of the CRB interface. This
	// register may be aliased across localities.
	uint32_t ctrl_status;
	// Offset 0x48 - 0x4b: Register used by Software to cancel command processing. This register
	// may be aliased across localities.
	uint32_t ctrl_cancel;
	// Offset 0x4c - 0x4f: Register used to indicate presence of command or response data in the
	// CRB buffer. This register may be aliased across localities.
	uint32_t ctrl_start;
	// Offset 0x50 - 0x53: Register used to configure interrupts. This register may be aliased
	// across localities.
	uint32_t ctrl_int_enable;
	// Offset 0x54 - 0x57: Register used to respond to interrupts. This register may be aliased
	// across localities.
	uint32_t ctrl_int_status;
	// Offset 0x58 - 0x5b: Size of the Command buffer. This register may be aliased across
	// localities.
	uint32_t ctrl_cmd_size;
	// Offset 0x5c - 0x5f: Lower 32bits of the Command buffer start address for the locality.
	// This register may be aliased across localities.
	uint32_t ctrl_cmd_addr_lo;
	// Offset 0x60 - 0x63: Upper 32bits of the Command buffer start address for the locality.
	// This register may be aliased across localities.
	uint32_t ctrl_cmd_addr_hi;
	// Offset 0x64 - 0x67: Size of the Response buffer. Note: If command and response buffers
	// are implemented as a single buffer, this field SHALL be identical to the value in the
	// TPM_CRB_CTRL_CMD_SIZE_x buffer. This register may be aliased across localities.
	uint32_t ctrl_resp_size;
	// Offset 0x68 - 0x6f: Address of the start of the Response buffer. Note: If command and
	// response buffers are implemented as a single buffer, this field SHALL contain the same
	// address contained in TPM_CRB_CTRL_CMD_HADDR_x and TPM_CRB_CMD_LADDR_x. This register may
	// be aliased across localities.
	uint64_t ctrl_resp_addr;
	// Offset 0x70 - 0x7f: Reserved
	uint8_t _res70[16];
	// Offset 0x80 - 0x880: Command/Response Data may be defined as large as 3968. This is
	// implementation-specific. However, the full address space has been reserved. This buffer
	// may be aliased across localities. This field accepts data transfers from 1B up to the
	// size indicated by TPM_CRB_INTF_ID_x.CapDataXferSizeSupport (see section 6.4.2.2 CRB
	// Interface Identifier Register).
	uint8_t data_buffer[CRB_DATA_BUF_SIZE];
} __packed __aligned(__alignof(uint32_t));

void set_loc_state_all(struct loc_and_crb_ctrl *loc_ptr[5], uint32_t loc_state)
{
	/* The TPM_LOC_STATE_x register is aliased across all localities, set in one step */
	for (unsigned i = 0; i < 5; i++)
		loc_ptr[i]->loc_state = loc_state;
}

/* Service request handlers */
static rpc_status_t command_handler(void *context, struct rpc_request *req);
static rpc_status_t locality_req_handler(void *context, struct rpc_request *req);

/* Handler mapping table for service */
static const struct service_handler handler_table[] = {
	{ TPM_START_QUALIFIER_COMMAND, command_handler },
	{ TPM_START_QUALIFIER_LOCALITY_REQ, locality_req_handler },
};

static inline uint32_t tpm_get_request_length(uint8_t *buf)
{
	if (!buf)
		return 0;

	return (uint32_t)((buf[2] << 24) + (buf[3] << 16) + (buf[4] << 8 ) + buf[5]);
}

static rpc_status_t command_handler(void *context, struct rpc_request *req)
{
	struct tpm_crb_provider *this_instance = (struct tpm_crb_provider *)context;
	struct loc_and_crb_ctrl *req_loc = NULL;
	uint8_t locality = req->client_id;
	uint8_t *resp_data = NULL;
	uint8_t *req_data = NULL;
	size_t resp_max_size = 0;
	size_t resp_len = 0;
	size_t req_len = 0;
	size_t crb_size = 4096; /* TODO: this config should come from the build system */

	DMSG("Processing TPM service command at locality %d", locality);

	/* The locality which made the request */
	req_loc = this_instance->loc_ptr[locality];

	if (!(req_loc->ctrl_start & CRB_CTRL_START_COMMAND)) {
		req->service_status = TPM_ERROR_INV_CRB_CTRL_DATA;
		return RPC_ERROR_INTERNAL;
	}

	req_len = tpm_get_request_length(req_loc->data_buffer);
	if (req_len == 0 || crb_size < req_len) {
		req->service_status = TPM_ERROR_INV_CRB_CTRL_DATA;
		return RPC_ERROR_INTERNAL;
	}

	req_data = req_loc->data_buffer;
	resp_data = req_loc->data_buffer;
	resp_max_size = crb_size;

	ms_tpm_backend_execute_command(req_data, req_len, &resp_data, &resp_len, resp_max_size);

	/* All operations done, clear the pending request */
	req_loc->ctrl_start &= ~CRB_CTRL_START_COMMAND;
	req->service_status = TPM_STATUS_OK;

	return RPC_SUCCESS;
}

static rpc_status_t locality_req_handler(void *context, struct rpc_request *req)
{
	struct tpm_crb_provider *this_instance = (struct tpm_crb_provider *)context;
	struct loc_and_crb_ctrl *req_loc = NULL;
	uint8_t locality = req->client_id;

	if (locality == 4) {
		EMSG("Locality 4 handling is currently not supported");
		req->service_status = TPM_ERROR_NOTSUP;
		return RPC_ERROR_INTERNAL;
	}

	DMSG("Processing TPM service locality request at locality %d", locality);

	/* The locality which made the request */
	req_loc = this_instance->loc_ptr[locality];

	if (req_loc->loc_ctrl & LOC_CTRL_REQUEST_ACCESS) {
		uint32_t loc_state = req_loc->loc_state;

		DMSG("Locality access request");

		/* TODO: execute locality arbitration algorithm */

		/* Clear the valid status bit before manipulating register contents */
		loc_state &= ~LOC_STATE_TPM_REG_VALID_STATUS;
		set_loc_state_all(this_instance->loc_ptr, loc_state);

		/* Clear current active locality and set the new */
		loc_state &= ~SHIFT_U32(LOC_STATE_ACTIVE_LOCALITY_MASK,
					LOC_STATE_ACTIVE_LOCALITY_SHIFT);
		loc_state |= SHIFT_U32(locality & LOC_STATE_ACTIVE_LOCALITY_MASK,
				       LOC_STATE_ACTIVE_LOCALITY_SHIFT);
		loc_state |= LOC_STATE_LOC_ASSIGNED;
		set_loc_state_all(this_instance->loc_ptr, loc_state);

		/* Set the valid status bit */
		loc_state |= LOC_STATE_TPM_REG_VALID_STATUS;
		set_loc_state_all(this_instance->loc_ptr, loc_state);

		/* Set the locality status to granted */
		req_loc->loc_status |= LOC_STS_GRANTED;

		/* All operations done, clear the pending request */
		req_loc->loc_ctrl &= ~LOC_CTRL_REQUEST_ACCESS;
	}

	if (req_loc->loc_ctrl & LOC_CTRL_RELINQUISH) {
		uint32_t loc_state = req_loc->loc_state;

		DMSG("Locality relinquish");

		/* Clear the valid status bit before manipulating register contents */
		loc_state &= ~LOC_STATE_TPM_REG_VALID_STATUS;
		set_loc_state_all(this_instance->loc_ptr, loc_state);

		/* Clear current active locality and the locality assigned bit */
		loc_state &= ~SHIFT_U32(LOC_STATE_ACTIVE_LOCALITY_MASK,
					LOC_STATE_ACTIVE_LOCALITY_SHIFT);
		loc_state &= ~LOC_STATE_LOC_ASSIGNED;
		set_loc_state_all(this_instance->loc_ptr, loc_state);

		/* Set the valid status bit */
		loc_state |= LOC_STATE_TPM_REG_VALID_STATUS;
		set_loc_state_all(this_instance->loc_ptr, loc_state);

		/* Clear the locality granted bit */
		req_loc->loc_status &= ~LOC_STS_GRANTED;

		/* All operations done, clear the pending request */
		req_loc->loc_ctrl &= ~LOC_CTRL_RELINQUISH;
	}

	req->service_status = TPM_STATUS_OK;

	return RPC_SUCCESS;
}

struct rpc_service_interface *tpm_provider_init(struct tpm_crb_provider *context,
						uint8_t *ns_crb, size_t ns_crb_size,
						uint8_t* s_crb, size_t s_crb_size)
{
	const struct rpc_uuid tpm_crb_service_uuid = {
		.uuid = TPM_CRB_FFA_UUID
	};

	if (!context || !ns_crb || ns_crb_size == 0 || !s_crb || s_crb_size == 0)
		return NULL;

	/* All of the locality and CRB control registers should be cleared an power on */
	memset(ns_crb, 0, ns_crb_size);
	memset(s_crb, 0, s_crb_size);

	/* Note: these are the CRB VAs that we got from the SP boot info */
	context->loc_ptr[0] = (struct loc_and_crb_ctrl *)ns_crb;
	context->loc_ptr[1] = (struct loc_and_crb_ctrl *)(ns_crb + 0x1000);
	context->loc_ptr[2] = (struct loc_and_crb_ctrl *)(ns_crb + 0x2000);
	context->loc_ptr[3] = (struct loc_and_crb_ctrl *)(ns_crb + 0x3000);
	context->loc_ptr[4] = (struct loc_and_crb_ctrl *)s_crb;

	for (unsigned i = 0; i <= 4; i++) {
		uint64_t data_buf_addr = 0;

		/* Note: here we have to use the CRB PAs that's fixed at compile time */
		if (i < 4) {
			data_buf_addr = TPM_CRB_NS_PA + i * 0x1000 +
					offsetof(struct loc_and_crb_ctrl, data_buffer);
		} else {
			data_buf_addr = TPM_CRB_S_PA +
					offsetof(struct loc_and_crb_ctrl, data_buffer);
		}

		context->loc_ptr[i]->ctrl_cmd_addr_lo = data_buf_addr & UINT32_MAX;
		context->loc_ptr[i]->ctrl_cmd_addr_hi = data_buf_addr >> 32;
		context->loc_ptr[i]->ctrl_cmd_size = CRB_DATA_BUF_SIZE;
		context->loc_ptr[i]->ctrl_resp_addr = data_buf_addr;
		context->loc_ptr[i]->ctrl_resp_size = CRB_DATA_BUF_SIZE;
	}

	/* Set the valid bit in Locality State Register for all localities */
	set_loc_state_all(context->loc_ptr, LOC_STATE_TPM_REG_VALID_STATUS);

	service_provider_init(&context->base_provider, context, &tpm_crb_service_uuid,
			      handler_table, ARRAY_SIZE(handler_table));

	return service_provider_get_rpc_interface(&context->base_provider);
}
