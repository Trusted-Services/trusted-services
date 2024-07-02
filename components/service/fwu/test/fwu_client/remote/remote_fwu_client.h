/*
 * Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef REMOTE_FWU_CLIENT_H
#define REMOTE_FWU_CLIENT_H

#include "service/common/client/service_client.h"
#include "service/fwu/test/fwu_client/fwu_client.h"
#include "service_locator.h"

/*
 * An fwu_client that calls a remote fwu service provider via a
 * deployment specific RPC caller. Assumes that call request and
 * response parameters are serialized in-line with the FWU-A specification.
 */
class remote_fwu_client : public fwu_client {
public:
	remote_fwu_client();
	~remote_fwu_client();

	int discover(int16_t *service_status, uint8_t *version_major, uint8_t *version_minor,
		     uint16_t *num_func, uint64_t *max_payload_size, uint32_t *flags,
		     uint32_t *vendor_specific_flags, uint8_t *function_presence);

	int begin_staging(uint32_t vendor_flags, uint32_t partial_update_count,
			  struct uuid_octets update_guid[]);

	int end_staging(void);

	int cancel_staging(void);

	int accept(const struct uuid_octets *image_type_uuid);

	int select_previous(void);

	int open(const struct uuid_octets *uuid, op_type op_type, uint32_t *handle);

	int commit(uint32_t handle, bool accepted);

	int write_stream(uint32_t handle, const uint8_t *data, size_t data_len);

	int read_stream(uint32_t handle, uint8_t *buf, size_t buf_size, size_t *read_len,
			size_t *total_len);

private:
	int invoke_no_param(unsigned int opcode);
	void open_session(void);
	void close_session(void);

	struct service_client m_client;
	struct rpc_caller_session *m_rpc_session;
	struct service_context *m_service_context;
};

#endif /* REMOTE_FWU_CLIENT_H */
