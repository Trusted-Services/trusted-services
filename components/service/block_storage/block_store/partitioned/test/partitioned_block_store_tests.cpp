/*
 * Copyright (c) 2022-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstring>
#include "common/uuid/uuid.h"
#include "service/block_storage/block_store/device/ram/ram_block_store.h"
#include "service/block_storage/block_store/partitioned/partitioned_block_store.h"
#include "CppUTest/TestHarness.h"

TEST_GROUP(PartitionedBlockStoreTests)
{
	void setup()
	{
		/* Initialize a ram_block_store to use as the back store */
		uuid_guid_octets_from_canonical(&m_back_store_guid,
			"6ec10ff6-4252-4ef7-aeca-5036db6697df");

		struct block_store *back_store = ram_block_store_init(
			&m_ram_store,
			&m_back_store_guid,
			BACK_STORE_NUM_BLOCKS,
			BACK_STORE_BLOCK_SIZE);

		CHECK_TRUE(back_store);

		/* Stack a partitioned_block_store over the back store */
		m_block_store = partitioned_block_store_init(
			&m_partitioned_store,
			LOCAL_CLIENT_ID,
			&m_back_store_guid,
			back_store,
			NULL);

		CHECK_TRUE(back_store);

		/* Configure partition 1 */
		uuid_guid_octets_from_canonical(&m_partition_1_guid,
			"18ae7d62-0974-4dd3-8d2e-e9c166554165");

		CHECK_TRUE(partitioned_block_store_add_partition(
			&m_partitioned_store,
			&m_partition_1_guid,
			PARTITION_1_STARTING_LBA,
			PARTITION_1_ENDING_LBA,
			0, NULL));

		/* Configure partition 2 */
		uuid_guid_octets_from_canonical(&m_partition_2_guid,
			"7924fda7-4666-41d7-887a-91a913712b93");

		CHECK_TRUE(partitioned_block_store_add_partition(
			&m_partitioned_store,
			&m_partition_2_guid,
			PARTITION_2_STARTING_LBA,
			PARTITION_2_ENDING_LBA,
			0, NULL));
	}

	void teardown()
	{
		ram_block_store_deinit(&m_ram_store);
		partitioned_block_store_deinit(&m_partitioned_store);
	}

	/* Back store configuration */
	static const size_t BACK_STORE_NUM_BLOCKS = 100;
	static const size_t BACK_STORE_BLOCK_SIZE = 512;

	/* Partition 1 configuration */
	static const size_t PARTITION_1_STARTING_LBA = 5;
	static const size_t PARTITION_1_ENDING_LBA = 49;

	/* Partition 2 configuration */
	static const size_t PARTITION_2_STARTING_LBA = 60;
	static const size_t PARTITION_2_ENDING_LBA = 99;

	static const uint32_t LOCAL_CLIENT_ID = 11;

	struct block_store *m_block_store;
	struct ram_block_store m_ram_store;
	struct partitioned_block_store m_partitioned_store;
	struct uuid_octets m_partition_1_guid;
	struct uuid_octets m_partition_2_guid;
	struct uuid_octets m_back_store_guid;
};

TEST(PartitionedBlockStoreTests, getPartitionInfo)
{
	struct storage_partition_info info;

	/* Check partition info for partition 1 */
	psa_status_t status = block_store_get_partition_info(
		m_block_store, &m_partition_1_guid, &info);

	LONGS_EQUAL(PSA_SUCCESS, status);
	LONGS_EQUAL(PARTITION_1_ENDING_LBA - PARTITION_1_STARTING_LBA + 1, info.num_blocks);
	LONGS_EQUAL(BACK_STORE_BLOCK_SIZE, info.block_size);
	MEMCMP_EQUAL(m_partition_1_guid.octets,
		info.partition_guid.octets, sizeof(info.partition_guid.octets));
	MEMCMP_EQUAL(m_back_store_guid.octets,
		info.parent_guid.octets, sizeof(info.parent_guid.octets));

	/* Check partition info for partition 2 */
	status = block_store_get_partition_info(
		m_block_store, &m_partition_2_guid, &info);

	LONGS_EQUAL(PSA_SUCCESS, status);
	LONGS_EQUAL(PARTITION_2_ENDING_LBA - PARTITION_2_STARTING_LBA + 1, info.num_blocks);
	LONGS_EQUAL(BACK_STORE_BLOCK_SIZE, info.block_size);
	MEMCMP_EQUAL(m_partition_2_guid.octets,
		info.partition_guid.octets, sizeof(info.partition_guid.octets));
	MEMCMP_EQUAL(m_back_store_guid.octets,
		info.parent_guid.octets, sizeof(info.parent_guid.octets));
}

