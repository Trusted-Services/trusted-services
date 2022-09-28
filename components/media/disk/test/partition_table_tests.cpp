/*
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <string>
#include <cstring>
#include <common/uuid/uuid.h>
#include <service/block_storage/block_store/device/ram/ram_block_store.h>
#include <service/block_storage/config/ref/ref_partition_configurator.h>
#include <media/volume/index/volume_index.h>
#include <media/volume/block_io_dev/block_io_dev.h>
#include <media/volume/base_io_dev/base_io_dev.h>
#include <media/disk/disk_images/ref_partition.h>
#include <media/disk/formatter/disk_formatter.h>
#include <media/disk/partition_table.h>
#include <CppUTest/TestHarness.h>

TEST_GROUP(PartitionTableTests)
{
	void setup()
	{
		/* Load reference GPT image into ram block store */
		size_t block_size = PLAT_PARTITION_BLOCK_SIZE;
		size_t num_blocks = ref_partition_data_length / block_size;

		m_block_store = ram_block_store_init(&m_ram_block_store,
			NULL,
			num_blocks, block_size);

		CHECK_TRUE(m_block_store);

		memset(m_partition_guid.octets, 0, sizeof(m_partition_guid.octets));

		m_dev_handle = 0;
		m_volume_spec = 0;

		int result = block_io_dev_init(&m_block_io_dev,
			m_block_store, &m_partition_guid,
			&m_dev_handle, &m_volume_spec);

		LONGS_EQUAL(0, result);
		CHECK_TRUE(m_dev_handle);
		CHECK_TRUE(m_volume_spec);

		result = disk_formatter_clone(
			m_dev_handle, m_volume_spec,
			ref_partition_data, ref_partition_data_length);

		LONGS_EQUAL(0, result);

		volume_index_init();
		volume_index_add(TEST_VOLUME_ID, m_dev_handle, m_volume_spec);
	}

	void teardown()
	{
		block_io_dev_deinit(&m_block_io_dev);
		ram_block_store_deinit(&m_ram_block_store);
		volume_index_clear();
	}

	void uuid_from_canonical(uuid_t *uuid, const char *canonical)
	{
		uuid_parse_to_octets_reversed(canonical, (uint8_t*)uuid, sizeof(uuid_t));
	}

	void corrupt_mbr()
	{
		/* Scribble over the protective MBR signature */
		static const char scribble[] = "scribble";
		psa_status_t status = ram_block_store_modify(
			&m_ram_block_store, 510,
			(const uint8_t*)scribble, sizeof(scribble));
		LONGS_EQUAL(PSA_SUCCESS, status);
	}

	void corrupt_primary_gpt_header()
	{
		/* Scribble over the primary GPT header in block 1 */
		static const char scribble[] = "scribble";
		psa_status_t status = ram_block_store_modify(
			&m_ram_block_store, PLAT_PARTITION_BLOCK_SIZE * 1,
			(const uint8_t*)scribble, sizeof(scribble));
		LONGS_EQUAL(PSA_SUCCESS, status);
	}

	static const unsigned int TEST_VOLUME_ID = 5;
	static const uint32_t CLIENT_ID = 0;
	static const size_t FIRST_USABLE_LBA = 34;

	struct uuid_octets m_partition_guid;
	struct block_store *m_block_store;
	struct ram_block_store m_ram_block_store;
	struct block_io_dev m_block_io_dev;
	uintptr_t m_dev_handle;
	uintptr_t m_volume_spec;
};

TEST(PartitionTableTests, loadRefPartitionTable)
{
	int result = load_partition_table(TEST_VOLUME_ID);
	LONGS_EQUAL(0, result);

	/* Check for expected partition entries */
	const partition_entry_t *partition_entry = NULL;
	uuid_t partition_guid;

	uuid_from_canonical(&partition_guid, REF_PARTITION_1_GUID);
	partition_entry = get_partition_entry_by_uuid(&partition_guid);
	CHECK_TRUE(partition_entry);
	UNSIGNED_LONGS_EQUAL(
		FIRST_USABLE_LBA + REF_PARTITION_1_STARTING_LBA,
		partition_entry->start / PLAT_PARTITION_BLOCK_SIZE);
	UNSIGNED_LONGS_EQUAL(
		(REF_PARTITION_1_ENDING_LBA - REF_PARTITION_1_STARTING_LBA + 1),
		partition_entry->length / PLAT_PARTITION_BLOCK_SIZE);

	uuid_from_canonical(&partition_guid, REF_PARTITION_2_GUID);
	partition_entry = get_partition_entry_by_uuid(&partition_guid);
	CHECK_TRUE(partition_entry);
	UNSIGNED_LONGS_EQUAL(
		FIRST_USABLE_LBA + REF_PARTITION_2_STARTING_LBA,
		partition_entry->start / PLAT_PARTITION_BLOCK_SIZE);
	UNSIGNED_LONGS_EQUAL(
		(REF_PARTITION_2_ENDING_LBA - REF_PARTITION_2_STARTING_LBA + 1),
		partition_entry->length / PLAT_PARTITION_BLOCK_SIZE);

	uuid_from_canonical(&partition_guid, REF_PARTITION_3_GUID);
	partition_entry = get_partition_entry_by_uuid(&partition_guid);
	CHECK_TRUE(partition_entry);
	UNSIGNED_LONGS_EQUAL(
		FIRST_USABLE_LBA + REF_PARTITION_3_STARTING_LBA,
		partition_entry->start / PLAT_PARTITION_BLOCK_SIZE);
	UNSIGNED_LONGS_EQUAL(
		(REF_PARTITION_3_ENDING_LBA - REF_PARTITION_3_STARTING_LBA + 1),
		partition_entry->length / PLAT_PARTITION_BLOCK_SIZE);

	uuid_from_canonical(&partition_guid, REF_PARTITION_4_GUID);
	partition_entry = get_partition_entry_by_uuid(&partition_guid);
	CHECK_TRUE(partition_entry);
	UNSIGNED_LONGS_EQUAL(
		FIRST_USABLE_LBA + REF_PARTITION_4_STARTING_LBA,
		partition_entry->start / PLAT_PARTITION_BLOCK_SIZE);
	UNSIGNED_LONGS_EQUAL(
		(REF_PARTITION_4_ENDING_LBA - REF_PARTITION_4_STARTING_LBA + 1),
		partition_entry->length / PLAT_PARTITION_BLOCK_SIZE);
}

TEST(PartitionTableTests, detectCorruptedMbr)
{
	corrupt_mbr();
	int result = load_partition_table(TEST_VOLUME_ID);
	LONGS_EQUAL(-ENOENT, result);
}

// Shows up defect in TF-A where failed GPT header CRC results in an assert.
IGNORE_TEST(PartitionTableTests, detectCorruptedGptHeader)
{
	/* Load should be successful with a corrupted primary GPT header as
	 * backup is still available.
	 */
	corrupt_primary_gpt_header();
	int result = load_partition_table(TEST_VOLUME_ID);
	LONGS_EQUAL(0, result);
}
