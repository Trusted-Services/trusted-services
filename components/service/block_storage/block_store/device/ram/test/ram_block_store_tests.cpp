/*
 * Copyright (c) 2022-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstring>
#include <stdint.h>
#include "common/uuid/uuid.h"
#include "service/block_storage/block_store/device/ram/ram_block_store.h"
#include "CppUTest/TestHarness.h"

TEST_GROUP(RamBlockStoreTests)
{
	void setup()
	{
		uuid_guid_octets_from_canonical(&m_partition_guid,
			"6152f22b-8128-4c1f-981f-3bd279519907");

		m_block_store = ram_block_store_init(&m_ram_block_store,
			&m_partition_guid, NUM_BLOCKS, BLOCK_SIZE);

		CHECK_TRUE(m_block_store);
	}

	void teardown()
	{
		ram_block_store_deinit(&m_ram_block_store);
	}

	static const size_t NUM_BLOCKS = 100;
	static const size_t BLOCK_SIZE = 512;
	static const uint32_t CLIENT_ID = 27;

	struct uuid_octets m_partition_guid;
	struct block_store *m_block_store;
	struct ram_block_store m_ram_block_store;
};

TEST(RamBlockStoreTests, getPartitionInfo)
{
	struct storage_partition_info info;

	psa_status_t status =
		block_store_get_partition_info(m_block_store, &m_partition_guid, &info);

	LONGS_EQUAL(PSA_SUCCESS, status);
	LONGS_EQUAL(NUM_BLOCKS, info.num_blocks);
	LONGS_EQUAL(BLOCK_SIZE, info.block_size);
	MEMCMP_EQUAL(m_partition_guid.octets,
		info.partition_guid.octets, sizeof(info.partition_guid.octets));
}

TEST(RamBlockStoreTests, openClose)
{
	storage_partition_handle_t handle;

	psa_status_t status =
		block_store_open(m_block_store, CLIENT_ID, &m_partition_guid, &handle);
	LONGS_EQUAL(PSA_SUCCESS, status);

	status = block_store_close(m_block_store, CLIENT_ID, handle);
	LONGS_EQUAL(PSA_SUCCESS, status);
}

TEST(RamBlockStoreTests, writeReadEraseBlock)
{
	storage_partition_handle_t handle;
	uint8_t write_buffer[BLOCK_SIZE];
	uint8_t read_buffer[BLOCK_SIZE];
	size_t data_len = 0;
	size_t num_written = 0;
	uint64_t lba = 10;

	psa_status_t status =
		block_store_open(m_block_store, CLIENT_ID, &m_partition_guid, &handle);
	LONGS_EQUAL(PSA_SUCCESS, status);

	/* Expect to be able to write to a block as all blocks are assumed to
	 * be in the erased state */
	memset(write_buffer, 0xaa, BLOCK_SIZE);
	status = block_store_write(m_block_store, CLIENT_ID, handle, lba,
		0, write_buffer, BLOCK_SIZE, &num_written);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(BLOCK_SIZE, num_written);

	/* Expect to read back the same data */
	memset(read_buffer, 0, BLOCK_SIZE);
	status = block_store_read(m_block_store, CLIENT_ID, handle, lba,
		0, BLOCK_SIZE, read_buffer, &data_len);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(BLOCK_SIZE, data_len);
	MEMCMP_EQUAL(write_buffer, read_buffer, BLOCK_SIZE);

	/* Write to the previously written block in partition 2 should be successful */
	memset(write_buffer, 0xbb, BLOCK_SIZE);
	status = block_store_write(m_block_store, CLIENT_ID, handle, lba,
		0, write_buffer, BLOCK_SIZE, &num_written);
	LONGS_EQUAL(PSA_SUCCESS, status);

	/* Erase the block */
	status = block_store_erase(m_block_store, CLIENT_ID, handle, lba, 1);
	LONGS_EQUAL(PSA_SUCCESS, status);

	/* Write to an erased block should now work */
	status = block_store_write(m_block_store, CLIENT_ID, handle, lba,
		0, write_buffer, BLOCK_SIZE, &num_written);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(BLOCK_SIZE, num_written);

	status = block_store_close(m_block_store, CLIENT_ID, handle);
	LONGS_EQUAL(PSA_SUCCESS, status);
}

