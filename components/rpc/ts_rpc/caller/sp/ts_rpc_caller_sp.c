/*
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ts_rpc_caller_sp.h"
#include <sp_memory_management.h>
#include <sp_messaging.h>
#include <string.h>
#include <malloc.h>
#include <ffa_api.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <trace.h>

#ifndef MAX_RPC_ENDPOINT_NUM
#define MAX_RPC_ENDPOINT_NUM ((uint8_t)(16))
#endif /* MAX_RPC_ENDPOINT_NUM */

#ifndef MAX_SHM_NUM
#define MAX_SHM_NUM ((uint8_t)(16))
#endif /* MAX_SHM_NUM */

static const struct sp_uuid ts_ffa_uuid = {
	.uuid = { 0xbd, 0xcd, 0x76, 0xd7, 0x82, 0x5e, 0x47, 0x51,
		  0x96, 0x3b, 0x86, 0xd4, 0xf8, 0x49, 0x43, 0xac}
};

struct ts_rpc_caller_sp_context {
	uint16_t own_id;
	uint16_t endpoint_id;
	uint8_t interface_id;
	struct rpc_caller_shared_memory shared_memories[MAX_SHM_NUM];
	bool has_opened_session;
};

static struct rpc_caller_shared_memory *find_shared_memory_descriptor(
	struct ts_rpc_caller_sp_context *context, uint64_t id)
{
	size_t i = 0;

	for (i = 0; i < MAX_SHM_NUM; i++) {
		if (context->shared_memories[i].id == id)
			return &context->shared_memories[i];
	}

	return NULL;
}

rpc_status_t open_session(void *context, const struct rpc_uuid *service_uuid, uint16_t endpoint_id)
{
	struct ts_rpc_caller_sp_context *this_context = (struct ts_rpc_caller_sp_context *)context;
	sp_result sp_res = SP_RESULT_INTERNAL_ERROR;
	rpc_status_t status = RPC_ERROR_INTERNAL;
	struct sp_msg req = { 0 };
	struct sp_msg resp = { 0 };
	uint8_t interface_id = 0;

	if (!context || !service_uuid) {
		EMSG("invalid arguments");
		status = RPC_ERROR_INVALID_VALUE;
		goto out;
	}

	if (this_context->has_opened_session) {
		EMSG("the session is already open");
		status = RPC_ERROR_INVALID_STATE;
		goto out;
	}

	if (endpoint_id == this_context->own_id) {
		EMSG("cannot open RPC session to self");
		status = RPC_ERROR_INVALID_VALUE;
		goto out;
	}

	req.source_id = this_context->own_id;
	req.destination_id = endpoint_id;
	req.is_64bit_message = false;

	ts_rpc_abi_set_management_interface_id(req.args.args32);
	ts_rpc_abi_set_opcode(req.args.args32, TS_RPC_ABI_MANAGEMENT_OPCODE_VERSION);

	sp_res = sp_msg_send_direct_req(&req, &resp);
	if (sp_res != SP_RESULT_OK) {
		EMSG("sp_msg_send_direct_req(): error %"PRId32, sp_res);
		goto out;
	}

	if (ts_rpc_abi_get_version(resp.args.args32) != TS_RPC_PROTOCOL_VERSION) {
		EMSG("endpoint %"PRIu16" does not support protocol version %"PRIu32"",
			endpoint_id, TS_RPC_PROTOCOL_VERSION);
		goto out;
	}

	memset(req.args.args32, 0, sizeof(req.args.args32));
	memset(resp.args.args32, 0, sizeof(resp.args.args32));

	ts_rpc_abi_set_management_interface_id(req.args.args32);
	ts_rpc_abi_set_opcode(req.args.args32, TS_RPC_ABI_MANAGEMENT_OPCODE_INTERFACE_ID_QUERY);
	ts_rpc_abi_set_uuid(req.args.args32, service_uuid);

	sp_res = sp_msg_send_direct_req(&req, &resp);
	if (sp_res != SP_RESULT_OK) {
		EMSG("sp_msg_send_direct_req(): error %"PRId32, sp_res);
		goto out;
	}

	status = ts_rpc_abi_get_rpc_status(resp.args.args32);
	if (status == RPC_SUCCESS) {
		interface_id = ts_rpc_abi_get_queried_interface_id(resp.args.args32);

		this_context->endpoint_id = endpoint_id;
		this_context->interface_id = interface_id;
		this_context->has_opened_session = true;
	}

out:
	return status;
}

