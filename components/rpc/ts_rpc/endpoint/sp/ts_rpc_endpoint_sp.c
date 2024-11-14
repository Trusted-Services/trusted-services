/*
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ts_rpc_endpoint_sp.h"
#include "sp_discovery.h"
#include "sp_memory_management.h"
#include "trace.h"
#include <stdlib.h>
#include <string.h>

static const struct ts_rpc_shared_memory null_shared_memory = {
	.owner_id = 0xffff, .handle = FFA_MEM_HANDLE_INVALID, .data = NULL, .size = 0, .used = true
};

static struct ts_rpc_shared_memory *find_free_shared_memory_descriptor(
	struct ts_rpc_endpoint_sp *endpoint)
{
	struct ts_rpc_shared_memory *memory = endpoint->shared_memories;
	struct ts_rpc_shared_memory *end = memory + endpoint->shared_memory_count;

	for (; memory < end; memory++)
		if (!memory->used)
			return memory;

	return NULL;
}

static struct ts_rpc_shared_memory *find_shared_memory_descriptor(
	struct ts_rpc_endpoint_sp *endpoint, uint16_t owner_id, uint64_t handle)
{
	struct ts_rpc_shared_memory *memory = endpoint->shared_memories;
	struct ts_rpc_shared_memory *end = memory + endpoint->shared_memory_count;

	for (; memory < end; memory++)
		if (memory->used && memory->owner_id == owner_id && memory->handle == handle)
			return memory;

	return NULL;
}

static rpc_status_t handle_memory_retrieve(struct ts_rpc_endpoint_sp *endpoint, uint16_t source_id,
					   uint64_t memory_handle, uint64_t memory_tag)
{
	sp_result sp_res = SP_RESULT_INTERNAL_ERROR;
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region region = { 0 };
	uint32_t in_region_count = 0;
	uint32_t out_region_count = 1;
	struct ts_rpc_shared_memory *memory = NULL;

	(void)memory_tag;

	memory = find_free_shared_memory_descriptor(endpoint);
	if (!memory) {
		EMSG("No available shared memory slot");
		return RPC_ERROR_NOT_FOUND;
	}

	desc.sender_id = source_id;
	desc.memory_type = sp_memory_type_not_specified;
	desc.flags.transaction_type = sp_memory_transaction_type_share;
	acc_desc.receiver_id = endpoint->own_id;
	acc_desc.data_access = sp_data_access_read_write;

	sp_res = sp_memory_retrieve(&desc, &acc_desc, &region, in_region_count,
				    &out_region_count, memory_handle);
	if (sp_res != SP_RESULT_OK) {
		EMSG("Failed to retrieve memory: %d", sp_res);
		return RPC_ERROR_TRANSPORT_LAYER;
	}

	memory->owner_id = source_id;
	memory->handle = memory_handle;
	memory->data = region.address;
	memory->size = region.page_count * FFA_MEM_TRANSACTION_PAGE_SIZE;
	memory->used = true;

	return RPC_SUCCESS;
}

static rpc_status_t handle_memory_relinquish(struct ts_rpc_endpoint_sp *endpoint,
					     uint16_t source_id, uint64_t memory_handle)
{
	sp_result sp_res = SP_RESULT_INTERNAL_ERROR;
	uint16_t endpoints[1] = { 0 };
	uint32_t endpoint_count = 1;
	struct sp_memory_transaction_flags flags = {
		.zero_memory = false,
		.operation_time_slicing = false,
	};
	struct ts_rpc_shared_memory *memory = NULL;

	memory = find_shared_memory_descriptor(endpoint, source_id, memory_handle);
	if (!memory) {
		EMSG("Shared memory not found");
		return RPC_ERROR_NOT_FOUND;
	}

	endpoints[0] = endpoint->own_id;

	sp_res = sp_memory_relinquish(memory->handle, endpoints, endpoint_count, &flags);
	if (sp_res != SP_RESULT_OK) {
		EMSG("Failed to relinquish memory: %d", sp_res);
		return RPC_ERROR_TRANSPORT_LAYER;
	}

	*memory = (struct ts_rpc_shared_memory){ 0 };

	return RPC_SUCCESS;
}

static rpc_status_t handle_interface_id_query(struct ts_rpc_endpoint_sp *endpoint,
					      const struct rpc_uuid *service_uuid,
					      uint8_t *interface_id)
{
	size_t i = 0;

	for (i = 0; i < endpoint->service_count; i++) {
		if (endpoint->services[i] &&
		    rpc_uuid_equal(service_uuid, &endpoint->services[i]->uuid)) {
			*interface_id = i;
			return RPC_SUCCESS;
		}
	}

	*interface_id = 0;
	return RPC_ERROR_NOT_FOUND;
}

static void handle_management_interface(struct ts_rpc_endpoint_sp *endpoint, uint16_t source_id,
					const uint32_t request[5], uint32_t response[5])
{
	uint16_t opcode = ts_rpc_abi_get_opcode(request);
	rpc_status_t status = RPC_ERROR_INTERNAL;

	switch (opcode) {
	case TS_RPC_ABI_MANAGEMENT_OPCODE_VERSION:
		ts_rpc_abi_set_version(response, TS_RPC_ABI_VERSION_V1);
		/* Version call doesn't return RPC status, return from the function here */
		return;

	case TS_RPC_ABI_MANAGEMENT_OPCODE_MEMORY_RETRIEVE: {
		uint64_t memory_handle = ts_rpc_abi_get_memory_handle(request);
		uint64_t memory_tag = ts_rpc_abi_get_memory_tag(request);

		status = handle_memory_retrieve(endpoint, source_id, memory_handle, memory_tag);
		break;
	}

	case TS_RPC_ABI_MANAGEMENT_OPCODE_MEMORY_RELINQUISH: {
		uint64_t memory_handle = ts_rpc_abi_get_memory_handle(request);

		status = handle_memory_relinquish(endpoint, source_id, memory_handle);
		break;
	}

	case TS_RPC_ABI_MANAGEMENT_OPCODE_INTERFACE_ID_QUERY: {
		struct rpc_uuid uuid = { 0 };
		uint8_t interface_id = 0;

		ts_rpc_abi_get_uuid(request, &uuid);
		status = handle_interface_id_query(endpoint, &uuid, &interface_id);
		if (status == RPC_SUCCESS)
			ts_rpc_abi_set_queried_interface_id(response, interface_id);
		break;
	}

	default:
		status = RPC_ERROR_INVALID_VALUE;
	}

	ts_rpc_abi_set_rpc_status(response, status);
}

