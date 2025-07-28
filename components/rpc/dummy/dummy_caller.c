/*
 * Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "dummy_caller.h"
#include <stdlib.h>

struct dummy_caller_context {
	rpc_status_t rpc_status;
	service_status_t service_status;
	uint8_t *req_buf;
};

rpc_status_t open_session(void *context, const struct rpc_uuid *service_uuid, uint16_t endpoint_id)
{
	(void)context;
	(void)service_uuid;
	(void)endpoint_id;

	return RPC_SUCCESS;
}

rpc_status_t find_and_open_session(void *context, const struct rpc_uuid *service_uuid)
{
	(void)context;
	(void)service_uuid;

	return RPC_SUCCESS;
}

static rpc_status_t close_session(void *context)
{
	struct dummy_caller_context *caller_context = (struct dummy_caller_context *)context;

	free(caller_context->req_buf);

	return RPC_SUCCESS;
}

static rpc_status_t create_shared_memory(void *context, size_t size,
					 struct rpc_caller_shared_memory *shared_memory)
{
	rpc_status_t status = RPC_ERROR_RESOURCE_FAILURE;
	struct dummy_caller_context *caller_context = (struct dummy_caller_context *)context;

	if (caller_context->req_buf)
		return RPC_ERROR_INVALID_STATE;

	caller_context->req_buf = calloc(1, size);
	shared_memory->buffer = caller_context->req_buf;
	shared_memory->id = 0;
	if (caller_context->req_buf != NULL) {
		shared_memory->size = size;
		status = RPC_SUCCESS;
	} else {
		shared_memory->size = 0;
	}

	return status;
}

static rpc_status_t release_shared_memory(void *context,
					  struct rpc_caller_shared_memory *shared_memory)
{
	struct dummy_caller_context *caller_context = (struct dummy_caller_context *)context;

	if (shared_memory->buffer != caller_context->req_buf)
		return RPC_ERROR_INVALID_VALUE;

	if (!caller_context->req_buf)
		return RPC_ERROR_INVALID_STATE;

	free(caller_context->req_buf);
	caller_context->req_buf = NULL;

	return RPC_ERROR_INTERNAL;
}

static rpc_status_t call(void *context, uint16_t opcode,
			 struct rpc_caller_shared_memory *shared_memory, size_t request_length,
			 size_t *response_length, service_status_t *service_status)
{
	struct dummy_caller_context *caller_context = (struct dummy_caller_context *)context;

	(void)opcode;
	(void)shared_memory;
	(void)request_length;

	*response_length = 0;
	*service_status = caller_context->rpc_status;

	return caller_context->rpc_status;
}

rpc_status_t dummy_caller_init(struct rpc_caller_interface *rpc_caller, rpc_status_t rpc_status,
			       service_status_t service_status)
{
	struct dummy_caller_context *context = NULL;

	if (!rpc_caller || rpc_caller->context)
		return RPC_ERROR_INVALID_VALUE;

	context = (struct dummy_caller_context *)calloc(1, sizeof(struct dummy_caller_context));
	if (!context)
		return RPC_ERROR_INTERNAL;

	context->rpc_status = rpc_status;
	context->service_status = service_status;
	context->req_buf = NULL;

	rpc_caller->context = context;
	rpc_caller->open_session = open_session;
	rpc_caller->find_and_open_session = find_and_open_session;
	rpc_caller->close_session = close_session;
	rpc_caller->create_shared_memory = create_shared_memory;
	rpc_caller->release_shared_memory = release_shared_memory;
	rpc_caller->call = call;

	return RPC_SUCCESS;
}

rpc_status_t dummy_caller_deinit(struct rpc_caller_interface *rpc_caller)
{
	if (!rpc_caller || !rpc_caller->context)
		return RPC_ERROR_INVALID_VALUE;

	free(rpc_caller->context);

	return RPC_SUCCESS;
}
