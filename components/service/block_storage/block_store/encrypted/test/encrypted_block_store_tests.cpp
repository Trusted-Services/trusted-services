/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstring>
#include <service/crypto/client/psa/psa_crypto_client.h>
#include <service_locator.h>

#include "CppUTest/TestHarness.h"
#include "service/block_storage/block_store/device/ram/ram_block_store.h"
#include "service/block_storage/block_store/encrypted/encrypted_block_store.h"

TEST_GROUP(EncryptedBlockStoreTests)
{
	void setup()
	{
		open_crypto_session();

		uuid_guid_octets_from_canonical(&m_partition_guid,
						"6152f22b-8128-4c1f-981f-3bd279519907");

		m_back_store = ram_block_store_init(&m_ram_store, &m_partition_guid, NUM_BLOCKS,
						    BLOCK_SIZE);

		CHECK_TRUE(m_back_store);

		m_block_store = encrypted_block_store_init(&m_encrypted_store, CLIENT_ID,
							   &m_partition_guid, m_back_store);

		CHECK_TRUE(m_block_store);

		m_status = block_store_open(m_encrypted_store.back_store,
					    m_encrypted_store.local_client_id, &m_partition_guid,
					    &m_encrypted_store.back_store_handle);

		LONGS_EQUAL(PSA_SUCCESS, m_status);
	}

	void teardown()
	{
		encrypted_block_store_deinit(&m_encrypted_store);
		ram_block_store_deinit(&m_ram_store);

		close_crypto_session();
	}

	void open_crypto_session()
	{
		service_locator_init();
		m_crypto_service_context = service_locator_query("sn:trustedfirmware.org:crypto:0");

		if (m_crypto_service_context) {
			m_crypto_session = service_context_open(m_crypto_service_context);
			if (m_crypto_session) {
				psa_crypto_client_init(m_crypto_session);
				psa_crypto_init();
			}
		}
	}

	void close_crypto_session()
	{
		psa_crypto_client_deinit();

		if (m_crypto_service_context && m_crypto_session) {
			service_context_close(m_crypto_service_context, m_crypto_session);
			m_crypto_session = NULL;

			service_context_relinquish(m_crypto_service_context);
			m_crypto_service_context = NULL;
		}
	}

	/* Back store configuration */
	static const size_t NUM_BLOCKS = 100;
	static const size_t BLOCK_SIZE = 512;
	static const uint32_t CLIENT_ID = 27;
	static const size_t AES_BLOCK_SIZE = 16;

	struct service_context *m_crypto_service_context;
	struct rpc_caller_session *m_crypto_session;

	psa_status_t m_status = PSA_SUCCESS;
	struct block_store *m_back_store;
	struct block_store *m_block_store;
	struct ram_block_store m_ram_store;
	struct encrypted_block_store m_encrypted_store;
	struct uuid_octets m_partition_guid = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
};

TEST(EncryptedBlockStoreTests, getPartitionInfo)
{
	struct storage_partition_info info;
	struct uuid_octets partition_guid = { 5 };

	/* Request wrong partition guid */
	psa_status_t status = block_store_get_partition_info(m_block_store, &partition_guid, &info);
	LONGS_EQUAL(PSA_ERROR_INVALID_ARGUMENT, status);

	status = block_store_get_partition_info(m_block_store, &m_partition_guid, &info);

	LONGS_EQUAL(PSA_SUCCESS, status);
	LONGS_EQUAL(NUM_BLOCKS, info.num_blocks);
	LONGS_EQUAL(BLOCK_SIZE, info.block_size);
	MEMCMP_EQUAL(m_partition_guid.octets, info.partition_guid.octets,
		     sizeof(info.partition_guid.octets));
}

TEST(EncryptedBlockStoreTests, openClose)
{
	storage_partition_handle_t handle;

	psa_status_t status =
		block_store_open(m_block_store, CLIENT_ID, &m_partition_guid, &handle);
	LONGS_EQUAL(PSA_SUCCESS, status);

	status = block_store_close(m_block_store, CLIENT_ID, handle);
	LONGS_EQUAL(PSA_SUCCESS, status);
}