rpc_status_t find_and_open_session(void *context, const struct rpc_uuid *service_uuid)
{
	struct ts_rpc_caller_sp_context *this_context = (struct ts_rpc_caller_sp_context *)context;
	sp_result sp_res = SP_RESULT_INTERNAL_ERROR;
	ffa_result ffa_res = FFA_INVALID_PARAMETERS;
	rpc_status_t status = RPC_SUCCESS;
	uint32_t rpc_endpoint_count = MAX_RPC_ENDPOINT_NUM;
	struct sp_partition_info rpc_endpoints[MAX_RPC_ENDPOINT_NUM] = {0};
	uint32_t i = 0;

	if (!context || !service_uuid) {
		EMSG("invalid arguments");
		return RPC_ERROR_INVALID_VALUE;
	}

	if (this_context->has_opened_session) {
		EMSG("the session is already open");
		return RPC_ERROR_INVALID_STATE;
	}

	sp_res = sp_discovery_partition_info_get(&ts_ffa_uuid, rpc_endpoints, &rpc_endpoint_count);
	if (sp_res != SP_RESULT_OK) {
		EMSG("sp_discovery_partition_info_get(): error %"PRId32, sp_res);
		return RPC_ERROR_INTERNAL;
	}

	ffa_res = ffa_rx_release();
	if (ffa_res != FFA_OK) {
		EMSG("ffa_rx_release(): error %"PRId32, ffa_res);
		return 0;
	}

	for (i = 0; i < rpc_endpoint_count; i++) {
		if (rpc_endpoints[i].partition_id == this_context->own_id)
			continue;

		status = open_session(context, service_uuid, rpc_endpoints[i].partition_id);

		if (status == RPC_SUCCESS)
			return RPC_SUCCESS;
	}

	EMSG("no SP found supporting protocol version %"PRIu32" and the requested service",
	     TS_RPC_PROTOCOL_VERSION);
	return RPC_ERROR_INTERNAL;
}

rpc_status_t create_shared_memory(void *context, size_t size,
				  struct rpc_caller_shared_memory *shared_memory)
{
	struct ts_rpc_caller_sp_context *this_context = (struct ts_rpc_caller_sp_context *)context;
	sp_result sp_res = SP_RESULT_OK;
	rpc_status_t status = RPC_ERROR_INTERNAL;
	struct sp_msg req = { 0 };
	struct sp_msg resp = { 0 };

	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region region = { 0 };

	uint64_t handle = 0;
	void *buffer = NULL;
	struct rpc_caller_shared_memory *stored_shared_memory_desc = NULL;

	if (!context || !shared_memory) {
		EMSG("invalid arguments");
		status = RPC_ERROR_INVALID_VALUE;
		goto err;
	}

	if (!this_context->has_opened_session) {
		EMSG("session should be opened before creating shared memory");
		status = RPC_ERROR_INVALID_STATE;
		goto err;
	}

	if (size == 0) {
		shared_memory->buffer = NULL;
		shared_memory->size = 0;
		shared_memory->id = FFA_MEM_HANDLE_INVALID;

		return RPC_SUCCESS;
	}

	stored_shared_memory_desc = find_shared_memory_descriptor(this_context,
								  FFA_MEM_HANDLE_INVALID);
	if (!stored_shared_memory_desc) {
		status = RPC_ERROR_INTERNAL;
		EMSG("cannot find empty shared memory descriptor");
		goto err;
	}