static void handle_service_interfaces(struct ts_rpc_endpoint_sp *endpoint, uint16_t source_id,
				      const uint32_t request[5], uint32_t response[5])
{
	uint8_t interface_id = 0;
	uint64_t memory_handle = 0;
	uint32_t request_length = 0;
	struct rpc_service_interface *service = NULL;
	const struct ts_rpc_shared_memory *memory = NULL;
	struct rpc_request rpc_request = { 0 };
	rpc_status_t status = RPC_ERROR_INTERNAL;

	interface_id = ts_rpc_abi_get_interface_id(request);
	if (interface_id >= endpoint->service_count) {
		status = RPC_ERROR_INVALID_VALUE;
		goto out;
	}

	service = endpoint->services[interface_id];
	if (!service) {
		status = RPC_ERROR_INTERNAL;
		goto out;
	}

	memory_handle = ts_rpc_abi_get_memory_handle(request);
	if (memory_handle != FFA_MEM_HANDLE_INVALID) {
		/* Normal call with data in the shared memory */
		memory = find_shared_memory_descriptor(endpoint, source_id, memory_handle);
		if (!memory) {
			status = RPC_ERROR_NOT_FOUND;
			goto out;
		}
	} else {
		/* Call without a shared memory */
		memory = &null_shared_memory;
	}

	request_length = ts_rpc_abi_get_request_length(request);
	if (request_length > memory->size) {
		status = RPC_ERROR_INVALID_VALUE;
		goto out;
	}

	rpc_request.request.data = memory->data;
	rpc_request.request.data_length = request_length;
	rpc_request.request.size = memory->size;

	rpc_request.response.data = memory->data;
	rpc_request.response.data_length = 0;
	rpc_request.response.size = memory->size;

	rpc_request.source_id = source_id;
	rpc_request.interface_id = interface_id;
	rpc_request.opcode = ts_rpc_abi_get_opcode(request);
	rpc_request.client_id = ts_rpc_abi_get_client_id(request);

	status = rpc_service_receive(service, &rpc_request);
	if (status == RPC_SUCCESS) {
		ts_rpc_abi_set_response_length(response, rpc_request.response.data_length);
		ts_rpc_abi_set_service_status(response, rpc_request.service_status);
	}

out:
	ts_rpc_abi_set_rpc_status(response, status);
}

