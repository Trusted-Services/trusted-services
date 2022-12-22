/*
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstring>
#include <common/uuid/uuid.h>
#include <protocols/service/fwu/packed-c/fwu_proto.h>
#include <CppUTest/TestHarness.h>
#include "client_metadata_fetcher.h"

client_metadata_fetcher::client_metadata_fetcher(
	fwu_client *fwu_client) :
	metadata_fetcher(),
	m_fwu_client(fwu_client)
{

}

client_metadata_fetcher::~client_metadata_fetcher()
{
	delete m_fwu_client;
	m_fwu_client = NULL;
}

void client_metadata_fetcher::open()
{

}

void client_metadata_fetcher::close(void)
{

}

void client_metadata_fetcher::fetch(uint8_t *buf, size_t buf_size)
{
	int status = 0;
	uint32_t stream_handle = 0;
	struct uuid_octets uuid;

	uuid_guid_octets_from_canonical(&uuid, FWU_METADATA_CANONICAL_UUID);

	status = m_fwu_client->open(&uuid, &stream_handle);
	LONGS_EQUAL(0, status);

	size_t read_len = 0;
	size_t total_len = 0;

	status = m_fwu_client->read_stream(
		stream_handle,
		buf, buf_size,
		&read_len, &total_len);
	LONGS_EQUAL(0, status);

	m_fwu_client->commit(stream_handle, false);
}