	size = ROUNDUP(size, FFA_MEM_TRANSACTION_PAGE_SIZE);
	buffer = memalign(FFA_MEM_TRANSACTION_PAGE_SIZE, size);
	if (!buffer) {
		EMSG("memalign(): failed to allocate %lu bytes with %"PRIu32" alignment", size,
		     FFA_MEM_TRANSACTION_PAGE_SIZE);
		status = RPC_ERROR_INTERNAL;
		goto err;
	}
	memset(buffer, 0, size);

	desc.sender_id = this_context->own_id;
	desc.memory_type = sp_memory_type_normal_memory;
	desc.mem_region_attr.normal_memory.cacheability = sp_cacheability_write_back;
	desc.mem_region_attr.normal_memory.shareability = sp_shareability_inner_shareable;

	acc_desc.data_access = sp_data_access_read_write;
	acc_desc.instruction_access = sp_instruction_access_not_specified;
	acc_desc.receiver_id = this_context->endpoint_id;

	region.address = buffer;
	region.page_count = (uint32_t)ROUNDUP_DIV(size, FFA_MEM_TRANSACTION_PAGE_SIZE);

	sp_res = sp_memory_share(&desc, &acc_desc, 1, &region, 1, &handle);
	if (sp_res != SP_RESULT_OK) {
		EMSG("sp_memory_share(): error %"PRId32, sp_res);
		status = RPC_ERROR_INTERNAL;
		goto err;
	}

	req.source_id = this_context->own_id;
	req.destination_id = this_context->endpoint_id;
	req.is_64bit_message = false;

	ts_rpc_abi_set_management_interface_id(req.args.args32);
	ts_rpc_abi_set_opcode(req.args.args32, TS_RPC_ABI_MANAGEMENT_OPCODE_MEMORY_RETRIEVE);
	ts_rpc_abi_set_memory_handle(req.args.args32, handle);
	ts_rpc_abi_set_memory_tag(req.args.args32, 0);

	sp_res = sp_msg_send_direct_req(&req, &resp);
	if (sp_res != SP_RESULT_OK) {
		EMSG("sp_msg_send_direct_req(): error %"PRId32, sp_res);
		status = RPC_ERROR_INTERNAL;
		goto err;
	}

	status = ts_rpc_abi_get_rpc_status(resp.args.args32);
	if (status != RPC_SUCCESS) {
		EMSG("RPC endpoint error: %"PRId32, status);

		sp_res = sp_memory_reclaim(handle, 0);
		if (sp_res != SP_RESULT_OK)
			EMSG("sp_memory_reclaim(): error %"PRId32, sp_res);

		status = RPC_ERROR_INTERNAL;
		goto err;
	}

	shared_memory->id = handle;
	shared_memory->buffer = buffer;
	shared_memory->size = size;

	*stored_shared_memory_desc = *shared_memory;

	return RPC_SUCCESS;

err:
	if (buffer)
		free(buffer);
	return status;
}