rpc_status_t ts_rpc_endpoint_sp_init(struct ts_rpc_endpoint_sp *endpoint, size_t service_count,
				     size_t shared_memory_count)
{
	sp_result result = SP_RESULT_OK;
	uint16_t major = 0;
	uint16_t minor = 0;

	if (!endpoint || !service_count)
		return RPC_ERROR_INVALID_VALUE;

	result = sp_discovery_ffa_version_get(&major, &minor);
	if (result != SP_RESULT_OK)
		return RPC_ERROR_TRANSPORT_LAYER;

	if (major != FFA_VERSION_MAJOR || minor < FFA_VERSION_MINOR)
		return RPC_ERROR_TRANSPORT_LAYER;

	result = sp_discovery_own_id_get(&endpoint->own_id);
	if (result != SP_RESULT_OK)
		return RPC_ERROR_TRANSPORT_LAYER;

	endpoint->services = calloc(service_count, sizeof(struct rpc_service_interface *));
	if (!endpoint->services)
		return RPC_ERROR_RESOURCE_FAILURE;

	endpoint->service_count = service_count;
	endpoint->shared_memories = calloc(shared_memory_count,
					   sizeof(struct ts_rpc_shared_memory));
	if (!endpoint->shared_memories) {
		free(endpoint->services);
		return RPC_ERROR_RESOURCE_FAILURE;
	}

	endpoint->shared_memory_count = shared_memory_count;

	return RPC_SUCCESS;
}

rpc_status_t ts_rpc_endpoint_sp_deinit(struct ts_rpc_endpoint_sp *endpoint)
{
	struct ts_rpc_shared_memory *memory = NULL;
	struct ts_rpc_shared_memory *end = NULL;
	rpc_status_t status = RPC_SUCCESS;

	if (!endpoint)
		return RPC_ERROR_INVALID_VALUE;

	memory = endpoint->shared_memories;
	end = memory + endpoint->shared_memory_count;

	for (; memory < end; memory++) {
		status = handle_memory_relinquish(endpoint, memory->owner_id, memory->handle);
		if (status)
			return status;
	}

	free(endpoint->services);
	free(endpoint->shared_memories);

	*endpoint = (struct ts_rpc_endpoint_sp){ 0 };

	return RPC_SUCCESS;
}

rpc_status_t ts_rpc_endpoint_sp_add_service(struct ts_rpc_endpoint_sp *endpoint,
					    struct rpc_service_interface *service)
{
	size_t i = 0;

	if (!endpoint || !service)
		return RPC_ERROR_INVALID_VALUE;

	for (i = 0; i < endpoint->service_count; i++) {
		if (!endpoint->services[i]) {
			endpoint->services[i] = service;
			return RPC_SUCCESS;
		}
	}

	return RPC_ERROR_RESOURCE_FAILURE;
}

void ts_rpc_endpoint_sp_receive(struct ts_rpc_endpoint_sp *endpoint, const struct sp_msg *request,
				struct sp_msg *response)
{
	response->source_id = request->destination_id;
	response->destination_id = request->source_id;
	response->is_64bit_message = request->is_64bit_message;
	memset(&response->args, 0x00, sizeof(response->args));
	ts_rpc_abi_copy_control_reg(response->args.args32, request->args.args32);

	if (!request->is_64bit_message) {
		if (ts_rpc_abi_is_management_interface_id(request->args.args32)) {
			handle_management_interface(endpoint, request->source_id,
						    request->args.args32, response->args.args32);
		} else {
			handle_service_interfaces(endpoint, request->source_id,
						  request->args.args32, response->args.args32);
		}
	} else {
		EMSG("64 bit FF-A messages are not supported by the TS RPC layer, src = 0x%04x",
		     request->source_id);
		ts_rpc_abi_set_rpc_status(response->args.args32, RPC_ERROR_INVALID_VALUE);
	}
}