TEST(EncryptedBlockStoreTests, writeReadEraseBlock)
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

	/* Request write with wrong offset */
	status = block_store_write(m_block_store, CLIENT_ID, handle, lba, BLOCK_SIZE + 1,
				   write_buffer, BLOCK_SIZE, &num_written);
	LONGS_EQUAL(PSA_ERROR_INVALID_ARGUMENT, status);

	/* Expect to be able to write to a block as all blocks are assumed to
	 * be in the erased state */
	memset(write_buffer, 0xaa, BLOCK_SIZE);
	status = block_store_write(m_block_store, CLIENT_ID, handle, lba, 0, write_buffer,
				   BLOCK_SIZE, &num_written);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(BLOCK_SIZE, num_written);

	/* Verify that only the encrypted data can be accessed int the back store */
	memset(read_buffer, 0, BLOCK_SIZE);
	status = block_store_read(m_back_store, CLIENT_ID, handle, lba, 0, BLOCK_SIZE, read_buffer,
				  &data_len);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(BLOCK_SIZE, data_len);
	CHECK(memcmp(write_buffer, read_buffer, BLOCK_SIZE) != 0);

	/* Request read with wrong offset */
	status = block_store_read(m_block_store, CLIENT_ID, handle, lba, BLOCK_SIZE + 1, BLOCK_SIZE,
				  read_buffer, &data_len);
	LONGS_EQUAL(PSA_ERROR_INVALID_ARGUMENT, status);

	/* Read back the original data if read through the encrypted block store */
	memset(read_buffer, 0, BLOCK_SIZE);
	status = block_store_read(m_block_store, CLIENT_ID, handle, lba, 0, BLOCK_SIZE, read_buffer,
				  &data_len);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(BLOCK_SIZE, data_len);
	MEMCMP_EQUAL(write_buffer, read_buffer, BLOCK_SIZE);

	/* Write the same block again without erase. */
	memset(write_buffer, 0xbb, BLOCK_SIZE);
	status = block_store_write(m_block_store, CLIENT_ID, handle, lba, 0, write_buffer,
				   BLOCK_SIZE, &num_written);
	LONGS_EQUAL(PSA_SUCCESS, status);

	/* Erase the block */
	status = block_store_erase(m_block_store, CLIENT_ID, handle, lba, 1);
	LONGS_EQUAL(PSA_SUCCESS, status);

	/* Write to an erased block should work again */
	status = block_store_write(m_block_store, CLIENT_ID, handle, lba, 0, write_buffer,
				   BLOCK_SIZE, &num_written);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(BLOCK_SIZE, num_written);

	status = block_store_close(m_block_store, CLIENT_ID, handle);
	LONGS_EQUAL(PSA_SUCCESS, status);
}

TEST(EncryptedBlockStoreTests, writeBiggerThanBlock)
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
	status = block_store_write(m_block_store, CLIENT_ID, handle, lba, 0, write_buffer,
				   sizeof(write_buffer), &num_written);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(BLOCK_SIZE, num_written);

	/* Expect to read back the same data upto the end of a block */
	memset(read_buffer, 0, BLOCK_SIZE);
	status = block_store_read(m_block_store, CLIENT_ID, handle, lba, 0, BLOCK_SIZE, read_buffer,
				  &data_len);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(BLOCK_SIZE, data_len);
	MEMCMP_EQUAL(write_buffer, read_buffer, BLOCK_SIZE);

	status = block_store_close(m_block_store, CLIENT_ID, handle);
	LONGS_EQUAL(PSA_SUCCESS, status);
}

TEST(EncryptedBlockStoreTests, writeToInvalidBlock)
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
	status = block_store_write(m_block_store, CLIENT_ID, handle, lba, 0, write_buffer,
				   BLOCK_SIZE, &num_written);
	LONGS_EQUAL(PSA_ERROR_INVALID_ARGUMENT, status);

	status = block_store_close(m_block_store, CLIENT_ID, handle);
	LONGS_EQUAL(PSA_SUCCESS, status);
}

TEST(EncryptedBlockStoreTests, eraseOperations)
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

