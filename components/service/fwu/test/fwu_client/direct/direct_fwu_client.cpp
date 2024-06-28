/*
 * Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "direct_fwu_client.h"

#include <cstring>

#include "service/fwu/common/update_agent_interface.h"

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

int direct_fwu_client::begin_staging(void)
{
	return update_agent_begin_staging(*m_update_agent, 0, 0, NULL);
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

int direct_fwu_client::open(const struct uuid_octets *uuid, uint32_t *handle)
{
	return update_agent_open(*m_update_agent, uuid, 0, handle);
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
