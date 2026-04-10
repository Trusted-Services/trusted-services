/*
 * Copyright (c) 2026, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <cstdio>
#include <cstring>
#include <stdint.h>
#include <string>
#include <vector>
#include <stdlib.h>

#include "common/uuid/uuid.h"
#include "components/service/block_storage/block_store/device/rpmb/rpmb_block_store.h"
#include "deployments/unit-tests/include/unit_test_utils.h"
#include "components/common/libc/mock/mock_libc.h"
#include "components/service/rpmb/frontend/mock/mock_rpmb_frontend.h"

TEST_GROUP(RpmbBlockStoreTests)
{
	void setup()
	{
		m_rpmb_frontend = {
			.platform = NULL,
			.backend = NULL,
			.dev_id = 0,
			.initialized = 1,
			.block_count = 0,
			.key = {},
			.write_counter = 0
		};

		uuid_guid_octets_from_canonical(&m_disk_guid,
						"6152f22b-8128-4c1f-981f-3bd279519907");
		memset(&m_null_guid, 0, sizeof(m_null_guid));
	}

	TEST_TEARDOWN()
	{
		mock().checkExpectations();
		mock().removeAllComparatorsAndCopiers();
		mock().clear();
	}

	void mock_init_store(size_t block_size = 256, size_t block_count = 2048)
	{
		expect_rpmb_frontend_block_size(&m_rpmb_frontend, block_size, PSA_SUCCESS);
		expect_rpmb_frontend_block_count(&m_rpmb_frontend, block_count, PSA_SUCCESS);

		m_block_store = rpmb_block_store_init(&m_rpmb_block_store, &m_disk_guid, &m_rpmb_frontend, 512);
		CHECK(m_block_store != NULL);
	}

	void mock_deinit_store(void)
	{
		mock().disable();
		rpmb_block_store_deinit(&m_rpmb_block_store);
		mock().enable();
	}


	static const size_t NUM_BLOCKS = 100;
	static const size_t BLOCK_SIZE = 512;
	static const uint32_t CLIENT_ID = 21u;

	struct rpmb_block_store m_rpmb_block_store;
	struct block_store *m_back_store;
	struct block_store *m_block_store;
	struct uuid_octets m_disk_guid;
	struct uuid_octets m_null_guid;
	storage_partition_handle_t m_partition_handle;
	struct storage_partition_info info = { NUM_BLOCKS, BLOCK_SIZE, { 0 }, { 0 } };
	struct rpmb_frontend m_rpmb_frontend;
};

TEST(RpmbBlockStoreTests, init_FrontendFailsReturningMediaInfo)
{
	struct block_store *store = NULL;
	struct rpmb_block_store rpmb_block_store = {};
	struct uuid_octets partition_guid = {};
	struct rpmb_frontend rpmb_frontend = {
		.platform = NULL,
		.backend = NULL,
		.dev_id = 0,
		.initialized = 1,
		.block_count = 0,
		.key = {},
		.write_counter = 0
	};

	expect_rpmb_frontend_block_count(&rpmb_frontend, 2048, PSA_SUCCESS);
	expect_rpmb_frontend_block_size(&rpmb_frontend, 256, PSA_ERROR_GENERIC_ERROR);

	store = rpmb_block_store_init(&rpmb_block_store, &partition_guid, &rpmb_frontend, 512);
	CHECK(store == NULL);

	expect_rpmb_frontend_block_count(&rpmb_frontend, 2048, PSA_ERROR_GENERIC_ERROR);

	store = rpmb_block_store_init(&rpmb_block_store, &partition_guid, &rpmb_frontend, 512);
	CHECK(store == NULL);
}

TEST(RpmbBlockStoreTests, init_invalidBlockSize)
{
	struct block_store *store = NULL;
	struct rpmb_block_store rpmb_block_store = {};
	struct uuid_octets partition_guid = {};
	struct rpmb_frontend rpmb_frontend = {
		.platform = NULL,
		.backend = NULL,
		.dev_id = 0,
		.initialized = 1,
		.block_count = 0,
		.key = {},
		.write_counter = 0
	};

	// block size not power of 2
	expect_rpmb_frontend_block_count(&rpmb_frontend, 2048, PSA_SUCCESS);
	expect_rpmb_frontend_block_size(&rpmb_frontend, 257, PSA_SUCCESS);
	store = rpmb_block_store_init(&rpmb_block_store, &partition_guid, &rpmb_frontend, 512);
	CHECK(store == NULL);

	// LBA size not power of 2
	expect_rpmb_frontend_block_count(&rpmb_frontend, 2048, PSA_SUCCESS);
	expect_rpmb_frontend_block_size(&rpmb_frontend, 256, PSA_SUCCESS);
	store = rpmb_block_store_init(&rpmb_block_store, &partition_guid, &rpmb_frontend, 256*3);
	CHECK(store == NULL);

	// LBA smaller than block size
	expect_rpmb_frontend_block_count(&rpmb_frontend, 2048, PSA_SUCCESS);
	expect_rpmb_frontend_block_size(&rpmb_frontend, 256, PSA_SUCCESS);
	store = rpmb_block_store_init(&rpmb_block_store, &partition_guid, &rpmb_frontend, 8);
	CHECK(store == NULL);
}

TEST(RpmbBlockStoreTests, init_OutOfMemory)
{
	struct block_store *store = NULL;
	struct rpmb_block_store rpmb_block_store = {};
	struct uuid_octets partition_guid = {};
	struct rpmb_frontend rpmb_frontend = {
		.platform = NULL,
		.backend = NULL,
		.dev_id = 0,
		.initialized = 1,
		.block_count = 0,
		.key = {},
		.write_counter = 0
	};


	expect_rpmb_frontend_block_count(&rpmb_frontend, 2048, PSA_SUCCESS);
	expect_rpmb_frontend_block_size(&rpmb_frontend, 256, PSA_SUCCESS);
	expect_calloc(NULL);
	mock_libc_enable();

	store = rpmb_block_store_init(&rpmb_block_store, &partition_guid, &rpmb_frontend, 512);
	CHECK(store == NULL);
	mock_libc_disable();
}

TEST(RpmbBlockStoreTests, GetPartitionInfo)
{
	struct storage_partition_info info = {0};
	struct uuid_octets wrong_uuid = {0};
	psa_status_t st = PSA_SUCCESS;

	mock_init_store();

	st = m_block_store->interface->get_partition_info(&m_rpmb_block_store, &m_null_guid, &info);
	CHECK_EQUAL(PSA_ERROR_INVALID_ARGUMENT, st);

	uuid_guid_octets_from_canonical(&m_disk_guid,
						"6152f22b-8128-4c1f-981f-3bd279519907");
	st = m_block_store->interface->get_partition_info(&m_rpmb_block_store, &wrong_uuid, &info);
	CHECK_EQUAL(PSA_ERROR_INVALID_ARGUMENT, st);

	st = m_block_store->interface->get_partition_info(&m_rpmb_block_store, &m_disk_guid, &info);
	CHECK_EQUAL(PSA_SUCCESS, st);

	CHECK_EQUAL(512, info.block_size);
	CHECK_EQUAL(2048/2, info.num_blocks);
	MEMCMP_EQUAL((const void *) &m_null_guid, (const void *) &info.parent_guid, sizeof(m_null_guid));
	MEMCMP_EQUAL((const void *) &m_disk_guid, (const void *) &info.partition_guid, sizeof(m_disk_guid));

	mock_deinit_store();
}

void fill_buffer(uint8_t* buf, size_t size)
{
        for (size_t i = 0; i < size; ++i) {
                buf[i] = (uint8_t) rand();
        }
}

TEST(RpmbBlockStoreTests, Read)
{
	psa_status_t st = PSA_SUCCESS;
	size_t n_read = 0;
	storage_partition_handle_t handle = 0;

	uint8_t buffer[512] = {0};
	uint8_t buffer1[512] = {0};

	mock_init_store();

	// Pass NULL context
	st = m_block_store->interface->read(0, 0, 0, 0, 0, sizeof(buffer), buffer, &n_read);
	CHECK_EQUAL(PSA_ERROR_INVALID_ARGUMENT, st);
	CHECK_EQUAL(0, n_read);

	// Pass 0 client ID
	st = m_block_store->interface->read(&m_rpmb_block_store, 0, 0, 0, 0, sizeof(buffer), buffer, &n_read);
	CHECK_EQUAL(PSA_ERROR_NOT_PERMITTED, st);
	CHECK_EQUAL(0, n_read);

	// Pass invalid device handle
	st = m_block_store->interface->read(&m_rpmb_block_store, 0, 1, 0, 0, sizeof(buffer), buffer, &n_read);
	CHECK_EQUAL(PSA_ERROR_INVALID_ARGUMENT, st);
	CHECK_EQUAL(0, n_read);

	// Offset > block size
	st = m_block_store->interface->read(&m_rpmb_block_store, 0, 0, 0, 513, sizeof(buffer), buffer, &n_read);
	CHECK_EQUAL(PSA_ERROR_INVALID_ARGUMENT, st);
	CHECK_EQUAL(0, n_read);

	st = m_block_store->interface->open(&m_rpmb_block_store, 22, &m_disk_guid, &handle);
	CHECK_EQUAL(PSA_SUCCESS, st);
	// Invalid handle
	st = m_block_store->interface->read(&m_rpmb_block_store, 22, handle+1, 0, 0, sizeof(buffer), buffer, &n_read);
	CHECK_EQUAL(PSA_ERROR_INVALID_ARGUMENT, st);
	CHECK_EQUAL(0, n_read);

	// Invalid client ID
	st = m_block_store->interface->read(&m_rpmb_block_store, 22+1, handle, 0, 0, sizeof(buffer), buffer, &n_read);
	CHECK_EQUAL(PSA_ERROR_NOT_PERMITTED, st);
	CHECK_EQUAL(0, n_read);

	// LBA overflow
	st = m_block_store->interface->read(&m_rpmb_block_store, 22, handle, -1lu, 0, sizeof(buffer), buffer, &n_read);
	CHECK_EQUAL(PSA_ERROR_GENERIC_ERROR, st);
	CHECK_EQUAL(0, n_read);

	// LBA out of range
	st = m_block_store->interface->read(&m_rpmb_block_store, 22, handle, (2048/2)+1, 0, sizeof(buffer), buffer, &n_read);
	CHECK_EQUAL(PSA_ERROR_INVALID_ARGUMENT, st);
	CHECK_EQUAL(0, n_read);

	// Backend returns read error
	expect_rpmb_frontend_read(m_rpmb_block_store.frontend, 0, buffer1, sizeof(buffer1), 2, PSA_ERROR_INVALID_ARGUMENT);
	st = m_block_store->interface->read(&m_rpmb_block_store, 22, handle, 0, 0, sizeof(buffer), buffer, &n_read);
	CHECK_EQUAL(PSA_ERROR_INVALID_ARGUMENT, st);
	CHECK_EQUAL(0, n_read);

	// Check offset
	uint64_t lba = 77;
	for ( size_t x = 0; x < 512; x++) {
		fill_buffer(buffer1, sizeof(buffer1));
		expect_rpmb_frontend_read(m_rpmb_block_store.frontend, lba * 2, buffer1, sizeof(buffer1), 2, PSA_SUCCESS);
		st = m_block_store->interface->read(&m_rpmb_block_store, 22, handle, lba, x, sizeof(buffer), buffer, &n_read);
		CHECK_EQUAL(PSA_SUCCESS, st);
		CHECK_EQUAL(sizeof(buffer) - x, n_read);
		MEMCMP_EQUAL(buffer1 + x, buffer, sizeof(buffer) - x);
	}

	mock_deinit_store();
}

TEST(RpmbBlockStoreTests, Write)
{
	psa_status_t st = PSA_SUCCESS;
	size_t n_written = 0;
	storage_partition_handle_t handle = 0;

	uint8_t buffer[512] = {0};
	uint8_t buffer1[512] = {0};

	mock_init_store();

	// Pass NULL context
	st = m_block_store->interface->write(0, 0, 0, 0, 0, buffer, sizeof(buffer), &n_written);
	CHECK_EQUAL(PSA_ERROR_INVALID_ARGUMENT, st);
	CHECK_EQUAL(0, n_written);

	// Pass 0 client ID
	st = m_block_store->interface->write(&m_rpmb_block_store, 0, 0, 0, 0, buffer, sizeof(buffer), &n_written);
	CHECK_EQUAL(PSA_ERROR_NOT_PERMITTED, st);
	CHECK_EQUAL(0, n_written);

	// Pass invalid device handle
	st = m_block_store->interface->write(&m_rpmb_block_store, 0, 1, 0, 0, buffer, sizeof(buffer), &n_written);
	CHECK_EQUAL(PSA_ERROR_INVALID_ARGUMENT, st);
	CHECK_EQUAL(0, n_written);

	// Offset > block size
	st = m_block_store->interface->write(&m_rpmb_block_store, 0, 0, 0, 513, buffer, sizeof(buffer), &n_written);
	CHECK_EQUAL(PSA_ERROR_INVALID_ARGUMENT, st);
	CHECK_EQUAL(0, n_written);

	st = m_block_store->interface->open(&m_rpmb_block_store, 22, &m_disk_guid, &handle);
	CHECK_EQUAL(PSA_SUCCESS, st);
	// Invalid handle
	st = m_block_store->interface->write(&m_rpmb_block_store, 22, handle+1, 0, 0, buffer, sizeof(buffer), &n_written);
	CHECK_EQUAL(PSA_ERROR_INVALID_ARGUMENT, st);
	CHECK_EQUAL(0, n_written);

	// Invalid client ID
	st = m_block_store->interface->write(&m_rpmb_block_store, 22+1, handle, 0, 0, buffer, sizeof(buffer), &n_written);
	CHECK_EQUAL(PSA_ERROR_NOT_PERMITTED, st);
	CHECK_EQUAL(0, n_written);

	// LBA overflow
	st = m_block_store->interface->write(&m_rpmb_block_store, 22, handle, -1lu, 0, buffer, sizeof(buffer), &n_written);
	CHECK_EQUAL(PSA_ERROR_GENERIC_ERROR, st);
	CHECK_EQUAL(0, n_written);

	// LBA out of range
	st = m_block_store->interface->write(&m_rpmb_block_store, 22, handle, (2048/2)+1, 0, buffer, sizeof(buffer), &n_written);
	CHECK_EQUAL(PSA_ERROR_INVALID_ARGUMENT, st);
	CHECK_EQUAL(0, n_written);

	// Backend returns write error
	expect_rpmb_frontend_write(m_rpmb_block_store.frontend, 0, buffer1, sizeof(buffer1), 2, PSA_ERROR_INVALID_ARGUMENT);
	st = m_block_store->interface->write(&m_rpmb_block_store, 22, handle, 0, 0, buffer, sizeof(buffer), &n_written);
	CHECK_EQUAL(PSA_ERROR_INVALID_ARGUMENT, st);
	CHECK_EQUAL(0, n_written);

	// Backend returns write error
	expect_rpmb_frontend_read(m_rpmb_block_store.frontend, 0, buffer1, sizeof(buffer1), 2, PSA_ERROR_INVALID_ARGUMENT);
	st = m_block_store->interface->write(&m_rpmb_block_store, 22, handle, 0, 1, buffer, sizeof(buffer), &n_written);
	CHECK_EQUAL(PSA_ERROR_INVALID_ARGUMENT, st);
	CHECK_EQUAL(0, n_written);


	// Check offset
	uint64_t lba = 77;
	for ( size_t x = 0; x < 512; x++) {
		fill_buffer(buffer1, sizeof(buffer1));
		memcpy(buffer1 + x, buffer, sizeof(buffer1) - x);
		if (x != 0)
			expect_rpmb_frontend_read(m_rpmb_block_store.frontend, lba * 2, buffer1, sizeof(buffer1), 2, PSA_SUCCESS);
		expect_rpmb_frontend_write(m_rpmb_block_store.frontend, lba * 2, buffer1, sizeof(buffer1), 2, PSA_SUCCESS);
		st = m_block_store->interface->write(&m_rpmb_block_store, 22, handle, lba, x, buffer, sizeof(buffer), &n_written);
		CHECK_EQUAL(PSA_SUCCESS, st);
		CHECK_EQUAL(sizeof(buffer) - x, n_written);
		MEMCMP_EQUAL(buffer1 + x, buffer, sizeof(buffer) - x);
	}

	mock_deinit_store();
}