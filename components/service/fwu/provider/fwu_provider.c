/*
 * Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fwu_provider.h"

#include <stddef.h>

#include "common/uuid/uuid.h"
#include "protocols/rpc/common/packed-c/status.h"
#include "protocols/service/fwu/packed-c/opcodes.h"
#include "service/fwu/common/update_agent_interface.h"
#include "service/fwu/provider/serializer/fwu_provider_serializer.h"
#include "fwu_uuid.h"

/* Service request handlers */
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
	{ TS_FWU_OPCODE_BEGIN_STAGING, begin_staging_handler },
	{ TS_FWU_OPCODE_END_STAGING, end_staging_handler },
	{ TS_FWU_OPCODE_CANCEL_STAGING, cancel_staging_handler },
	{ TS_FWU_OPCODE_OPEN, open_handler },
	{ TS_FWU_OPCODE_WRITE_STREAM, write_stream_handler },
	{ TS_FWU_OPCODE_READ_STREAM, read_stream_handler },
	{ TS_FWU_OPCODE_COMMIT, commit_handler },
	{ TS_FWU_OPCODE_ACCEPT_IMAGE, accept_image_handler },
	{ TS_FWU_OPCODE_SELECT_PREVIOUS, select_previous_handler }
};

struct rpc_service_interface *fwu_provider_init(struct fwu_provider *context,
					struct update_agent *update_agent)
{
	const struct rpc_uuid service_uuid = { .uuid = TS_FWU_SERVICE_UUID };
	/* Initialise the fwu_provider */
	context->update_agent = update_agent;

	for (size_t encoding = 0; encoding < TS_RPC_ENCODING_LIMIT; ++encoding)
		context->serializers[encoding] = NULL;

	service_provider_init(&context->base_provider, context, &service_uuid, handler_table,
			      sizeof(handler_table) / sizeof(struct service_handler));

	return service_provider_get_rpc_interface(&context->base_provider);
}

void fwu_provider_deinit(struct fwu_provider *context)
{
	(void)context;
}

void fwu_provider_register_serializer(struct fwu_provider *context, unsigned int encoding,
				      const struct fwu_provider_serializer *serializer)
{
	if (encoding < TS_RPC_ENCODING_LIMIT) {
		context->serializers[encoding] = serializer;
	}
}

static const struct fwu_provider_serializer *get_fwu_serializer(struct fwu_provider *this_instance,
								const struct rpc_request *req)
{
	const struct fwu_provider_serializer *serializer = NULL;
	unsigned int encoding = 0;

	if (encoding < TS_RPC_ENCODING_LIMIT)
		serializer = this_instance->serializers[encoding];

	return serializer;
}

static rpc_status_t begin_staging_handler(void *context, struct rpc_request *req)
{
	struct fwu_provider *this_instance = (struct fwu_provider *)context;

	req->service_status = update_agent_begin_staging(this_instance->update_agent, 0, 0, NULL);

	return RPC_SUCCESS;
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
	const struct fwu_provider_serializer *serializer = get_fwu_serializer(this_instance, req);
	struct uuid_octets image_type_uuid;

	if (serializer)
		rpc_status = serializer->deserialize_open_req(req_buf, &image_type_uuid);

	if (rpc_status == RPC_SUCCESS) {
		uint32_t handle = 0;
		req->service_status =
			update_agent_open(this_instance->update_agent, &image_type_uuid, 0,
					  &handle);

		if (!req->service_status) {
			struct rpc_buffer *resp_buf = &req->response;
			rpc_status = serializer->serialize_open_resp(resp_buf, handle);
		}
	}

	return rpc_status;
}

static rpc_status_t write_stream_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	struct fwu_provider *this_instance = (struct fwu_provider *)context;
	const struct fwu_provider_serializer *serializer = get_fwu_serializer(this_instance, req);
	uint32_t handle = 0;
	size_t data_len = 0;
	const uint8_t *data = NULL;

	if (serializer)
		rpc_status = serializer->deserialize_write_stream_req(req_buf, &handle, &data_len,
								      &data);

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
	const struct fwu_provider_serializer *serializer = get_fwu_serializer(this_instance, req);
	uint32_t handle = 0;

	if (serializer)
		rpc_status = serializer->deserialize_read_stream_req(req_buf, &handle);

	if (rpc_status == RPC_SUCCESS) {
		struct rpc_buffer *resp_buf = &req->response;
		uint8_t *payload_buf;
		size_t max_payload;
		size_t read_len = 0;
		size_t total_len = 0;

		serializer->read_stream_resp_payload(resp_buf, &payload_buf, &max_payload);

		req->service_status = update_agent_read_stream(this_instance->update_agent, handle,
							 payload_buf, max_payload, &read_len,
							 &total_len);

		if (!req->service_status)
			rpc_status = serializer->serialize_read_stream_resp(resp_buf, read_len,
									    total_len);

	}

	return rpc_status;
}

static rpc_status_t commit_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	struct fwu_provider *this_instance = (struct fwu_provider *)context;
	const struct fwu_provider_serializer *serializer = get_fwu_serializer(this_instance, req);
	uint32_t handle = 0;
	bool accepted = false;
	size_t max_atomic_len = 0;

	if (serializer)
		rpc_status = serializer->deserialize_commit_req(req_buf, &handle, &accepted,
								&max_atomic_len);

	if (rpc_status == RPC_SUCCESS) {
		uint32_t progress = 0;
		uint32_t total_work = 0;

		req->service_status = update_agent_commit(this_instance->update_agent, handle,
							  accepted, 0, &progress, &total_work);

		if (!req->service_status) {
			struct rpc_buffer *resp_buf = &req->response;
			rpc_status = serializer->serialize_commit_resp(resp_buf, progress, total_work);
		}
	}

	return rpc_status;
}

static rpc_status_t accept_image_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	struct fwu_provider *this_instance = (struct fwu_provider *)context;
	const struct fwu_provider_serializer *serializer = get_fwu_serializer(this_instance, req);
	struct uuid_octets image_type_uuid;

	if (serializer)
		rpc_status = serializer->deserialize_accept_req(req_buf, &image_type_uuid);

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
