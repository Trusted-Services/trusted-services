/*
 * Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "fwu_provider_serializer.h"

#include <string.h>

#include "protocols/rpc/common/packed-c/status.h"
#include "protocols/service/fwu/fwu_proto.h"
#include "util.h"

rpc_status_t fwu_serialize_discover_resp(const struct rpc_buffer *resp_buf, int16_t service_status,
					 uint8_t version_major, uint8_t version_minor,
					 uint16_t num_func, uint64_t max_payload_size,
					 uint32_t flags, uint32_t vendor_specific_flags,
					 uint8_t *function_presence)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_RESPONSE_BODY;
	struct fwu_discover_out *resp_msg = NULL;
	size_t len = 0;

	if (ADD_OVERFLOW(sizeof(*resp_msg), num_func, &len))
		return RPC_ERROR_INVALID_RESPONSE_BODY;

	if (len <= resp_buf->size) {
		resp_msg = (struct fwu_discover_out *)resp_buf->data;

		resp_msg->service_status = service_status;
		resp_msg->version_major = version_major;
		resp_msg->version_minor = version_minor;
		resp_msg->off_function_presence =
			offsetof(struct fwu_discover_out, function_presence);
		resp_msg->num_func = num_func;
		resp_msg->max_payload_size = max_payload_size;
		resp_msg->flags = flags;
		resp_msg->vendor_specific_flags = vendor_specific_flags;
		memcpy(resp_msg->function_presence, function_presence, num_func);

		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

rpc_status_t fwu_deserialize_begin_staging_req(const struct rpc_buffer *req_buf,
					       uint32_t *vendor_flags,
					       uint32_t *partial_update_count,
					       uint32_t max_update_count,
					       struct uuid_octets *update_guid)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;
	size_t expected_fixed_len = sizeof(struct fwu_begin_staging_in);

	if (expected_fixed_len <= req_buf->data_length) {
		const struct fwu_begin_staging_in *recv_msg =
			(const struct fwu_begin_staging_in *)req_buf->data;
		size_t full_len = 0;

		if (ADD_OVERFLOW(expected_fixed_len, recv_msg->partial_update_count, &full_len))
			return RPC_ERROR_INVALID_REQUEST_BODY;

		if (recv_msg->partial_update_count > max_update_count)
			return RPC_ERROR_INTERNAL;

		*vendor_flags = recv_msg->vendor_flags;
		*partial_update_count = recv_msg->partial_update_count;

		memcpy(update_guid, recv_msg->update_guid,
		       UUID_OCTETS_LEN * recv_msg->partial_update_count);

		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

rpc_status_t fwu_deserialize_open_req(const struct rpc_buffer *req_buf,
				      struct uuid_octets *image_type_uuid,
				      uint8_t *op_type)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;
	size_t expected_fixed_len = sizeof(struct fwu_open_in);

	if (expected_fixed_len <= req_buf->data_length) {
		const struct fwu_open_in *recv_msg =
			(const struct fwu_open_in *)req_buf->data;

		if (recv_msg->op_type == FWU_OPEN_OP_TYPE_READ ||
		    recv_msg->op_type == FWU_OPEN_OP_TYPE_WRITE) {
			memcpy(image_type_uuid->octets, recv_msg->image_type_uuid, UUID_OCTETS_LEN);
			*op_type = recv_msg->op_type;
			rpc_status = RPC_SUCCESS;
		} else {
			rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;
		}
	}

	return rpc_status;
}

rpc_status_t fwu_serialize_open_resp(struct rpc_buffer *resp_buf, uint32_t handle)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	size_t fixed_len = sizeof(struct fwu_open_out);

	if (fixed_len <= resp_buf->size) {
		struct fwu_open_out *resp_msg = (struct fwu_open_out *)resp_buf->data;

		resp_msg->handle = handle;

		resp_buf->data_length = fixed_len;
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

/* Operation: write_stream */
rpc_status_t fwu_deserialize_write_stream_req(const struct rpc_buffer *req_buf, uint32_t *handle,
					      size_t *data_length, const uint8_t **data)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;
	size_t expected_fixed_len = sizeof(struct fwu_write_stream_in);

	if (expected_fixed_len <= req_buf->data_length) {
		const struct fwu_write_stream_in *recv_msg =
			(const struct fwu_write_stream_in *)req_buf->data;

		*handle = recv_msg->handle;
		*data_length = recv_msg->data_len;
		*data = recv_msg->payload;
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

/* Operation: read_stream */
rpc_status_t fwu_deserialize_read_stream_req(const struct rpc_buffer *req_buf, uint32_t *handle)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;
	size_t expected_fixed_len = sizeof(struct fwu_read_stream_in);

	if (expected_fixed_len <= req_buf->data_length) {
		const struct fwu_read_stream_in *recv_msg =
			(const struct fwu_read_stream_in *)req_buf->data;

		*handle = recv_msg->handle;
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

void fwu_read_stream_resp_payload(const struct rpc_buffer *resp_buf, uint8_t **payload_buf,
				  size_t *max_payload)
{
	struct fwu_read_stream_out *resp_msg = (struct fwu_read_stream_out *)resp_buf->data;
	size_t fixed_len = offsetof(struct fwu_read_stream_out, payload);

	*max_payload = 0;
	*payload_buf = resp_msg->payload;

	if (fixed_len < resp_buf->size)
		*max_payload = resp_buf->size - fixed_len;
}

rpc_status_t fwu_serialize_read_stream_resp(struct rpc_buffer *resp_buf, size_t read_bytes,
					    size_t total_bytes)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct fwu_read_stream_out *resp_msg = (struct fwu_read_stream_out *)resp_buf->data;
	size_t proto_overhead = offsetof(struct fwu_read_stream_out, payload);

	if (read_bytes > (SIZE_MAX - proto_overhead))
		return RPC_ERROR_INVALID_VALUE;

	size_t required_len = proto_overhead + read_bytes;

	if (required_len <= resp_buf->size) {
		resp_msg->read_bytes = read_bytes;
		resp_msg->total_bytes = total_bytes;

		resp_buf->data_length = required_len;
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

/* Operation: commit */
rpc_status_t fwu_deserialize_commit_req(const struct rpc_buffer *req_buf, uint32_t *handle,
					bool *accepted, size_t *max_atomic_len)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;
	size_t expected_fixed_len = sizeof(struct fwu_commit_in);

	if (expected_fixed_len <= req_buf->data_length) {
		const struct fwu_commit_in *recv_msg =
			(const struct fwu_commit_in *)req_buf->data;

		*handle = recv_msg->handle;
		*accepted = (recv_msg->acceptance_req == 0);
		*max_atomic_len = recv_msg->max_atomic_len;
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

rpc_status_t fwu_serialize_commit_resp(struct rpc_buffer *resp_buf, size_t progress,
				       size_t total_work)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct fwu_commit_out *resp_msg = (struct fwu_commit_out *)resp_buf->data;

	size_t required_len = sizeof(struct fwu_commit_out);

	if (required_len <= resp_buf->size) {
		resp_msg->progress = progress;
		resp_msg->total_work = total_work;

		resp_buf->data_length = required_len;
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

/* Operation: accept_image */
rpc_status_t fwu_deserialize_accept_req(const struct rpc_buffer *req_buf,
					struct uuid_octets *image_type_uuid)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;
	size_t expected_fixed_len = sizeof(struct fwu_accept_image_in);

	if (expected_fixed_len <= req_buf->data_length) {
		const struct fwu_accept_image_in *recv_msg =
			(const struct fwu_accept_image_in *)req_buf->data;

		memcpy(image_type_uuid->octets, recv_msg->image_type_uuid, UUID_OCTETS_LEN);
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}
