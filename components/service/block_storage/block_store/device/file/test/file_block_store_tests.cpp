/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <CppUTest/TestHarness.h>
#include <cstdio>
#include <cstring>
#include <stdint.h>
#include <string>

#include "common/uuid/uuid.h"
#include "service/block_storage/block_store/device/file/file_block_store.h"

TEST_GROUP(FileBlockStoreTests)
{
	void setup()
	{
		m_filename = std::string("file_block_store.tmp");
		memset(m_disk_guid.octets, 0, sizeof(m_disk_guid.octets));

		struct block_store *block_store =
			file_block_store_init(&m_file_block_store, &m_disk_guid, m_filename.c_str(), BLOCK_SIZE);

		CHECK_TRUE(block_store);

		psa_status_t status = file_block_store_configure(&m_file_block_store, &m_disk_guid,
								 NUM_BLOCKS, BLOCK_SIZE);

		LONGS_EQUAL(PSA_SUCCESS, status);

		status = block_store_open(&m_file_block_store.base_block_device.base_block_store,
					  CLIENT_ID, &m_disk_guid, &m_partition_handle);

		LONGS_EQUAL(PSA_SUCCESS, status);
	}

	void teardown()
	{
		block_store_close(&m_file_block_store.base_block_device.base_block_store, CLIENT_ID,
				  m_partition_handle);

		file_block_store_deinit(&m_file_block_store);
		remove(m_filename.c_str());
	}

	void set_block(size_t lba, size_t offset, size_t len, uint8_t val, size_t * num_written)
	{
		struct block_store *bs = &m_file_block_store.base_block_device.base_block_store;
		uint8_t write_buf[len];

		memset(write_buf, val, len);
		*num_written = 0;

		psa_status_t status = block_store_write(bs, CLIENT_ID, m_partition_handle, lba,
							offset, write_buf, len, num_written);

		LONGS_EQUAL(PSA_SUCCESS, status);
	}

	void check_block(size_t lba, size_t offset, size_t len, uint8_t expected_val)
	{
		struct block_store *bs = &m_file_block_store.base_block_device.base_block_store;
		uint8_t read_buf[len];
		size_t num_read = 0;

		psa_status_t status = block_store_read(bs, CLIENT_ID, m_partition_handle, lba,
						       offset, len, read_buf, &num_read);

		LONGS_EQUAL(PSA_SUCCESS, status);
		UNSIGNED_LONGS_EQUAL(len, num_read);

		for (size_t i = 0; i < len; i++)
			BYTES_EQUAL(expected_val, read_buf[i]);
	}

	void erase_blocks(uint32_t begin_lba, size_t num_blocks)
	{
		struct block_store *bs = &m_file_block_store.base_block_device.base_block_store;

		psa_status_t status =
			block_store_erase(bs, CLIENT_ID, m_partition_handle, begin_lba, num_blocks);

		LONGS_EQUAL(PSA_SUCCESS, status);
	}

	static const size_t NUM_BLOCKS = 100;
	static const size_t BLOCK_SIZE = 512;
	static const uint32_t CLIENT_ID = 27;

	std::string m_filename;
	struct uuid_octets m_disk_guid;
	struct file_block_store m_file_block_store;
	storage_partition_handle_t m_partition_handle;
};

/*
 * Check a sequence of whole block writes and reads.
 */
TEST(FileBlockStoreTests, wholeBlockRw)
{
	size_t num_written = 0;

	set_block(7, 0, BLOCK_SIZE, 'a', &num_written);
	UNSIGNED_LONGS_EQUAL(BLOCK_SIZE, num_written);

	set_block(6, 0, BLOCK_SIZE, 'b', &num_written);
	UNSIGNED_LONGS_EQUAL(BLOCK_SIZE, num_written);

	set_block(1, 0, BLOCK_SIZE, 'c', &num_written);
	UNSIGNED_LONGS_EQUAL(BLOCK_SIZE, num_written);

	set_block(9, 0, BLOCK_SIZE, 'd', &num_written);
	UNSIGNED_LONGS_EQUAL(BLOCK_SIZE, num_written);

	/* Check written blocks are as expected */
	check_block(7, 0, BLOCK_SIZE, 'a');
	check_block(6, 0, BLOCK_SIZE, 'b');
	check_block(1, 0, BLOCK_SIZE, 'c');
	check_block(9, 0, BLOCK_SIZE, 'd');

	/* Erase all the written blocks */
	erase_blocks(9, 1);
	erase_blocks(1, 1);
	erase_blocks(7, 1);
	erase_blocks(6, 1);
}

/*
 * Check state when initialised with existing disk image file
 */
TEST(FileBlockStoreTests, initWithExistingDiskImage)
{
	/* Write to last block to expand disk image file to entire size */
	size_t num_written = 0;

	set_block(NUM_BLOCKS - 1, 0, BLOCK_SIZE, 'a', &num_written);
	UNSIGNED_LONGS_EQUAL(BLOCK_SIZE, num_written);

	/* Close the block_store opened during setup. This will have created a disk image file */
	block_store_close(&m_file_block_store.base_block_device.base_block_store, CLIENT_ID,
			  m_partition_handle);

	file_block_store_deinit(&m_file_block_store);

	/* Re-initialise and open */
	struct block_store *block_store =
		file_block_store_init(&m_file_block_store, &m_disk_guid, m_filename.c_str(), BLOCK_SIZE);

	CHECK_TRUE(block_store);

	psa_status_t status = file_block_store_configure(&m_file_block_store, &m_disk_guid,
								 NUM_BLOCKS, BLOCK_SIZE);
	LONGS_EQUAL(PSA_SUCCESS, status);

	status =
		block_store_open(block_store, CLIENT_ID, &m_disk_guid, &m_partition_handle);
	LONGS_EQUAL(PSA_SUCCESS, status);

	/* Expect disk partition size to reflect existing disk file */
	struct storage_partition_info disk_info;

	status = block_store_get_partition_info(block_store, &m_disk_guid, &disk_info);
	LONGS_EQUAL(PSA_SUCCESS, status);

	UNSIGNED_LONGS_EQUAL(NUM_BLOCKS, disk_info.num_blocks);
	UNSIGNED_LONGS_EQUAL(BLOCK_SIZE, disk_info.block_size);
}