TEST(EncryptedBlockStoreTests, isBackstoreEncrypted)
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
	status = block_store_write(m_block_store, CLIENT_ID, handle, lba, 0, write_buffer,
				   BLOCK_SIZE, &num_written);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(BLOCK_SIZE, num_written);

	/* Expect to read back encrypted data from the back store */
	memset(read_buffer, 0, BLOCK_SIZE);
	status = block_store_read(m_back_store, CLIENT_ID, handle, lba, 0, BLOCK_SIZE, read_buffer,
				  &data_len);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(BLOCK_SIZE, data_len);
	CHECK(memcmp(write_buffer, read_buffer, BLOCK_SIZE) != 0);

	status = block_store_close(m_block_store, CLIENT_ID, handle);
	LONGS_EQUAL(PSA_SUCCESS, status);
}

TEST(EncryptedBlockStoreTests, unalignedBlockStoreInit)
{
	struct block_store *back_store;
	struct block_store *block_store;

	struct ram_block_store ram_store;
	struct encrypted_block_store m_encrypted_store;
	struct uuid_octets partition_guid = { 0 };
	back_store =
		ram_block_store_init(&ram_store, &partition_guid, NUM_BLOCKS, AES_BLOCK_SIZE + 1);

	CHECK_TRUE(back_store);

	block_store = encrypted_block_store_init(&m_encrypted_store, CLIENT_ID, &partition_guid,
						 back_store);

	CHECK_FALSE(block_store);

	ram_block_store_deinit(&ram_store);
}

TEST(EncryptedBlockStoreTests, writeUnaligned)
{
	storage_partition_handle_t handle;
	uint8_t write_buffer[AES_BLOCK_SIZE + 1];
	uint8_t read_buffer[AES_BLOCK_SIZE + 1];
	size_t data_len = 0;
	size_t num_written = 0;
	uint64_t lba = 10;

	psa_status_t status =
		block_store_open(m_block_store, CLIENT_ID, &m_partition_guid, &handle);
	LONGS_EQUAL(PSA_SUCCESS, status);

	/* Expect to be able to write to a block as all blocks are assumed to
	 * be in the erased state */
	memset(write_buffer, 0xaa, AES_BLOCK_SIZE);
	status = block_store_write(m_block_store, CLIENT_ID, handle, lba, 0, write_buffer,
				   AES_BLOCK_SIZE + 1, &num_written);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(AES_BLOCK_SIZE + 1, num_written);

	/* Expect to read back encrypted data from the back store */
	memset(read_buffer, 0, AES_BLOCK_SIZE + 1);
	status = block_store_read(m_back_store, CLIENT_ID, handle, lba, 0, AES_BLOCK_SIZE + 1,
				  read_buffer, &data_len);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(AES_BLOCK_SIZE + 1, data_len);
	CHECK(memcmp(write_buffer, read_buffer, AES_BLOCK_SIZE + 1) != 0);

	status = block_store_close(m_block_store, CLIENT_ID, handle);
	LONGS_EQUAL(PSA_SUCCESS, status);
}

TEST(EncryptedBlockStoreTests, writeInTwoChunks)
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

	for (size_t i = 0; i < BLOCK_SIZE; i++)
		write_buffer[i] = i;

	status = block_store_write(m_block_store, CLIENT_ID, handle, lba, 0, write_buffer,
				   BLOCK_SIZE / 2, &num_written);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(BLOCK_SIZE / 2, num_written);

	status = block_store_write(m_block_store, CLIENT_ID, handle, lba, BLOCK_SIZE / 2,
				   write_buffer + BLOCK_SIZE / 2, BLOCK_SIZE / 2, &num_written);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(BLOCK_SIZE / 2, num_written);

	/* Expect to read back the same data upto the end of a block */
	memset(read_buffer, 0, BLOCK_SIZE);
	status = block_store_read(m_block_store, CLIENT_ID, handle, lba, 0, BLOCK_SIZE, read_buffer,
				  &data_len);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(BLOCK_SIZE, data_len);
	MEMCMP_EQUAL(write_buffer, read_buffer, BLOCK_SIZE);
}
