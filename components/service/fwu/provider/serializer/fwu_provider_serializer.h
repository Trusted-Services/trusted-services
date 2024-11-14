/*
 * Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FWU_PROVIDER_SERIALIZER_H
#define FWU_PROVIDER_SERIALIZER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "common/uuid/uuid.h"
#include "rpc/common/endpoint/rpc_service_interface.h"

/* Operation: discover */
rpc_status_t fwu_serialize_discover_resp(const struct rpc_buffer *resp_buf, int16_t service_status,
					 uint8_t version_major, uint8_t version_minor,
					 uint16_t num_func, uint64_t max_payload_size,
					 uint32_t flags, uint32_t vendor_specific_flags,
					 uint8_t *function_presence);

/* Operation: begin staging */
rpc_status_t fwu_deserialize_begin_staging_req(const struct rpc_buffer *req_buf,
					       uint32_t *vendor_flags,
					       uint32_t *partial_update_count,
					       uint32_t max_update_count,
					       struct uuid_octets *update_guid);

/* Operation: open */
rpc_status_t fwu_deserialize_open_req(const struct rpc_buffer *req_buf,
				      struct uuid_octets *image_type_uuid,
				      uint8_t *op_type);

rpc_status_t fwu_serialize_open_resp(struct rpc_buffer *resp_buf, uint32_t handle);

/* Operation: write_stream */
rpc_status_t fwu_deserialize_write_stream_req(const struct rpc_buffer *req_buf, uint32_t *handle,
					      size_t *data_length, const uint8_t **data);

/* Operation: read_stream */
rpc_status_t fwu_deserialize_read_stream_req(const struct rpc_buffer *req_buf, uint32_t *handle);

void fwu_read_stream_resp_payload(const struct rpc_buffer *resp_buf, uint8_t **payload_buf,
				  size_t *max_payload);

rpc_status_t fwu_serialize_read_stream_resp(struct rpc_buffer *resp_buf, size_t read_bytes,
					    size_t total_bytes);

/* Operation: commit */
rpc_status_t fwu_deserialize_commit_req(const struct rpc_buffer *req_buf, uint32_t *handle,
					bool *accepted, size_t *max_atomic_len);

rpc_status_t fwu_serialize_commit_resp(struct rpc_buffer *resp_buf, size_t progress,
				       size_t total_work);

/* Operation: accept_image */
rpc_status_t fwu_deserialize_accept_req(const struct rpc_buffer *req_buf,
					struct uuid_octets *image_type_uuid);

#endif /* FWU_PROVIDER_SERIALIZER_H */
