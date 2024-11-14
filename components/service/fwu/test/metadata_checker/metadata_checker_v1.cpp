/*
 * Copyright (c) 2022-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "metadata_checker_v1.h"

#include <CppUTest/TestHarness.h>

#include "protocols/service/fwu/metadata_v1.h"
#include "service/fwu/agent/fw_directory.h"

const size_t metadata_checker_v1::MAX_FWU_METADATA_SIZE =
	offsetof(struct fwu_metadata, img_entry) +
	FWU_MAX_FW_DIRECTORY_ENTRIES * sizeof(struct fwu_image_entry);

metadata_checker_v1::metadata_checker_v1(metadata_fetcher *metadata_fetcher,
					 unsigned int num_images)
	: metadata_checker(MAX_FWU_METADATA_SIZE, metadata_fetcher)
	, m_num_images(num_images)
{
}

metadata_checker_v1::~metadata_checker_v1()
{
}

void metadata_checker_v1::get_active_indices(uint32_t *active_index,
					     uint32_t *previous_active_index)
{
	load_metadata();

	struct fwu_metadata *metadata = reinterpret_cast<struct fwu_metadata *>(m_meta_buf);

	*active_index = metadata->active_index;
	*previous_active_index = metadata->previous_active_index;
}

void metadata_checker_v1::check_regular(unsigned int boot_index)
{
	load_metadata();

	struct fwu_metadata *metadata = reinterpret_cast<struct fwu_metadata *>(m_meta_buf);

	UNSIGNED_LONGS_EQUAL(boot_index, metadata->active_index);
	CHECK_TRUE(is_all_accepted(boot_index));
}

void metadata_checker_v1::check_ready_for_staging(unsigned int boot_index)
{
	load_metadata();

	struct fwu_metadata *metadata = reinterpret_cast<struct fwu_metadata *>(m_meta_buf);

	UNSIGNED_LONGS_EQUAL(boot_index, metadata->active_index);
	UNSIGNED_LONGS_EQUAL(metadata->active_index, metadata->previous_active_index);
}

void metadata_checker_v1::check_ready_to_activate(unsigned int boot_index)
{
	load_metadata();

	struct fwu_metadata *metadata = reinterpret_cast<struct fwu_metadata *>(m_meta_buf);

	UNSIGNED_LONGS_EQUAL(boot_index, metadata->previous_active_index);
	CHECK_TRUE(metadata->active_index != boot_index);
}

void metadata_checker_v1::check_trial(unsigned int boot_index)
{
	load_metadata();

	struct fwu_metadata *metadata = reinterpret_cast<struct fwu_metadata *>(m_meta_buf);

	UNSIGNED_LONGS_EQUAL(boot_index, metadata->active_index);
	CHECK_TRUE(metadata->previous_active_index != boot_index);
	CHECK_FALSE(is_all_accepted(boot_index));
}

void metadata_checker_v1::check_fallback_to_previous(unsigned int boot_index)
{
	load_metadata();

	struct fwu_metadata *metadata = reinterpret_cast<struct fwu_metadata *>(m_meta_buf);

	UNSIGNED_LONGS_EQUAL(boot_index, metadata->previous_active_index);
	CHECK_TRUE(metadata->active_index != boot_index);
}

bool metadata_checker_v1::is_all_accepted(unsigned int boot_index) const
{
	bool result = true;
	struct fwu_metadata *metadata = reinterpret_cast<struct fwu_metadata *>(m_meta_buf);

	for (unsigned int i = 0; i < m_num_images; i++) {
		if (!metadata->img_entry[i].img_props[boot_index].accepted) {
			result = false;
			break;
		}
	}

	return result;
}
