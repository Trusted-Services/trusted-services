/*
 * Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "remote_fwu_client.h"

#include <cassert>
#include <climits>
#include <cstring>

#include "protocols/rpc/common/packed-c/encoding.h"
#include "protocols/rpc/common/packed-c/status.h"
#include "protocols/service/fwu/fwu_proto.h"
#include "protocols/service/fwu/opcodes.h"
#include "protocols/service/fwu/status.h"
#include "util.h"

remote_fwu_client::remote_fwu_client()
	: fwu_client()
	, m_client()
	, m_rpc_session(NULL)
	, m_service_context(NULL)
{
	open_session();
}

remote_fwu_client::~remote_fwu_client()
{
	close_session();
}

void remote_fwu_client::open_session(void)
{
	m_rpc_session = NULL;
	m_service_context = NULL;

	service_locator_init();

	m_service_context = service_locator_query("sn:trustedfirmware.org:fwu:0");

	if (m_service_context) {
		m_rpc_session =
			service_context_open(m_service_context);

		if (m_rpc_session) {
			service_client_init(&m_client, m_rpc_session);

		} else {
			service_context_relinquish(m_service_context);
			m_service_context = NULL;
		}
	}
}

void remote_fwu_client::close_session(void)
{
	if (m_service_context) {
		service_client_deinit(&m_client);

		if (m_rpc_session) {
			service_context_close(m_service_context, m_rpc_session);
			m_rpc_session = NULL;
		}

		service_context_relinquish(m_service_context);
		m_service_context = NULL;
	}
}

rpc_call_handle remote_fwu_client::fwu_caller_session_begin(struct rpc_caller_session *session,
					uint8_t **request_buffer, size_t request_length,
					size_t response_max_length)
{
	struct fwu_request_header *req_header = NULL;
	rpc_call_handle handle = NULL;

	request_length += sizeof(*req_header);
	response_max_length += sizeof(struct fwu_response_header);

	handle = rpc_caller_session_begin(session, (uint8_t **)&req_header, request_length,
					  response_max_length);
	if (!handle)
		return handle;

	*request_buffer = req_header->payload;

	return new fwu_call_handle{req_header, handle};
}

rpc_status_t remote_fwu_client::fwu_caller_session_invoke(rpc_call_handle handle, uint32_t opcode,
				uint8_t **response_buffer, size_t *response_length,
				service_status_t *service_status)
{
	struct fwu_response_header *resp_header = NULL;
	fwu_call_handle *fwu_handle = (fwu_call_handle *)handle;
	rpc_status_t status = RPC_ERROR_INTERNAL;

	fwu_handle->req_header->func_id = opcode;

	status = rpc_caller_session_invoke(fwu_handle->handle, 0, (uint8_t **)&resp_header,
					   response_length, service_status);
	if (status != RPC_SUCCESS)
		return status;

	*service_status = resp_header->status;
	*response_buffer = resp_header->payload;

	return status;
}

rpc_status_t remote_fwu_client::fwu_caller_session_end(rpc_call_handle handle)
{
	fwu_call_handle *fwu_handle = (fwu_call_handle *)handle;
	rpc_status_t status = RPC_ERROR_INTERNAL;

	status = rpc_caller_session_end(fwu_handle->handle);
	delete fwu_handle;

	return status;
}

int remote_fwu_client::invoke_no_param(unsigned int opcode)
{
	int fwu_status = FWU_STATUS_NOT_AVAILABLE;
	size_t req_len = 0;
	rpc_call_handle call_handle;
	uint8_t *req_buf;

	if (!m_service_context)
		return fwu_status;

	call_handle = fwu_caller_session_begin(m_rpc_session, &req_buf, req_len, 0);

	if (call_handle) {
		uint8_t *resp_buf;
		size_t resp_len;
		service_status_t service_status;

		m_client.rpc_status = fwu_caller_session_invoke(call_handle, opcode,
							&resp_buf, &resp_len, &service_status);

		if (m_client.rpc_status == TS_RPC_CALL_ACCEPTED)
			fwu_status = service_status;

		fwu_caller_session_end(call_handle);
	}

	return fwu_status;
}

int remote_fwu_client::discover(int16_t *service_status, uint8_t *version_major,
				uint8_t *version_minor, uint16_t *num_func,
				uint64_t *max_payload_size, uint32_t *flags,
				uint32_t *vendor_specific_flags, uint8_t *function_presence)
{
	int fwu_status = FWU_STATUS_NOT_AVAILABLE;
	struct fwu_discover_out *resp_msg = NULL;
	size_t max_resp_len = 0;

	if (!m_service_context)
		return fwu_status;

	if (ADD_OVERFLOW(FWU_FUNC_ID_COUNT, sizeof(*resp_msg), &max_resp_len))
		return FWU_STATUS_OUT_OF_BOUNDS;

	rpc_call_handle call_handle = NULL;
	uint8_t *req_buf = NULL;

	call_handle = fwu_caller_session_begin(m_rpc_session, &req_buf, 0, max_resp_len);

	if (call_handle) {
		uint8_t *resp_buf = NULL;
		size_t resp_len = 0;
		service_status_t req_service_status = 0;

		m_client.rpc_status = fwu_caller_session_invoke(call_handle, FWU_FUNC_ID_DISCOVER,
								&resp_buf, &resp_len,
								&req_service_status);

		if (m_client.rpc_status == TS_RPC_CALL_ACCEPTED) {
			fwu_status = req_service_status;

			if (fwu_status == FWU_STATUS_SUCCESS) {
				if (resp_len <= max_resp_len) {
					resp_msg = (struct fwu_discover_out *)resp_buf;

					*service_status = resp_msg->service_status;
					*version_major = resp_msg->version_major;
					*num_func = resp_msg->num_func;
					*max_payload_size = resp_msg->max_payload_size;
					*flags = resp_msg->flags;
					*vendor_specific_flags = resp_msg->vendor_specific_flags;
					memcpy(function_presence, resp_msg->function_presence,
					resp_msg->num_func);
				} else {
					fwu_status = FWU_STATUS_OUT_OF_BOUNDS;
				}
			}
		}

		fwu_caller_session_end(call_handle);
	}

	return fwu_status;
}

int remote_fwu_client::begin_staging(uint32_t vendor_flags, uint32_t partial_update_count,
				     struct uuid_octets update_guid[])
{
	int fwu_status = FWU_STATUS_NOT_AVAILABLE;
	struct fwu_begin_staging_in *req_msg = NULL;
	size_t req_len = 0;
	size_t uuids_size = 0;

	if (!m_service_context)
		return fwu_status;

	if (MUL_OVERFLOW(partial_update_count, sizeof(*update_guid), &uuids_size) ||
	    ADD_OVERFLOW(uuids_size, sizeof(*req_msg), &req_len))
		return fwu_status;

	rpc_call_handle call_handle = NULL;
	uint8_t *req_buf = NULL;

	call_handle = fwu_caller_session_begin(m_rpc_session, &req_buf, req_len, 0);

	if (call_handle) {
		uint8_t *resp_buf = NULL;
		size_t resp_len = 0;
		service_status_t service_status = 0;

		req_msg = (struct fwu_begin_staging_in *)req_buf;
		req_msg->reserved = 0;
		req_msg->vendor_flags = vendor_flags;
		req_msg->partial_update_count = partial_update_count;
		memcpy(req_msg->update_guid, update_guid, uuids_size);

		m_client.rpc_status = fwu_caller_session_invoke(call_handle,
							FWU_FUNC_ID_BEGIN_STAGING,
							&resp_buf, &resp_len, &service_status);

		if (m_client.rpc_status == TS_RPC_CALL_ACCEPTED)
			fwu_status = service_status;

		fwu_caller_session_end(call_handle);
	}

	return fwu_status;
}

int remote_fwu_client::end_staging(void)
{
	return invoke_no_param(FWU_FUNC_ID_END_STAGING);
}

int remote_fwu_client::cancel_staging(void)
{
	return invoke_no_param(FWU_FUNC_ID_CANCEL_STAGING);
}

int remote_fwu_client::accept(const struct uuid_octets *image_type_uuid)
{
	int fwu_status = FWU_STATUS_NOT_AVAILABLE;
	struct fwu_accept_image_in req_msg = { 0 };
	size_t req_len = sizeof(struct fwu_accept_image_in);

	if (!m_service_context)
		return fwu_status;

	memcpy(req_msg.image_type_uuid, image_type_uuid->octets, OSF_UUID_OCTET_LEN);

	rpc_call_handle call_handle;
	uint8_t *req_buf;

	call_handle = fwu_caller_session_begin(m_rpc_session, &req_buf, req_len, 0);

	if (call_handle) {
		uint8_t *resp_buf;
		size_t resp_len;
		service_status_t service_status;

		memcpy(req_buf, &req_msg, req_len);

		m_client.rpc_status = fwu_caller_session_invoke(call_handle,
							FWU_FUNC_ID_ACCEPT_IMAGE,
							&resp_buf, &resp_len, &service_status);

		if (m_client.rpc_status == TS_RPC_CALL_ACCEPTED)
			fwu_status = service_status;

		fwu_caller_session_end(call_handle);
	}

	return fwu_status;
}

int remote_fwu_client::select_previous(void)
{
	return invoke_no_param(FWU_FUNC_ID_SELECT_PREVIOUS);
}

int remote_fwu_client::open(const struct uuid_octets *uuid, op_type op_type, uint32_t *handle)
{
	int fwu_status = FWU_STATUS_NOT_AVAILABLE;
	struct fwu_open_in req_msg = { 0 };
	size_t req_len = sizeof(struct fwu_open_in);

	if (!m_service_context)
		return fwu_status;

	memcpy(req_msg.image_type_uuid, uuid->octets, OSF_UUID_OCTET_LEN);
	req_msg.op_type = static_cast<uint8_t>(op_type);

	rpc_call_handle call_handle;
	uint8_t *req_buf;

	call_handle = fwu_caller_session_begin(m_rpc_session, &req_buf, req_len,
					       sizeof(struct fwu_open_out));

	if (call_handle) {
		uint8_t *resp_buf;
		size_t resp_len;
		service_status_t service_status;

		memcpy(req_buf, &req_msg, req_len);

		m_client.rpc_status = fwu_caller_session_invoke(call_handle,
							FWU_FUNC_ID_OPEN, &resp_buf,
							&resp_len, &service_status);

		if (m_client.rpc_status == TS_RPC_CALL_ACCEPTED) {
			fwu_status = service_status;

			if ((fwu_status == FWU_STATUS_SUCCESS) &&
			    (resp_len >= sizeof(struct fwu_open_out))) {
				struct fwu_open_out resp_msg;

				memcpy(&resp_msg, resp_buf, sizeof(struct fwu_open_out));
				*handle = resp_msg.handle;
			}
		}

		fwu_caller_session_end(call_handle);
	}

	return fwu_status;
}

int remote_fwu_client::commit(uint32_t handle, bool accepted)
{
	int fwu_status = FWU_STATUS_NOT_AVAILABLE;
	struct fwu_commit_in req_msg = { 0 };
	size_t req_len = sizeof(struct fwu_commit_in);

	if (!m_service_context)
		return fwu_status;

	req_msg.handle = handle;
	req_msg.acceptance_req = (accepted) ? 0 : 1;

	rpc_call_handle call_handle;
	uint8_t *req_buf;

	call_handle = fwu_caller_session_begin(m_rpc_session, &req_buf, req_len, 0);

	if (call_handle) {
		uint8_t *resp_buf;
		size_t resp_len;
		service_status_t service_status;

		memcpy(req_buf, &req_msg, req_len);

		m_client.rpc_status = fwu_caller_session_invoke(call_handle,
							FWU_FUNC_ID_COMMIT, &resp_buf,
							&resp_len, &service_status);

		if (m_client.rpc_status == TS_RPC_CALL_ACCEPTED)
			fwu_status = service_status;

		fwu_caller_session_end(call_handle);
	}

	return fwu_status;
}

int remote_fwu_client::write_stream(uint32_t handle, const uint8_t *data, size_t data_len)
{
	size_t proto_overhead = offsetof(fwu_write_stream_in, payload);
	size_t max_payload = (m_client.service_info.max_payload > proto_overhead) ?
				     m_client.service_info.max_payload - proto_overhead :
				     0;

	if (!data || (data_len > (SIZE_MAX - proto_overhead)))
		return FWU_STATUS_OUT_OF_BOUNDS;

	if (!m_service_context)
		return FWU_STATUS_NOT_AVAILABLE;

	if (!max_payload)
		return FWU_STATUS_NOT_AVAILABLE;

	size_t total_written = 0;
	struct fwu_write_stream_in req_msg;

	req_msg.handle = handle;

	while (total_written < data_len) {
		size_t bytes_remaining = data_len - total_written;
		size_t write_len = (bytes_remaining < max_payload) ? bytes_remaining : max_payload;
		size_t req_len = proto_overhead + write_len;

		req_msg.data_len = write_len;

		rpc_call_handle call_handle;
		uint8_t *req_buf;

		call_handle = fwu_caller_session_begin(m_rpc_session, &req_buf, req_len, 0);

		if (call_handle) {
			uint8_t *resp_buf;
			size_t resp_len;
			service_status_t service_status;

			memcpy(req_buf, &req_msg, proto_overhead);
			memcpy(&req_buf[proto_overhead], &data[total_written], write_len);

			total_written += write_len;

			m_client.rpc_status = fwu_caller_session_invoke(call_handle,
								FWU_FUNC_ID_WRITE_STREAM,
								&resp_buf, &resp_len, &service_status);

			fwu_caller_session_end(call_handle);

			if (m_client.rpc_status != TS_RPC_CALL_ACCEPTED)
				return FWU_STATUS_NOT_AVAILABLE;

			if (service_status != FWU_STATUS_SUCCESS)
				return (int)service_status;

		} else
			return FWU_STATUS_NOT_AVAILABLE;
	}

	return FWU_STATUS_SUCCESS;
}

int remote_fwu_client::read_stream(uint32_t handle, uint8_t *buf, size_t buf_size, size_t *read_len,
				   size_t *total_len)
{
	int fwu_status = FWU_STATUS_NOT_AVAILABLE;
	struct fwu_read_stream_in req_msg;
	size_t req_len = sizeof(struct fwu_read_stream_in);

	if (!m_service_context)
		return fwu_status;

	req_msg.handle = handle;

	*read_len = 0;
	*total_len = 0;

	rpc_call_handle call_handle;
	uint8_t *req_buf;

	call_handle = fwu_caller_session_begin(m_rpc_session, &req_buf, req_len,
					       sizeof(struct fwu_read_stream_out) + buf_size);

	if (call_handle) {
		uint8_t *resp_buf;
		size_t resp_len;
		service_status_t service_status;

		memcpy(req_buf, &req_msg, req_len);

		m_client.rpc_status = fwu_caller_session_invoke(call_handle,
							FWU_FUNC_ID_READ_STREAM,
							&resp_buf, &resp_len, &service_status);

		size_t proto_overhead = offsetof(fwu_read_stream_out, payload);

		if (m_client.rpc_status == TS_RPC_CALL_ACCEPTED) {
			fwu_status = service_status;

			if ((fwu_status == FWU_STATUS_SUCCESS) && (resp_len >= proto_overhead)) {
				const struct fwu_read_stream_out *resp_msg =
					(const struct fwu_read_stream_out *)resp_buf;

				*read_len = resp_msg->read_bytes;
				*total_len = resp_msg->total_bytes;

				if (buf && buf_size) {
					size_t copy_len = (resp_msg->read_bytes <= buf_size) ?
								  resp_msg->read_bytes :
								  buf_size;

					memcpy(buf, resp_msg->payload, copy_len);
				}
			}
		}

		fwu_caller_session_end(call_handle);
	}

	return fwu_status;
}