TEST(PartitionedBlockStoreTests, openClose)
{
	storage_partition_handle_t handle_1;
	storage_partition_handle_t handle_2;

	psa_status_t status = block_store_open(
		m_block_store, LOCAL_CLIENT_ID, &m_partition_1_guid, &handle_1);
	LONGS_EQUAL(PSA_SUCCESS, status);

	status = block_store_open(
		m_block_store, LOCAL_CLIENT_ID, &m_partition_1_guid, &handle_2);
	LONGS_EQUAL(PSA_SUCCESS, status);

	status = block_store_close(m_block_store, LOCAL_CLIENT_ID, handle_1);
	LONGS_EQUAL(PSA_SUCCESS, status);

	status = block_store_close(m_block_store, LOCAL_CLIENT_ID, handle_2);
	LONGS_EQUAL(PSA_SUCCESS, status);
}

TEST(PartitionedBlockStoreTests, writeReadEraseBlock)
{
	storage_partition_handle_t handle_1;
	storage_partition_handle_t handle_2;
	uint8_t write_buffer[BACK_STORE_BLOCK_SIZE];
	uint8_t read_buffer[BACK_STORE_BLOCK_SIZE];
	size_t data_len = 0;
	size_t num_written = 0;
	uint64_t lba_1 = 10;
	uint64_t lba_2 = 7;

	/* Open sessions associated with both partitions */
	psa_status_t status = block_store_open(
		m_block_store, LOCAL_CLIENT_ID, &m_partition_1_guid, &handle_1);
	LONGS_EQUAL(PSA_SUCCESS, status);

	status = block_store_open(
		m_block_store, LOCAL_CLIENT_ID, &m_partition_2_guid, &handle_2);
	LONGS_EQUAL(PSA_SUCCESS, status);

	/* Write different data to both partitions - expect both to work */
	memset(write_buffer, 0xaa, sizeof(write_buffer));
	status = block_store_write(
		m_block_store, LOCAL_CLIENT_ID, handle_1, lba_1,
		0, write_buffer, sizeof(write_buffer), &num_written);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(sizeof(write_buffer), num_written);

	memset(write_buffer, 0xbb, sizeof(write_buffer));
	status = block_store_write(
		m_block_store, LOCAL_CLIENT_ID, handle_2, lba_2,
		0, write_buffer, sizeof(write_buffer), &num_written);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(sizeof(write_buffer), num_written);

	/* Expect to read back the same data from both partitions*/
	memset(write_buffer, 0xaa, sizeof(write_buffer));
	memset(read_buffer, 0, sizeof(read_buffer));
	status = block_store_read(
		m_block_store, LOCAL_CLIENT_ID, handle_1, lba_1, 0,
		sizeof(read_buffer), read_buffer, &data_len);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(sizeof(write_buffer), data_len);
	MEMCMP_EQUAL(write_buffer, read_buffer, sizeof(write_buffer));

	memset(write_buffer, 0xbb, sizeof(write_buffer));
	memset(read_buffer, 0, sizeof(read_buffer));
	status = block_store_read(
		m_block_store, LOCAL_CLIENT_ID, handle_2, lba_2,
		0, sizeof(read_buffer), read_buffer, &data_len);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(sizeof(write_buffer), data_len);
	MEMCMP_EQUAL(write_buffer, read_buffer, sizeof(write_buffer));

	/* Erase the block in the partition 1 */
	status = block_store_erase(m_block_store, LOCAL_CLIENT_ID, handle_1, lba_1, 1);
	LONGS_EQUAL(PSA_SUCCESS, status);

	/* Write to an erased block in partition 1 should work as block was erase */
	status = block_store_write(
		m_block_store, LOCAL_CLIENT_ID, handle_1, lba_1,
		0, write_buffer, sizeof(write_buffer), &num_written);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(sizeof(write_buffer), num_written);

	/* Write to the previously written block in partition 2 should be successful */
	status = block_store_write(
		m_block_store, LOCAL_CLIENT_ID, handle_2, lba_2,
		0, write_buffer, sizeof(write_buffer), &num_written);
	LONGS_EQUAL(PSA_SUCCESS, status);

	/* Erase the block in the partition 2 */
	status = block_store_erase(m_block_store, LOCAL_CLIENT_ID, handle_2, lba_2, 1);
	LONGS_EQUAL(PSA_SUCCESS, status);

	/* Now the write to partition 2 should work again */
	status = block_store_write(
		m_block_store, LOCAL_CLIENT_ID, handle_2, lba_2,
		0, write_buffer, sizeof(write_buffer), &num_written);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(sizeof(write_buffer), num_written);

	/* Expect to successfully close both sessions */
	status = block_store_close(m_block_store, LOCAL_CLIENT_ID, handle_1);
	LONGS_EQUAL(PSA_SUCCESS, status);

	status = block_store_close(m_block_store, LOCAL_CLIENT_ID, handle_2);
	LONGS_EQUAL(PSA_SUCCESS, status);
}

