/*
 * Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fwu_provider.h"

#include <stddef.h>

#include "common/uuid/uuid.h"
#include "protocols/rpc/common/packed-c/status.h"
#include "protocols/service/fwu/opcodes.h"
#include "service/fwu/common/update_agent_interface.h"
#include "service/fwu/provider/serializer/fwu_provider_serializer.h"
#include "fwu_uuid.h"

#ifndef FWU_PROVIDER_MAX_PARTIAL_UPDATE_COUNT
#define FWU_PROVIDER_MAX_PARTIAL_UPDATE_COUNT	(4)
#endif /* FWU_PROVIDER_MAX_PARTIAL_UPDATE_COUNT */

/* Service request handlers */
static rpc_status_t discover_handler(void *context, struct rpc_request *req);
static rpc_status_t begin_staging_handler(void *context, struct rpc_request *req);
static rpc_status_t end_staging_handler(void *context, struct rpc_request *req);
static rpc_status_t cancel_staging_handler(void *context, struct rpc_request *req);
static rpc_status_t open_handler(void *context, struct rpc_request *req);
static rpc_status_t write_stream_handler(void *context, struct rpc_request *req);
static rpc_status_t read_stream_handler(void *context, struct rpc_request *req);
static rpc_status_t commit_handler(void *context, struct rpc_request *req);
static rpc_status_t accept_image_handler(void *context, struct rpc_request *req);
static rpc_status_t select_previous_handler(void *context, struct rpc_request *req);

/* Handler mapping table for service */
static const struct service_handler handler_table[] = {
	{ FWU_FUNC_ID_DISCOVER, discover_handler },
	{ FWU_FUNC_ID_BEGIN_STAGING, begin_staging_handler },
	{ FWU_FUNC_ID_END_STAGING, end_staging_handler },
	{ FWU_FUNC_ID_CANCEL_STAGING, cancel_staging_handler },
	{ FWU_FUNC_ID_OPEN, open_handler },
	{ FWU_FUNC_ID_WRITE_STREAM, write_stream_handler },
	{ FWU_FUNC_ID_READ_STREAM, read_stream_handler },
	{ FWU_FUNC_ID_COMMIT, commit_handler },
	{ FWU_FUNC_ID_ACCEPT_IMAGE, accept_image_handler },
	{ FWU_FUNC_ID_SELECT_PREVIOUS, select_previous_handler }
};

struct rpc_service_interface *fwu_provider_init(struct fwu_provider *context,
					struct update_agent *update_agent)
{
	const struct rpc_uuid service_uuid = { .uuid = TS_FWU_SERVICE_UUID };
	struct rpc_service_interface *rpc_interface = NULL;

	/* Initialise the fwu_provider */
	context->update_agent = update_agent;

	service_provider_init(&context->base_provider, context, &service_uuid, handler_table,
			      sizeof(handler_table) / sizeof(struct service_handler));

	rpc_interface = service_provider_get_rpc_interface(&context->base_provider);
	if (!rpc_interface)
		return NULL;

	return fwu_provider_shim_init(&context->shim, rpc_interface);
}

void fwu_provider_deinit(struct fwu_provider *context)
{
	(void)context;
}

static uint16_t generate_function_presence(const struct update_agent *agent,
					   uint8_t function_presence[FWU_FUNC_ID_COUNT])
{
	uint16_t num_func = 0;

#define ADD_FUNC_IF_PRESENT(func, id) \
do { \
	if (agent->interface->func != NULL) \
		function_presence[num_func++] = (id); \
} while (0)

	ADD_FUNC_IF_PRESENT(discover, FWU_FUNC_ID_DISCOVER);
	ADD_FUNC_IF_PRESENT(begin_staging, FWU_FUNC_ID_BEGIN_STAGING);
	ADD_FUNC_IF_PRESENT(end_staging, FWU_FUNC_ID_END_STAGING);
	ADD_FUNC_IF_PRESENT(cancel_staging, FWU_FUNC_ID_CANCEL_STAGING);
	ADD_FUNC_IF_PRESENT(open, FWU_FUNC_ID_OPEN);
	ADD_FUNC_IF_PRESENT(write_stream, FWU_FUNC_ID_WRITE_STREAM);
	ADD_FUNC_IF_PRESENT(read_stream, FWU_FUNC_ID_READ_STREAM);
	ADD_FUNC_IF_PRESENT(commit, FWU_FUNC_ID_COMMIT);
	ADD_FUNC_IF_PRESENT(accept_image, FWU_FUNC_ID_ACCEPT_IMAGE);
	ADD_FUNC_IF_PRESENT(select_previous, FWU_FUNC_ID_SELECT_PREVIOUS);

#undef ADD_FUNC_IF_PRESENT

	return num_func;
}

