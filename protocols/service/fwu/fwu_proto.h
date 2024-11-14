/*
 * Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FWU_PROTO_H
#define FWU_PROTO_H

#include <stdint.h>

#include "opcodes.h"
#include "protocols/common/osf/uuid.h"
#include "status.h"

/**
 * The major version number of the FWU-A access protocol. It will be incremented
 * on significant updates that may include breaking changes.
 */
#define FWU_PROTOCOL_VERSION_MAJOR 1

/**
 * The minor version number  It will be incremented in
 * small updates that are unlikely to include breaking changes.
 */
#define FWU_PROTOCOL_VERSION_MINOR 0

/**
 * Protocol GUIDs defined in FWU-A specification
 */
#define FWU_UPDATE_AGENT_CANONICAL_UUID "6823a838-1b06-470e-9774-0cce8bfb53fd"
#define FWU_DIRECTORY_CANONICAL_UUID	"deee58d9-5147-4ad3-a290-77666e2341a5"
#define FWU_METADATA_CANONICAL_UUID	"8a7a84a0-8387-40f6-ab41-a8b9a5a60d23"

#define FWU_OPEN_OP_TYPE_READ  (0)
#define FWU_OPEN_OP_TYPE_WRITE (1)

/**
 * Image directory
 */
#define FWU_IMAGE_DIRECTORY_VERSION	(2)

struct __attribute__((__packed__)) fwu_image_info_entry {
	uint8_t img_type_uuid[OSF_UUID_OCTET_LEN];
	uint32_t client_permissions;
	uint32_t img_max_size;
	uint32_t lowest_accepted_version;
	uint32_t img_version;
	uint32_t accepted;
	uint32_t reserved;
};

struct __attribute__((__packed__)) fwu_image_directory {
	uint32_t directory_version;
	uint32_t img_info_offset;
	uint32_t num_images;
	uint32_t correct_boot;
	uint32_t img_info_size;
	uint32_t reserved;
	struct fwu_image_info_entry img_info_entry[];
};

/**
 * Message parameters
 */

struct __attribute__((__packed__)) fwu_request_header {
	uint32_t func_id;
	uint8_t payload[];
};

struct __attribute__((__packed__)) fwu_response_header {
	uint32_t status;
	uint8_t payload[];
};

struct __attribute__((__packed__)) fwu_discover_out {
	int16_t service_status;
	uint8_t version_major;
	uint8_t version_minor;
	uint16_t off_function_presence;
	uint16_t num_func;
	uint64_t max_payload_size;
	uint32_t flags;
	uint32_t vendor_specific_flags;
	uint8_t function_presence[];
};

struct __attribute__((__packed__)) fwu_begin_staging_in {
	uint32_t reserved;
	uint32_t vendor_flags;
	uint32_t partial_update_count;
	uint8_t update_guid[];
};

struct __attribute__((__packed__)) fwu_open_in {
	uint8_t image_type_uuid[OSF_UUID_OCTET_LEN];
	uint8_t op_type;
};

struct __attribute__((__packed__)) fwu_open_out {
	uint32_t handle;
};

struct __attribute__((__packed__)) fwu_write_stream_in {
	uint32_t handle;
	uint32_t data_len;
	uint8_t payload[];
};

struct __attribute__((__packed__)) fwu_read_stream_in {
	uint32_t handle;
};

struct __attribute__((__packed__)) fwu_read_stream_out {
	uint32_t read_bytes;
	uint32_t total_bytes;
	uint8_t payload[];
};

struct __attribute__((__packed__)) fwu_commit_in {
	uint32_t handle;
	uint32_t acceptance_req;
	uint32_t max_atomic_len;
};

struct __attribute__((__packed__)) fwu_commit_out {
	uint32_t progress;
	uint32_t total_work;
};

struct __attribute__((__packed__)) fwu_accept_image_in {
	uint32_t reserved;
	uint8_t image_type_uuid[OSF_UUID_OCTET_LEN];
};

#endif /* FWU_PROTO_H */
