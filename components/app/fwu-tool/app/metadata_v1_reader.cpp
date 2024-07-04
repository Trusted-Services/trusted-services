/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include "metadata_reader.h"
#include "protocols/service/fwu/metadata_v1.h"

class metadata_v1_reader : public metadata_version_specific_reader {
public:
	metadata_v1_reader();
	~metadata_v1_reader();

	bool is_supported(const uint8_t *buf, size_t data_len) const override;

	void get_version(const uint8_t *buf, size_t data_len, unsigned int &version) const override;

	void get_active_index(const uint8_t *buf, size_t data_len,
			      unsigned int &active_index) const override;
};

/* Registers on static construction */
static metadata_v1_reader the_v1_reader;

metadata_v1_reader::metadata_v1_reader()
	: metadata_version_specific_reader()
{
	metadata_reader::instance()->register_reader(this);
}

metadata_v1_reader::~metadata_v1_reader()
{
}

bool metadata_v1_reader::is_supported(const uint8_t *buf, size_t data_len) const
{
	assert(buf);

	const struct fwu_metadata *metadata = (const struct fwu_metadata *)buf;

	return (data_len >= sizeof(struct fwu_metadata)) && (metadata->version == 1);
}

void metadata_v1_reader::get_version(const uint8_t *buf, size_t data_len,
				     unsigned int &version) const
{
	assert(buf);
	assert(data_len >= sizeof(struct fwu_metadata));

	const struct fwu_metadata *metadata = (const struct fwu_metadata *)buf;

	version = metadata->version;
}

void metadata_v1_reader::get_active_index(const uint8_t *buf, size_t data_len,
					  unsigned int &active_index) const
{
	assert(buf);
	assert(data_len >= sizeof(struct fwu_metadata));

	const struct fwu_metadata *metadata = (const struct fwu_metadata *)buf;

	active_index = metadata->active_index;
}