rpc_status_t release_shared_memory(void *context, struct rpc_caller_shared_memory *shared_memory)
{
	struct ts_rpc_caller_sp_context *this_context = (struct ts_rpc_caller_sp_context *)context;
	sp_result sp_res = SP_RESULT_OK;
	rpc_status_t status = RPC_ERROR_INTERNAL;
	struct sp_msg req = { 0 };
	struct sp_msg resp = { 0 };

	struct rpc_caller_shared_memory *to_release = NULL;

	if (!context || !shared_memory) {
		EMSG("invalid arguments");
		return RPC_ERROR_INVALID_VALUE;
	}

	if (shared_memory->id == FFA_MEM_HANDLE_INVALID)
		return RPC_SUCCESS;

	to_release = find_shared_memory_descriptor(this_context, shared_memory->id);
	if (!to_release) {
		EMSG("cannot find shared memory with specified handle");
		return RPC_ERROR_INTERNAL;
	}

	req.source_id = this_context->own_id;
	req.destination_id = this_context->endpoint_id;
	req.is_64bit_message = false;

	ts_rpc_abi_set_management_interface_id(req.args.args32);
	ts_rpc_abi_set_opcode(req.args.args32, TS_RPC_ABI_MANAGEMENT_OPCODE_MEMORY_RELINQUISH);
	ts_rpc_abi_set_memory_handle(req.args.args32, to_release->id);

	sp_res = sp_msg_send_direct_req(&req, &resp);
	if (sp_res != SP_RESULT_OK) {
		EMSG("sp_msg_send_direct_req(): error %"PRId32, sp_res);
		return RPC_ERROR_INTERNAL;
	}

	status = ts_rpc_abi_get_rpc_status(resp.args.args32);
	if (status != RPC_SUCCESS) {
		/*
		 * The RPC endpoint failed to relinquish the shared memory but
		 * still worth trying to reclaim the memory beside emitting an
		 * error message.
		 */
		EMSG("RPC endpoint error %"PRId32, status);
	}

	sp_res = sp_memory_reclaim(to_release->id, 0);
	if (sp_res != SP_RESULT_OK) {
		EMSG("sp_memory_reclaim(): error %"PRId32, sp_res);
		return RPC_ERROR_INTERNAL;
	}

	/*
	 * Only remove shared memory entry and free it if both relinquish AND reclaim were
	 * successful.
	 */
	if (status == RPC_SUCCESS) {
		free(to_release->buffer);

		to_release->id = FFA_MEM_HANDLE_INVALID;
		to_release->buffer = NULL;
		to_release->size = 0;
	}

	return status;
}

rpc_status_t call(void *context, uint16_t opcode, struct rpc_caller_shared_memory *shared_memory,
		  size_t request_length, size_t *response_length, service_status_t *service_status)
{

	struct ts_rpc_caller_sp_context *this_context = (struct ts_rpc_caller_sp_context *)context;
	sp_result sp_res = SP_RESULT_OK;
	rpc_status_t status = RPC_ERROR_INTERNAL;
	struct sp_msg req = { 0 };
	struct sp_msg resp = { 0 };

	if (!context || !shared_memory || !response_length || !service_status) {
		EMSG("invalid arguments");
		status = RPC_ERROR_INVALID_VALUE;
		goto out;
	}

	if (!this_context->has_opened_session) {
		EMSG("session should be opened before calling a service");
		status = RPC_ERROR_INVALID_STATE;
		goto out;
	}

	if (shared_memory->id != FFA_MEM_HANDLE_INVALID) {
		/* Checking if the shared memory was allocated by us */
		struct rpc_caller_shared_memory *stored_shared_memory = NULL;

		stored_shared_memory = find_shared_memory_descriptor(this_context,
								     shared_memory->id);
		if (!stored_shared_memory) {
			EMSG("cannot find shared memory with specified handle");
			status = RPC_ERROR_INTERNAL;
			goto out;
		}
	} else {
		/* Call with no shared memory, the request length must be 0 */
		if (request_length != 0) {
			status = RPC_ERROR_INVALID_VALUE;
			goto out;
		}
	}

	req.source_id = this_context->own_id;
	req.destination_id = this_context->endpoint_id;
	req.is_64bit_message = false;

	ts_rpc_abi_set_interface_id(req.args.args32, this_context->interface_id);
	ts_rpc_abi_set_opcode(req.args.args32, opcode);
	ts_rpc_abi_set_memory_handle(req.args.args32, shared_memory->id);
	ts_rpc_abi_set_request_length(req.args.args32, request_length);
	ts_rpc_abi_set_client_id(req.args.args32, 0);

	sp_res = sp_msg_send_direct_req(&req, &resp);
	if (sp_res != SP_RESULT_OK) {
		EMSG("sp_msg_send_direct_req(): error %"PRId32, sp_res);
		status = RPC_ERROR_INTERNAL;
		goto out;
	}

	status = ts_rpc_abi_get_rpc_status(resp.args.args32);
	if (status != RPC_SUCCESS) {
		EMSG("RPC endpoint error %"PRId32, status);
		goto out;
	}

	*service_status = ts_rpc_abi_get_service_status(resp.args.args32);
	*response_length = ts_rpc_abi_get_response_length(resp.args.args32);

out:
	return status;
}

