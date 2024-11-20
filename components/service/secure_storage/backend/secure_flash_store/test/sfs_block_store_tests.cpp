/*
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <CppUTest/TestHarness.h>
#include <service/block_storage/config/ref/ref_partition_configurator.h>
#include <service/block_storage/factory/ref_ram/block_store_factory.h>
#include <service/crypto/client/psa/psa_crypto_client.h>
#include <service/secure_storage/backend/secure_flash_store/flash/block_store_adapter/sfs_flash_block_store_adapter.h>
#include <service/secure_storage/backend/secure_flash_store/secure_flash_store.h>
#include <service/secure_storage/frontend/psa/its/its_frontend.h>
#include <service/secure_storage/frontend/psa/its/test/its_api_tests.h>
#include <service/secure_storage/frontend/psa/ps/ps_frontend.h>
#include <service/secure_storage/frontend/psa/ps/test/ps_api_tests.h>
#include <service_locator.h>
/**
 * Tests the secure flash store with a block_store flash driver.
 */
TEST_GROUP(SfsBlockStoreTests)
{
	void setup()
	{
		psa_status_t status = PSA_ERROR_GENERIC_ERROR;
		struct uuid_octets guid;
		const struct sfs_flash_info_t *flash_info = NULL;

		/*
		 * If block-store is configured to be encrypted, crypto client
		 * is needed for accessing the crypto functions for encryption.
		 */
		m_rpc_session = NULL;
		m_crypto_service_context = NULL;

		service_locator_init();

		m_crypto_service_context = service_locator_query("sn:trustedfirmware.org:crypto:0");
		CHECK_TRUE(m_crypto_service_context);

		m_rpc_session = service_context_open(m_crypto_service_context);
		CHECK_TRUE(m_rpc_session);

		status = psa_crypto_client_init(m_rpc_session);
		LONGS_EQUAL(PSA_SUCCESS, status);

		uuid_guid_octets_from_canonical(&guid, REF_PARTITION_2_GUID);

		block_store = ref_ram_block_store_factory_create();
		CHECK_TRUE(block_store);

		status = sfs_flash_block_store_adapter_init(&sfs_flash_adapter, CLIENT_ID,
							    block_store, &guid,
							    MIN_FLASH_BLOCK_SIZE, MAX_NUM_FILES,
							    &flash_info);

		LONGS_EQUAL(PSA_SUCCESS, status);
		CHECK_TRUE(flash_info);

		struct storage_backend *storage_backend = sfs_init(flash_info);
		CHECK_TRUE(storage_backend);

		psa_its_frontend_init(storage_backend);
		psa_ps_frontend_init(storage_backend);
	}

	void teardown()
	{
		sfs_flash_block_store_adapter_deinit(&sfs_flash_adapter);
		ref_ram_block_store_factory_destroy(block_store);

		block_store = NULL;

		if (m_crypto_service_context) {
			if (m_rpc_session) {
				service_context_close(m_crypto_service_context, m_rpc_session);
				m_rpc_session = NULL;
			}

			service_context_relinquish(m_crypto_service_context);
			m_crypto_service_context = NULL;
		}
	}

	static const uint32_t CLIENT_ID = 10;
	static const size_t MAX_NUM_FILES = 10;
	static const size_t MIN_FLASH_BLOCK_SIZE = 4096;

	struct block_store *block_store;
	struct sfs_flash_block_store_adapter sfs_flash_adapter;

	struct rpc_caller_session *m_rpc_session;
	struct service_context *m_crypto_service_context;
};

TEST(SfsBlockStoreTests, itsStoreNewItem)
{
	its_api_tests::storeNewItem();
}

TEST(SfsBlockStoreTests, itsStorageLimitTest)
{
	its_api_tests::storageLimitTest(5000);
}

TEST(SfsBlockStoreTests, psSet)
{
	ps_api_tests::set();
}

TEST(SfsBlockStoreTests, psCreateAndSetExtended)
{
	ps_api_tests::createAndSetExtended();
}