TEST(RamBlockStoreTests, writeBiggerThanBlock)
{
	storage_partition_handle_t handle;
	uint8_t write_buffer[BLOCK_SIZE + BLOCK_SIZE / 2];
	uint8_t read_buffer[BLOCK_SIZE];
	size_t data_len = 0;
	size_t num_written = 0;
	uint64_t lba = 10;

	psa_status_t status =
		block_store_open(m_block_store, CLIENT_ID, &m_partition_guid, &handle);
	LONGS_EQUAL(PSA_SUCCESS, status);

	/* Try writing more than the block size. Expect the operation to succeed but
	 * that the written data is limited to the block size*/
	memset(write_buffer, 0xaa, sizeof(write_buffer));
	status = block_store_write(m_block_store, CLIENT_ID, handle, lba,
		0, write_buffer, sizeof(write_buffer), &num_written);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(BLOCK_SIZE, num_written);

	/* Expect to read back the same data upto the end of a block */
	memset(read_buffer, 0, BLOCK_SIZE);
	status = block_store_read(m_block_store, CLIENT_ID, handle, lba,
		0, BLOCK_SIZE, read_buffer, &data_len);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(BLOCK_SIZE, data_len);
	MEMCMP_EQUAL(write_buffer, read_buffer, BLOCK_SIZE);

	status = block_store_close(m_block_store, CLIENT_ID, handle);
	LONGS_EQUAL(PSA_SUCCESS, status);
}

TEST(RamBlockStoreTests, writeToInvalidBlock)
{
	storage_partition_handle_t handle;
	uint8_t write_buffer[BLOCK_SIZE];
	size_t num_written = 0;
	uint64_t lba = NUM_BLOCKS + 7;

	psa_status_t status =
		block_store_open(m_block_store, CLIENT_ID, &m_partition_guid, &handle);
	LONGS_EQUAL(PSA_SUCCESS, status);

	/* Write to block beyond the limits of the storage partition should fail */
	memset(write_buffer, 0xaa, BLOCK_SIZE);
	status = block_store_write(m_block_store, CLIENT_ID, handle, lba,
		0, write_buffer, BLOCK_SIZE, &num_written);
	LONGS_EQUAL(PSA_ERROR_INVALID_ARGUMENT, status);

	status = block_store_close(m_block_store, CLIENT_ID, handle);
	LONGS_EQUAL(PSA_SUCCESS, status);
}

TEST(RamBlockStoreTests, eraseOperations)
{
	storage_partition_handle_t handle;

	psa_status_t status =
		block_store_open(m_block_store, CLIENT_ID, &m_partition_guid, &handle);
	LONGS_EQUAL(PSA_SUCCESS, status);

	/* Erase all blocks */
	status = block_store_erase(m_block_store, CLIENT_ID, handle, 0, NUM_BLOCKS);
	LONGS_EQUAL(PSA_SUCCESS, status);

	/* Erase all blocks using a length that exceeds the partition limit - should clip */
	status = block_store_erase(m_block_store, CLIENT_ID, handle, 0, UINT32_MAX);
	LONGS_EQUAL(PSA_SUCCESS, status);

	/* Begin LBA is outside of partition */
	status = block_store_erase(m_block_store, CLIENT_ID, handle, NUM_BLOCKS + 1, NUM_BLOCKS);
	LONGS_EQUAL(PSA_ERROR_INVALID_ARGUMENT, status);

	status = block_store_close(m_block_store, CLIENT_ID, handle);
	LONGS_EQUAL(PSA_SUCCESS, status);
}