rpc_status_t close_session(void *context)
{
	struct ts_rpc_caller_sp_context *this_context = (struct ts_rpc_caller_sp_context *)context;
	rpc_status_t status = RPC_ERROR_INTERNAL;

	uint8_t i = 0;

	if (!context) {
		EMSG("invalid arguments");
		return RPC_ERROR_INVALID_VALUE;
	}

	if (!this_context->has_opened_session) {
		EMSG("session is already closed");
		status = RPC_ERROR_INVALID_STATE;
		goto out;
	}

	for (i = 0; i < MAX_SHM_NUM; i++) {
		if (this_context->shared_memories[i].id != FFA_MEM_HANDLE_INVALID) {
			status = release_shared_memory(context, &this_context->shared_memories[i]);

			if (status != RPC_SUCCESS) {
				EMSG("failed to release shared memory with handle %"PRIu64"",
					this_context->shared_memories[i].id);
				goto out;
			}
		}
	}

	this_context->endpoint_id = 0;
	this_context->interface_id = 0;
	this_context->has_opened_session = false;

	return RPC_SUCCESS;

out:
	return status;
}

rpc_status_t ts_rpc_caller_sp_init(struct rpc_caller_interface *rpc_caller)
{
	struct ts_rpc_caller_sp_context *context = NULL;
	sp_result result = SP_RESULT_INTERNAL_ERROR;
	uint16_t own_id = 0;
	uint16_t major = 0;
	uint16_t minor = 0;
	uint8_t i = 0;

	if (!rpc_caller || rpc_caller->context)
		return RPC_ERROR_INVALID_VALUE;

	result = sp_discovery_ffa_version_get(&major, &minor);
	if (result != SP_RESULT_OK)
		return RPC_ERROR_TRANSPORT_LAYER;

	if (major != FFA_VERSION_MAJOR || minor < FFA_VERSION_MINOR)
		return RPC_ERROR_TRANSPORT_LAYER;

	if (sp_discovery_own_id_get(&own_id) != SP_RESULT_OK)
		return RPC_ERROR_INTERNAL;

	context = (struct ts_rpc_caller_sp_context *)
		calloc(1, sizeof(struct ts_rpc_caller_sp_context));
	if (!context)
		return RPC_ERROR_INTERNAL;

	for (i = 0; i < MAX_SHM_NUM; i++)
		context->shared_memories[i].id = FFA_MEM_HANDLE_INVALID;

	context->own_id = own_id;

	rpc_caller->context = context;
	rpc_caller->open_session = open_session;
	rpc_caller->find_and_open_session = find_and_open_session;
	rpc_caller->close_session = close_session;
	rpc_caller->create_shared_memory = create_shared_memory;
	rpc_caller->release_shared_memory = release_shared_memory;
	rpc_caller->call = call;

	return RPC_SUCCESS;
}

rpc_status_t ts_rpc_caller_sp_deinit(struct rpc_caller_interface *rpc_caller)
{
	struct ts_rpc_caller_sp_context *this_context =
		(struct ts_rpc_caller_sp_context *)rpc_caller->context;
	rpc_status_t status = RPC_ERROR_INTERNAL;

	if (!rpc_caller || !rpc_caller->context)
		return RPC_ERROR_INVALID_VALUE;

	if (!this_context->has_opened_session) {
		status = rpc_caller_close_session(rpc_caller);
		if (status != RPC_SUCCESS) {
			EMSG("failed to close session");
			return RPC_ERROR_INTERNAL;
		}
	}

	free(rpc_caller->context);
	rpc_caller->context = NULL;

	return RPC_SUCCESS;
}
