/*
 * Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FWU_CLIENT_H
#define FWU_CLIENT_H

#include <stddef.h>
#include <stdint.h>

#include "common/uuid/uuid.h"

/*
 * Presents a client interface for interacting with a fwu service provider.
 * Test cases that use this interface can potentially be reused with alternative
 * service provider deployments.
 */
class fwu_client {
public:
	enum class op_type {
		READ = 0,
		WRITE = 1,
	};

	fwu_client()
	{
	}

	virtual ~fwu_client()
	{
	}

	virtual int discover(int16_t *service_status, uint8_t *version_major,
			     uint8_t *version_minor, uint16_t *num_func, uint64_t *max_payload_size,
			     uint32_t *flags, uint32_t *vendor_specific_flags,
			     uint8_t *function_presence) = 0;

	virtual int begin_staging(uint32_t vendor_flags, uint32_t partial_update_count,
				  struct uuid_octets update_guid[]) = 0;

	virtual int end_staging(void) = 0;

	virtual int cancel_staging(void) = 0;

	virtual int accept(const struct uuid_octets *image_type_uuid) = 0;

	virtual int select_previous(void) = 0;

	virtual int open(const struct uuid_octets *uuid, op_type op_type, uint32_t *handle) = 0;

	virtual int commit(uint32_t handle, bool accepted) = 0;

	virtual int write_stream(uint32_t handle, const uint8_t *data, size_t data_len) = 0;

	virtual int read_stream(uint32_t handle, uint8_t *buf, size_t buf_size, size_t *read_len,
				size_t *total_len) = 0;
};

#endif /* FWU_CLIENT_H */