TEST(PartitionedBlockStoreTests, checkEraseLimits)
{
	storage_partition_handle_t handle_1;
	storage_partition_handle_t handle_2;
	uint8_t write_buffer[BACK_STORE_BLOCK_SIZE];
	uint8_t read_buffer[BACK_STORE_BLOCK_SIZE];
	size_t data_len = 0;
	size_t num_written = 0;

	/* Open sessions associated with both partitions */
	psa_status_t status = block_store_open(
		m_block_store, LOCAL_CLIENT_ID, &m_partition_1_guid, &handle_1);
	LONGS_EQUAL(PSA_SUCCESS, status);

	status = block_store_open(
		m_block_store, LOCAL_CLIENT_ID, &m_partition_2_guid, &handle_2);
	LONGS_EQUAL(PSA_SUCCESS, status);

	/* Write same data to the beginning of both partitions */
	memset(write_buffer, 0xaa, sizeof(write_buffer));
	status = block_store_write(
		m_block_store, LOCAL_CLIENT_ID, handle_1, 0,
		0, write_buffer, sizeof(write_buffer), &num_written);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(sizeof(write_buffer), num_written);

	status = block_store_write(
		m_block_store, LOCAL_CLIENT_ID, handle_2, 0,
		0, write_buffer, sizeof(write_buffer), &num_written);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(sizeof(write_buffer), num_written);

	/* Expect to read back the same data from both partitions*/
	memset(read_buffer, 0, sizeof(read_buffer));
	status = block_store_read(
		m_block_store, LOCAL_CLIENT_ID, handle_1, 0, 0,
		sizeof(read_buffer), read_buffer, &data_len);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(sizeof(write_buffer), data_len);
	MEMCMP_EQUAL(write_buffer, read_buffer, sizeof(write_buffer));

	memset(read_buffer, 0, sizeof(read_buffer));
	status = block_store_read(
		m_block_store, LOCAL_CLIENT_ID, handle_2, 0,
		0, sizeof(read_buffer), read_buffer, &data_len);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(sizeof(write_buffer), data_len);
	MEMCMP_EQUAL(write_buffer, read_buffer, sizeof(write_buffer));

	/* Erase partition 1 with a block count that exceeds the limits of the partition */
	status = block_store_erase(m_block_store, LOCAL_CLIENT_ID, handle_1, 0, 60);
	LONGS_EQUAL(PSA_SUCCESS, status);

	/* Still expect data in partition 2 to be intact */
	memset(read_buffer, 0, sizeof(read_buffer));
	status = block_store_read(
		m_block_store, LOCAL_CLIENT_ID, handle_2, 0,
		0, sizeof(read_buffer), read_buffer, &data_len);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(sizeof(write_buffer), data_len);
	MEMCMP_EQUAL(write_buffer, read_buffer, sizeof(write_buffer));

	/* Expect to successfully close both sessions */
	status = block_store_close(m_block_store, LOCAL_CLIENT_ID, handle_1);
	LONGS_EQUAL(PSA_SUCCESS, status);

	status = block_store_close(m_block_store, LOCAL_CLIENT_ID, handle_2);
	LONGS_EQUAL(PSA_SUCCESS, status);
}


TEST(PartitionedBlockStoreTests, writeToInvalidBlock)
{
	storage_partition_handle_t handle_1;
	storage_partition_handle_t handle_2;
	uint8_t write_buffer[BACK_STORE_BLOCK_SIZE];
	size_t num_written = 0;
	struct storage_partition_info info_1;
	struct storage_partition_info info_2;

	/* Choose LBAs that lie outside of limits of partitions */
	psa_status_t status = block_store_get_partition_info(
		m_block_store, &m_partition_1_guid, &info_1);
	LONGS_EQUAL(PSA_SUCCESS, status);

	status = block_store_get_partition_info(
		m_block_store, &m_partition_2_guid, &info_2);
	LONGS_EQUAL(PSA_SUCCESS, status);

	uint64_t lba_1 = info_1.num_blocks;
	uint64_t lba_2 = info_2.num_blocks + 1000;

	status = block_store_open(
		m_block_store, LOCAL_CLIENT_ID, &m_partition_1_guid, &handle_1);
	LONGS_EQUAL(PSA_SUCCESS, status);

	status = block_store_open(
		m_block_store, LOCAL_CLIENT_ID, &m_partition_2_guid, &handle_2);
	LONGS_EQUAL(PSA_SUCCESS, status);

	/* Write to block beyond the limits of the storage partition should fail */
	memset(write_buffer, 0xaa, sizeof(write_buffer));
	status = block_store_write(
		m_block_store, LOCAL_CLIENT_ID, handle_1, lba_1,
		0, write_buffer, sizeof(write_buffer), &num_written);
	LONGS_EQUAL(PSA_ERROR_INVALID_ARGUMENT, status);

	status = block_store_write(
		m_block_store, LOCAL_CLIENT_ID, handle_2, lba_2,
		0, write_buffer, sizeof(write_buffer), &num_written);
	LONGS_EQUAL(PSA_ERROR_INVALID_ARGUMENT, status);

	/* Close the sessions */
	status = block_store_close(m_block_store, LOCAL_CLIENT_ID, handle_1);
	LONGS_EQUAL(PSA_SUCCESS, status);

	status = block_store_close(m_block_store, LOCAL_CLIENT_ID, handle_2);
	LONGS_EQUAL(PSA_SUCCESS, status);
}
