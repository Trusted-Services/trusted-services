/*
 * Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "direct_fwu_client.h"

#include <cstring>

#include "service/fwu/common/update_agent_interface.h"
#include "protocols/service/fwu/fwu_proto.h"

direct_fwu_client::direct_fwu_client(struct update_agent **update_agent)
	: fwu_client()
	, m_update_agent(update_agent)
	, m_read_buf()
{
	/* The read buffer represents a communication buffer that will
	 * constrain the amount of data that may be read in a single read.
	 */
	memset(m_read_buf, 0, READ_BUF_SIZE);
}

direct_fwu_client::~direct_fwu_client()
{
}

int direct_fwu_client::discover(int16_t *service_status, uint8_t *version_major,
				uint8_t *version_minor, uint16_t *num_func,
				uint64_t *max_payload_size, uint32_t *flags,
				uint32_t *vendor_specific_flags, uint8_t *function_presence)
{
	struct fwu_discovery_result discovery_result = { 0 };
	int res = 0;

	res = update_agent_discover(*m_update_agent, &discovery_result);
	if (res)
		return res;

	*service_status = discovery_result.service_status;
	*version_major = discovery_result.version_major;
	*version_minor = discovery_result.version_minor;
	*max_payload_size = discovery_result.max_payload_size;
	*flags = discovery_result.flags;
	*vendor_specific_flags = discovery_result.vendor_specific_flags;

	*num_func = 0;

#define ADD_FUNC_IF_PRESENT(func, id) \
do { \
	if ((*m_update_agent)->interface->func != NULL) \
		function_presence[(*num_func)++] = (id); \
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

	return res;
}

int direct_fwu_client::begin_staging(uint32_t vendor_flags, uint32_t partial_update_count,
				     struct uuid_octets update_guid[])
{
	return update_agent_begin_staging(*m_update_agent, vendor_flags, partial_update_count,
					  update_guid);
}

int direct_fwu_client::end_staging(void)
{
	return update_agent_end_staging(*m_update_agent);
}

int direct_fwu_client::cancel_staging(void)
{
	return update_agent_cancel_staging(*m_update_agent);
}

int direct_fwu_client::accept(const struct uuid_octets *image_type_uuid)
{
	return update_agent_accept_image(*m_update_agent, image_type_uuid);
}

int direct_fwu_client::select_previous(void)
{
	return update_agent_select_previous(*m_update_agent);
}

int direct_fwu_client::open(const struct uuid_octets *uuid, op_type op_type, uint32_t *handle)
{
	return update_agent_open(*m_update_agent, uuid, (uint8_t)op_type, handle);
}

int direct_fwu_client::commit(uint32_t handle, bool accepted)
{
	uint32_t progress = 0;
	uint32_t total_work = 0;

	return update_agent_commit(*m_update_agent, handle, accepted, 0, &progress, &total_work);
}

int direct_fwu_client::write_stream(uint32_t handle, const uint8_t *data, size_t data_len)
{
	return update_agent_write_stream(*m_update_agent, handle, data, data_len);
}

int direct_fwu_client::read_stream(uint32_t handle, uint8_t *buf, size_t buf_size, size_t *read_len,
				   size_t *total_len)
{
	int status = update_agent_read_stream(*m_update_agent, handle, m_read_buf, READ_BUF_SIZE,
					      read_len, total_len);

	if (!status && buf && buf_size) {
		size_t copy_len = (*read_len <= buf_size) ? *read_len : buf_size;

		memcpy(buf, m_read_buf, copy_len);
	}

	return status;
}
