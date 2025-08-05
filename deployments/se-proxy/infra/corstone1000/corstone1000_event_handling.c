/*
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <psa/client.h>
#include <psa/sid.h>
#include <trace.h>

#include <protocols/rpc/common/packed-c/status.h>
#include "corstone1000_event_handling.h"

#define BOOT_CONFIRMED_EVENT   (0x3)
#define OPCODE_BOOT_CONFIRMED  BOOT_CONFIRMED_EVENT

enum corstone1000_ioctl_id {
	IOCTL_CORSTONE1000_FWU_HOST_ACK = 1,
};

/* Service request handlers */
static rpc_status_t boot_confirmed_handler(void *context, struct rpc_request *req);

/* Handler mapping table for service */
static const struct service_handler handler_table[] = {
	{.opcode = OPCODE_BOOT_CONFIRMED, .invoke = boot_confirmed_handler}
};

struct rpc_service_interface *corstone1000_event_provider_init(struct event_provider *context)
{
	if (!context)
		return NULL;

	service_provider_init(&context->base_provider, context,
			      &(const struct rpc_uuid){.uuid = { 0 }}, handler_table,
		sizeof(handler_table) / sizeof(struct service_handler));

	return service_provider_get_rpc_interface(&context->base_provider);
}

static rpc_status_t event_handler(uint32_t opcode, struct rpc_caller_interface *caller)
{
	if (!caller) {
		EMSG("%s rpc_caller is NULL", __func__);
		return TS_RPC_ERROR_RESOURCE_FAILURE;
	}

	IMSG("%s opcode %x", __func__, opcode);

	uint32_t ioctl_id;
	uint32_t mem_buf_start_addr;

	if (psa_ptr_to_u32_checked(&ioctl_id, &mem_buf_start_addr) != PSA_SUCCESS) {
		EMSG("%s start address of the memory buffer does not fit in 32 bits", __func__);
		return TS_RPC_ERROR_INTERNAL;
	}

	struct psa_invec in_vec[] = {
		{ .base = mem_buf_start_addr, .len = sizeof(ioctl_id) }
	};

	switch (opcode) {
	case OPCODE_BOOT_CONFIRMED:
		ioctl_id = IOCTL_CORSTONE1000_FWU_HOST_ACK;

		psa_call(caller, TFM_PLATFORM_SERVICE_HANDLE, TFM_PLATFORM_API_ID_IOCTL, in_vec,
			 IOVEC_LEN(in_vec), NULL, 0);
		break;

	default:
		EMSG("%s unsupported opcode", __func__);
		return TS_RPC_ERROR_INVALID_PARAMETER;
	}

	return TS_RPC_CALL_ACCEPTED;
}

static rpc_status_t boot_confirmed_handler(void *context, struct rpc_request *req)
{
	struct event_provider *this_instance = (struct event_provider *)context;

	return event_handler(req->opcode, this_instance->client.session->caller);
}
