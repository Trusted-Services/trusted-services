/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "metadata_checker_v2.h"

#include <CppUTest/TestHarness.h>

#include "protocols/service/fwu/metadata_v2.h"
#include "service/fwu/agent/fw_directory.h"

const size_t metadata_checker_v2::MAX_FWU_METADATA_SIZE =
	sizeof(struct fwu_metadata) + offsetof(struct fwu_fw_store_desc, img_entry) +
	sizeof(struct fwu_image_entry) * FWU_MAX_FW_DIRECTORY_ENTRIES;

metadata_checker_v2::metadata_checker_v2(metadata_fetcher *metadata_fetcher,
					 unsigned int num_images)
	: metadata_checker(MAX_FWU_METADATA_SIZE, metadata_fetcher)
{
}

metadata_checker_v2::~metadata_checker_v2()
{
}

void metadata_checker_v2::get_active_indices(uint32_t *active_index,
					     uint32_t *previous_active_index)
{
	load_metadata();

	struct fwu_metadata *metadata = reinterpret_cast<struct fwu_metadata *>(m_meta_buf);

	*active_index = metadata->active_index;
	*previous_active_index = metadata->previous_active_index;
}

void metadata_checker_v2::check_regular(unsigned int boot_index)
{
	load_metadata();

	struct fwu_metadata *metadata = reinterpret_cast<struct fwu_metadata *>(m_meta_buf);

	UNSIGNED_LONGS_EQUAL(boot_index, metadata->active_index);

	BYTES_EQUAL(FWU_METADATA_V2_BANK_STATE_ACCEPTED, metadata->bank_state[boot_index]);
}

void metadata_checker_v2::check_ready_for_staging(unsigned int boot_index)
{
	load_metadata();

	struct fwu_metadata *metadata = reinterpret_cast<struct fwu_metadata *>(m_meta_buf);

	UNSIGNED_LONGS_EQUAL(boot_index, metadata->active_index);

	BYTES_EQUAL(FWU_METADATA_V2_BANK_STATE_ACCEPTED, metadata->bank_state[boot_index]);
	BYTES_EQUAL(FWU_METADATA_V2_BANK_STATE_INVALID,
		    metadata->bank_state[alternate_bank_index(boot_index)]);
}

void metadata_checker_v2::check_ready_to_activate(unsigned int boot_index)
{
	load_metadata();

	struct fwu_metadata *metadata = reinterpret_cast<struct fwu_metadata *>(m_meta_buf);

	UNSIGNED_LONGS_EQUAL(boot_index, metadata->previous_active_index);
	CHECK_TRUE(metadata->active_index != boot_index);

	BYTES_EQUAL(FWU_METADATA_V2_BANK_STATE_VALID, metadata->bank_state[metadata->active_index]);
}

void metadata_checker_v2::check_trial(unsigned int boot_index)
{
	load_metadata();

	struct fwu_metadata *metadata = reinterpret_cast<struct fwu_metadata *>(m_meta_buf);

	UNSIGNED_LONGS_EQUAL(boot_index, metadata->active_index);
	CHECK_TRUE(metadata->previous_active_index != boot_index);

	BYTES_EQUAL(FWU_METADATA_V2_BANK_STATE_VALID, metadata->bank_state[boot_index]);
}

void metadata_checker_v2::check_fallback_to_previous(unsigned int boot_index)
{
	load_metadata();

	struct fwu_metadata *metadata = reinterpret_cast<struct fwu_metadata *>(m_meta_buf);

	UNSIGNED_LONGS_EQUAL(boot_index, metadata->previous_active_index);
	CHECK_TRUE(metadata->active_index != boot_index);

	BYTES_EQUAL(FWU_METADATA_V2_BANK_STATE_ACCEPTED, metadata->bank_state[boot_index]);
}

unsigned int metadata_checker_v2::alternate_bank_index(unsigned int bank_index)
{
	return (bank_index == 0) ? 1 : 0;
}