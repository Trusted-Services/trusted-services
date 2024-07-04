/*
 * Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "image_directory_checker.h"

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "common/uuid/uuid.h"

image_directory_checker::image_directory_checker()
	: m_buf(NULL)
	, m_buf_size(0)
	, m_total_read_len(0)
{
	alloc_buffer();
}

image_directory_checker::~image_directory_checker()
{
	delete[] m_buf;
}

int image_directory_checker::fetch_image_directory(fwu_client *fwu_client)
{
	int status = 0;
	uint32_t stream_handle = 0;
	size_t reported_total_len = 0;
	struct uuid_octets uuid;

	uuid_guid_octets_from_canonical(&uuid, FWU_DIRECTORY_CANONICAL_UUID);

	status = fwu_client->open(&uuid, fwu_client::op_type::READ, &stream_handle);
	if (status)
		return status;

	/* Read stream until all data is read */
	m_total_read_len = 0;

	do {
		size_t data_len_read = 0;
		size_t requested_read_len = m_buf_size - m_total_read_len;

		status = fwu_client->read_stream(stream_handle, &m_buf[m_total_read_len],
						 requested_read_len, &data_len_read,
						 &reported_total_len);

		m_total_read_len += data_len_read;

		assert(m_total_read_len <= reported_total_len);

		if (m_total_read_len == reported_total_len) {
			/* Read all the data */
			break;
		}

	} while (!status);

	status = fwu_client->commit(stream_handle, false);

	return status;
}

size_t image_directory_checker::num_images(void) const
{
	size_t num_images = 0;

	if (m_total_read_len >= offsetof(struct fwu_image_directory, img_info_entry)) {
		const struct fwu_image_directory *header =
			(const struct fwu_image_directory *)m_buf;

		num_images = header->num_images;
	}

	return num_images;
}

bool image_directory_checker::is_contents_equal(const image_directory_checker &rhs) const
{
	return (this->m_total_read_len > 0) && (this->m_total_read_len == rhs.m_total_read_len) &&
	       (this->m_buf && rhs.m_buf) &&
	       (memcmp(this->m_buf, rhs.m_buf, this->m_total_read_len) == 0);
}

const struct fwu_image_directory *image_directory_checker::get_header(void) const
{
	const struct fwu_image_directory *header = NULL;

	if (m_total_read_len >= offsetof(struct fwu_image_directory, img_info_entry))
		header = (const struct fwu_image_directory *)m_buf;

	return header;
}

const struct fwu_image_info_entry *
image_directory_checker::find_entry(const struct uuid_octets *img_type_uuid) const
{
	const struct fwu_image_info_entry *found_entry = NULL;

	const struct fwu_image_directory *header = get_header();

	if (header) {
		unsigned int index = 0;

		while ((const uint8_t *)&header->img_info_entry[index + 1] <=
		       &m_buf[m_total_read_len]) {
			if (uuid_is_equal(img_type_uuid->octets,
					  header->img_info_entry[index].img_type_uuid)) {
				found_entry = &header->img_info_entry[index];
				break;
			}

			++index;
		}
	}

	return found_entry;
}

void image_directory_checker::alloc_buffer(void)
{
	m_buf_size = offsetof(struct fwu_image_directory, img_info_entry) +
		     MAX_IMAGES * sizeof(fwu_image_info_entry);

	m_buf = new uint8_t[m_buf_size];
	assert(m_buf);
}