static rpc_status_t discover_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct fwu_provider *this_instance = (struct fwu_provider *)context;
	struct fwu_discovery_result discovery_result = { 0 };
	struct rpc_buffer *resp_buf = &req->response;

	req->service_status = update_agent_discover(this_instance->update_agent, &discovery_result);

	if (!req->service_status) {
		uint16_t num_func = 0;
		uint8_t function_presence[FWU_FUNC_ID_COUNT] = { 0 };

		num_func = generate_function_presence(this_instance->update_agent,
						      function_presence);

		rpc_status = fwu_serialize_discover_resp(
			resp_buf, discovery_result.service_status, discovery_result.version_major,
			discovery_result.version_minor, num_func, discovery_result.max_payload_size,
			discovery_result.flags, discovery_result.vendor_specific_flags,
			function_presence);
	} else {
		/*
		 * The actual service call failed, but the request was successful on the RPC level
		 */
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

static rpc_status_t begin_staging_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	struct fwu_provider *this_instance = (struct fwu_provider *)context;
	uint32_t vendor_flags = 0;
	uint32_t partial_update_count = 0;
	struct uuid_octets update_guid[FWU_PROVIDER_MAX_PARTIAL_UPDATE_COUNT];

	rpc_status = fwu_deserialize_begin_staging_req(
		req_buf, &vendor_flags, &partial_update_count,
		FWU_PROVIDER_MAX_PARTIAL_UPDATE_COUNT, update_guid);

	if (rpc_status == RPC_SUCCESS)
		req->service_status = update_agent_begin_staging(
			this_instance->update_agent, vendor_flags, partial_update_count,
			update_guid);

	return rpc_status;
}

static rpc_status_t end_staging_handler(void *context, struct rpc_request *req)
{
	struct fwu_provider *this_instance = (struct fwu_provider *)context;

	req->service_status = update_agent_end_staging(this_instance->update_agent);

	return RPC_SUCCESS;
}

static rpc_status_t cancel_staging_handler(void *context, struct rpc_request *req)
{
	struct fwu_provider *this_instance = (struct fwu_provider *)context;

	req->service_status = update_agent_cancel_staging(this_instance->update_agent);

	return RPC_SUCCESS;
}

static rpc_status_t open_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	struct fwu_provider *this_instance = (struct fwu_provider *)context;
	struct uuid_octets image_type_uuid = { 0 };
	uint8_t op_type = 0;

	rpc_status = fwu_deserialize_open_req(req_buf, &image_type_uuid, &op_type);

	if (rpc_status == RPC_SUCCESS) {
		uint32_t handle = 0;
		req->service_status =
			update_agent_open(this_instance->update_agent, &image_type_uuid, op_type,
					  &handle);

		if (!req->service_status) {
			struct rpc_buffer *resp_buf = &req->response;
			rpc_status = fwu_serialize_open_resp(resp_buf, handle);
		}
	}

	return rpc_status;
}

static rpc_status_t write_stream_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	struct fwu_provider *this_instance = (struct fwu_provider *)context;
	uint32_t handle = 0;
	size_t data_len = 0;
	const uint8_t *data = NULL;

	rpc_status = fwu_deserialize_write_stream_req(req_buf, &handle, &data_len, &data);

	if (rpc_status == RPC_SUCCESS) {
		req->service_status = update_agent_write_stream(this_instance->update_agent, handle,
								data, data_len);
	}

	return rpc_status;
}

static rpc_status_t read_stream_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	struct fwu_provider *this_instance = (struct fwu_provider *)context;
	uint32_t handle = 0;

	rpc_status = fwu_deserialize_read_stream_req(req_buf, &handle);

	if (rpc_status == RPC_SUCCESS) {
		struct rpc_buffer *resp_buf = &req->response;
		uint8_t *payload_buf;
		size_t max_payload;
		size_t read_len = 0;
		size_t total_len = 0;

		fwu_read_stream_resp_payload(resp_buf, &payload_buf, &max_payload);

		req->service_status = update_agent_read_stream(this_instance->update_agent, handle,
							 payload_buf, max_payload, &read_len,
							 &total_len);

		if (!req->service_status)
			rpc_status = fwu_serialize_read_stream_resp(resp_buf, read_len, total_len);

	}

	return rpc_status;
}

static rpc_status_t commit_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	struct fwu_provider *this_instance = (struct fwu_provider *)context;
	uint32_t handle = 0;
	bool accepted = false;
	size_t max_atomic_len = 0;

	rpc_status = fwu_deserialize_commit_req(req_buf, &handle, &accepted, &max_atomic_len);

	if (rpc_status == RPC_SUCCESS) {
		uint32_t progress = 0;
		uint32_t total_work = 0;

		req->service_status = update_agent_commit(this_instance->update_agent, handle,
							  accepted, 0, &progress, &total_work);

		if (!req->service_status) {
			struct rpc_buffer *resp_buf = &req->response;
			rpc_status = fwu_serialize_commit_resp(resp_buf, progress, total_work);
		}
	}

	return rpc_status;
}

static rpc_status_t accept_image_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	struct fwu_provider *this_instance = (struct fwu_provider *)context;
	struct uuid_octets image_type_uuid;

	rpc_status = fwu_deserialize_accept_req(req_buf, &image_type_uuid);

	if (rpc_status == RPC_SUCCESS)
		req->service_status = update_agent_accept_image(this_instance->update_agent,
								&image_type_uuid);

	return rpc_status;
}

static rpc_status_t select_previous_handler(void *context, struct rpc_request *req)
{
	struct fwu_provider *this_instance = (struct fwu_provider *)context;

	req->service_status = update_agent_select_previous(this_instance->update_agent);

	return RPC_SUCCESS;
}
