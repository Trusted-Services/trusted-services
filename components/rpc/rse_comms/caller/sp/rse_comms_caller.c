/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rse_comms_caller.h"

#include <stddef.h>
#include <stdlib.h>

#include "protocols/rpc/common/packed-c/status.h"
#include "rpc_caller.h"
#include "rpc_status.h"

struct rse_comms_caller_context {
	struct rse_comms_messenger rse_comms;
};

rpc_status_t open_session(void *context, const struct rpc_uuid *service_uuid, uint16_t endpoint_id)
{
	return RPC_SUCCESS;
}

rpc_status_t find_and_open_session(void *context, const struct rpc_uuid *service_uuid)
{
	return RPC_SUCCESS;
}

rpc_status_t close_session(void *context)
{
	return RPC_SUCCESS;
}

rpc_status_t create_shared_memory(void *context, size_t size,
				  struct rpc_caller_shared_memory *shared_memory)
{
	return RPC_ERROR_INVALID_VALUE;
}

rpc_status_t release_shared_memory(void *context, struct rpc_caller_shared_memory *shared_memory)
{
	return RPC_ERROR_INVALID_VALUE;
}

rpc_status_t call(void *context, uint16_t opcode, struct rpc_caller_shared_memory *shared_memory,
		  size_t request_length, size_t *response_length, service_status_t *service_status)
{
	return RPC_ERROR_INTERNAL;
}

rpc_status_t rse_comms_caller_invoke(rse_comms_call_handle handle, uint32_t opcode,
				     uint8_t **response_buffer, size_t *response_length)
{
	struct rpc_caller_interface *caller = (struct rpc_caller_interface *)handle;
	struct rse_comms_caller_context *context = NULL;
	int ret = 0;

	if (!handle || !caller->context)
		return RPC_ERROR_INVALID_VALUE;

	context = (struct rse_comms_caller_context *)caller->context;

	ret = rse_comms_messenger_call_invoke(&context->rse_comms, response_buffer,
					      response_length);
	if (ret < 0)
		return RPC_ERROR_TRANSPORT_LAYER;

	return RPC_SUCCESS;
}

rpc_status_t rse_comms_caller_init(struct rpc_caller_interface *rpc_caller)
{
	struct rse_comms_caller_context *context = NULL;
	int ret = 0;

	if (!rpc_caller || rpc_caller->context)
		return RPC_ERROR_INVALID_VALUE;

	context = (struct rse_comms_caller_context *)calloc(1,
				sizeof(struct rse_comms_caller_context));
	if (!context)
		return RPC_ERROR_INTERNAL;

	ret = rse_comms_messenger_init(&context->rse_comms);

	if (ret < 0) {
		free(context);
		return RPC_ERROR_TRANSPORT_LAYER;
	}

	rpc_caller->context = context;
	rpc_caller->open_session = open_session;
	rpc_caller->find_and_open_session = find_and_open_session;
	rpc_caller->close_session = close_session;
	rpc_caller->create_shared_memory = create_shared_memory;
	rpc_caller->release_shared_memory = release_shared_memory;
	rpc_caller->call = call;

	return RPC_SUCCESS;
}

rse_comms_call_handle rse_comms_caller_begin(struct rpc_caller_interface *caller,
					     uint8_t **request_buffer, size_t request_length)
{
	struct rse_comms_caller_context *context = NULL;
	int ret = 0;

	if (!caller || !caller->context)
		return NULL;

	context = (struct rse_comms_caller_context *)caller->context;

	ret = rse_comms_messenger_call_begin(&context->rse_comms, request_buffer, request_length);

	if (ret < 0)
		return NULL;

	return caller;
}

rpc_status_t rse_comms_caller_end(rse_comms_call_handle handle)
{
	struct rpc_caller_interface *caller = (struct rpc_caller_interface *)handle;
	struct rse_comms_caller_context *context = NULL;

	if (!handle || !caller->context)
		return RPC_ERROR_INVALID_VALUE;

	context = (struct rse_comms_caller_context *)caller->context;

	rse_comms_messenger_call_end(&context->rse_comms);

	return RPC_SUCCESS;
}

rpc_status_t rse_comms_caller_deinit(struct rpc_caller_interface *rpc_caller)
{
	struct rse_comms_caller_context *context = NULL;

	if (!rpc_caller)
		return RPC_ERROR_INVALID_VALUE;

	context = (struct rse_comms_caller_context *)rpc_caller->context;

	rse_comms_messenger_deinit(&context->rse_comms);

	free(context);
	rpc_caller->context = NULL;

	return RPC_SUCCESS;
}
