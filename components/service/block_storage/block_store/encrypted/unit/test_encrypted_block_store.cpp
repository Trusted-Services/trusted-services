/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <string.h>

#include "encrypted_block_store.h"
#include "mock_block_store.h"
#include "mock_crypto.h"
#include "mock_libc.h"
#include "unit_test_utils.h"

TEST_GROUP(EncryptedBlockStoreUnitTests)
{
	TEST_SETUP()
	{
		m_back_store = mock_block_store_init(&m_mock_store, &m_partition_guid, NUM_BLOCKS, BLOCK_SIZE);
	}

	TEST_TEARDOWN()
	{
		mock().checkExpectations();
		mock().removeAllComparatorsAndCopiers();
		mock().clear();
	}

	void set_expect_for_importing_root_key(void)
	{
		MOCK_IGNORE(psa_import_key);
		MOCK_IGNORE(psa_reset_key_attributes);
	}

	void set_expect_key_derivation(size_t number_of_keys)
	{
		MOCK_IGNORE_NCALL(psa_key_derivation_setup, number_of_keys);
		MOCK_IGNORE_NCALL(psa_key_derivation_input_bytes, number_of_keys);
		MOCK_IGNORE_NCALL(psa_key_derivation_input_key, number_of_keys);
		MOCK_IGNORE_NCALL(psa_key_derivation_input_bytes, number_of_keys);
		MOCK_IGNORE_NCALL(psa_key_derivation_output_key, number_of_keys);
		MOCK_IGNORE_NCALL(psa_key_derivation_abort, number_of_keys);
		MOCK_IGNORE_NCALL(psa_reset_key_attributes, number_of_keys);
	}

	void mock_init_store(void)
	{
		psa_status_t status = PSA_SUCCESS;

		expect_psa_crypto_init(PSA_SUCCESS);
		set_expect_for_importing_root_key();
		set_expect_key_derivation(2);
		MOCK_IGNORE(psa_destroy_key);
		expect_block_store_get_partition_info(m_back_store, &m_partition_guid, &info,
						      PSA_SUCCESS);
		MOCK_IGNORE(block_store_open);

		m_block_store = encrypted_block_store_init(&m_encrypted_store, CLIENT_ID,
							   &m_partition_guid, m_back_store);
		CHECK(m_block_store != NULL);

		status = block_store_open(m_encrypted_store.back_store, CLIENT_ID,
					  &m_partition_guid, &m_encrypted_store.back_store_handle);
		LONGS_EQUAL(PSA_SUCCESS, status);
	}

	void mock_deinit_store(void)
	{
		mock().disable();
		encrypted_block_store_deinit(&m_encrypted_store);
		mock().enable();
	}

	/* Ignores specific calls in encrypt_block function and then returns */
	void mock_ignore_encrypt_block(uint8_t stage = 255)
	{
		MOCK_TILL_STAGE(stage, 1, "psa_cipher_encrypt_setup", .ignoreOtherParameters());
		MOCK_TILL_STAGE(stage, 2, "psa_cipher_update",
			.withOutputParameterReturning("output_length", &BLK_AES_BLOCK_SIZE,
						       sizeof(BLK_AES_BLOCK_SIZE))
			.ignoreOtherParameters());
		/*
		 * Encryption is always called with multiples of AES blocks, which will always be.
		 * Full processed by the update call. Finish shall always provide 0 data length.
		 */
		MOCK_TILL_STAGE(stage, 3, "psa_cipher_finish",
			.withOutputParameterReturning("output_length", &MOCK_NULL_SIZE,
						       sizeof(MOCK_NULL_SIZE))
			.ignoreOtherParameters());
		MOCK_TILL_STAGE(stage, 4, "psa_cipher_encrypt_setup", .ignoreOtherParameters());
		MOCK_TILL_STAGE(stage, 5, "psa_cipher_set_iv", .ignoreOtherParameters());
		MOCK_TILL_STAGE(stage, 6, "psa_cipher_update",
			.withOutputParameterReturning("output_length", &BLOCK_SIZE,
						       sizeof(BLOCK_SIZE))
			.ignoreOtherParameters());
		MOCK_TILL_STAGE(stage, 7, "psa_cipher_finish",
			.withOutputParameterReturning("output_length", &MOCK_NULL_SIZE,
						       sizeof(MOCK_NULL_SIZE))
			.ignoreOtherParameters());
	}

	/* Ignores specific calls in decrypt_block function and then returns */
	void mock_ignore_decrypt_block(uint8_t stage = 255)
	{
		MOCK_TILL_STAGE(stage, 1, "psa_cipher_encrypt_setup", .ignoreOtherParameters());
		MOCK_TILL_STAGE(stage, 2, "psa_cipher_update",
				.withOutputParameterReturning("output_length", &BLK_AES_BLOCK_SIZE,
							      sizeof(BLK_AES_BLOCK_SIZE))
					.ignoreOtherParameters());
		/*
		 * Decryption is always called with multiples of AES blocks, which will always be.
		 * Full processed by the update call. Finish shall always provide 0 data length.
		 */
		MOCK_TILL_STAGE(stage, 3, "psa_cipher_finish",
			.withOutputParameterReturning("output_length", &MOCK_NULL_SIZE,
						       sizeof(MOCK_NULL_SIZE))
			.ignoreOtherParameters());
		MOCK_TILL_STAGE(stage, 4, "psa_cipher_decrypt_setup", .ignoreOtherParameters());
		MOCK_TILL_STAGE(stage, 5, "psa_cipher_set_iv", .ignoreOtherParameters());
		MOCK_TILL_STAGE(stage, 6, "psa_cipher_update",
			.withOutputParameterReturning("output_length", &BLOCK_SIZE,
						       sizeof(BLOCK_SIZE))
			.ignoreOtherParameters());
		MOCK_TILL_STAGE(stage, 7, "psa_cipher_finish",
			.withOutputParameterReturning("output_length", &MOCK_NULL_SIZE,
						       sizeof(MOCK_NULL_SIZE))
			.ignoreOtherParameters());
	}

	struct mock_block_store m_mock_store;
	struct block_store *m_back_store;
	struct block_store *m_block_store;
	struct encrypted_block_store m_encrypted_store;
	const uint32_t CLIENT_ID = 27;
	const size_t BLOCK_SIZE = 512;
	struct uuid_octets m_partition_guid = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
	};
	storage_partition_handle_t m_handle = 1;
	const uint64_t LBA = 2;
	const size_t NUM_BLOCKS = 3;
	psa_status_t m_status = PSA_SUCCESS;
	struct storage_partition_info info = { NUM_BLOCKS, BLOCK_SIZE, { 0 }, { 0 } };
	size_t BLK_AES_BLOCK_SIZE = 16;
	size_t MOCK_NULL_SIZE = 0;
};

TEST(EncryptedBlockStoreUnitTests, block_store_init_failure__null_arguments)
{
	m_block_store =
		encrypted_block_store_init(NULL, CLIENT_ID, &m_partition_guid, m_back_store);
	UNSIGNED_LONGLONGS_EQUAL(m_block_store, NULL);

	m_block_store =
		encrypted_block_store_init(&m_encrypted_store, CLIENT_ID, NULL, m_back_store);
	UNSIGNED_LONGLONGS_EQUAL(m_block_store, NULL);

	m_block_store =
		encrypted_block_store_init(&m_encrypted_store, CLIENT_ID, &m_partition_guid, NULL);
	UNSIGNED_LONGLONGS_EQUAL(m_block_store, NULL);
}

TEST(EncryptedBlockStoreUnitTests, block_store_init_failure__psa_crypto_init)
{
	expect_psa_crypto_init(PSA_ERROR_GENERIC_ERROR);

	m_block_store = encrypted_block_store_init(&m_encrypted_store, CLIENT_ID, &m_partition_guid,
						   m_back_store);
	UNSIGNED_LONGLONGS_EQUAL(m_block_store, NULL);
}

TEST(EncryptedBlockStoreUnitTests, block_store_init_failure__psa_import_key)
{
	expect_psa_crypto_init(PSA_SUCCESS);

	MOCK_IGNORE(psa_reset_key_attributes);
	MOCK_RETVAL_ONLY(psa_import_key, PSA_ERROR_GENERIC_ERROR);

	m_block_store = encrypted_block_store_init(&m_encrypted_store, CLIENT_ID, &m_partition_guid,
						   m_back_store);
	UNSIGNED_LONGLONGS_EQUAL(m_block_store, NULL);
}

TEST(EncryptedBlockStoreUnitTests, block_store_init_failure_decrypt_key__psa_key_derivation_setup)
{
	expect_psa_crypto_init(PSA_SUCCESS);
	set_expect_for_importing_root_key();

	/* First derivation (encrypt key) fails */
	MOCK_RETVAL_ONLY(psa_key_derivation_setup, PSA_ERROR_GENERIC_ERROR);
	MOCK_IGNORE(psa_key_derivation_abort);
	MOCK_IGNORE(psa_reset_key_attributes);

	/* Root key is destroyed */
	MOCK_IGNORE(psa_destroy_key);

	m_block_store = encrypted_block_store_init(&m_encrypted_store, CLIENT_ID, &m_partition_guid,
						   m_back_store);
	UNSIGNED_LONGLONGS_EQUAL(m_block_store, NULL);
}

TEST(EncryptedBlockStoreUnitTests, block_store_init_failure__inbuffer_allocation)
{
	expect_psa_crypto_init(PSA_SUCCESS);
	set_expect_for_importing_root_key();
	set_expect_key_derivation(2);
	MOCK_IGNORE(psa_destroy_key);
	expect_block_store_get_partition_info(m_back_store, &m_partition_guid, &info, PSA_SUCCESS);

	mock_libc_enable();
	expect_calloc(NULL);

	m_block_store = encrypted_block_store_init(&m_encrypted_store, CLIENT_ID, &m_partition_guid,
						   m_back_store);

	mock_libc_disable();

	CHECK(m_block_store == NULL);
}

TEST(EncryptedBlockStoreUnitTests, block_store_init_failure__outbuffer_allocation)
{
	uint8_t buffer[1] = { 0 };

	expect_psa_crypto_init(PSA_SUCCESS);
	set_expect_for_importing_root_key();
	set_expect_key_derivation(2);
	MOCK_IGNORE(psa_destroy_key);
	expect_block_store_get_partition_info(m_back_store, &m_partition_guid, &info, PSA_SUCCESS);

	mock_libc_enable();
	expect_calloc(&buffer);
	expect_calloc(NULL);

	m_block_store = encrypted_block_store_init(&m_encrypted_store, CLIENT_ID, &m_partition_guid,
						   m_back_store);

	mock_libc_disable();

	CHECK(m_block_store == NULL);
}

TEST(EncryptedBlockStoreUnitTests,
     block_store_init_failure_decrypt_key__psa_key_derivation_input_bytes_salt)
{
	expect_psa_crypto_init(PSA_SUCCESS);
	set_expect_for_importing_root_key();

	/* First derivation (encrypt key) fails */
	MOCK_IGNORE(psa_key_derivation_setup);
	MOCK_RETVAL_ONLY(psa_key_derivation_input_bytes, PSA_ERROR_GENERIC_ERROR);
	MOCK_IGNORE(psa_key_derivation_abort);
	MOCK_IGNORE(psa_reset_key_attributes);

	/* Root key is destroyed */
	MOCK_IGNORE(psa_destroy_key);

	m_block_store = encrypted_block_store_init(&m_encrypted_store, CLIENT_ID, &m_partition_guid,
						   m_back_store);
	UNSIGNED_LONGLONGS_EQUAL(m_block_store, NULL);
}

TEST(EncryptedBlockStoreUnitTests,
     block_store_init_failure_decrypt_key__psa_key_derivation_input_key)
{
	expect_psa_crypto_init(PSA_SUCCESS);
	set_expect_for_importing_root_key();

	/* First derivation (encrypt key) fails */
	MOCK_IGNORE(psa_key_derivation_setup);
	MOCK_IGNORE(psa_key_derivation_input_bytes);
	MOCK_RETVAL_ONLY(psa_key_derivation_input_key, PSA_ERROR_GENERIC_ERROR);
	MOCK_IGNORE(psa_key_derivation_abort);
	MOCK_IGNORE(psa_reset_key_attributes);

	/* Root key is destroyed */
	MOCK_IGNORE(psa_destroy_key);

	m_block_store = encrypted_block_store_init(&m_encrypted_store, CLIENT_ID, &m_partition_guid,
						   m_back_store);
	UNSIGNED_LONGLONGS_EQUAL(m_block_store, NULL);
}

TEST(EncryptedBlockStoreUnitTests,
     block_store_init_failure_decrypt_key__psa_key_derivation_input_bytes_info)
{
	expect_psa_crypto_init(PSA_SUCCESS);
	set_expect_for_importing_root_key();

	/* First derivation (encrypt key) fails */
	MOCK_IGNORE(psa_key_derivation_setup);
	MOCK_IGNORE(psa_key_derivation_input_bytes);
	MOCK_IGNORE(psa_key_derivation_input_key);
	MOCK_RETVAL_ONLY(psa_key_derivation_input_bytes, PSA_ERROR_GENERIC_ERROR);
	MOCK_IGNORE(psa_key_derivation_abort);
	MOCK_IGNORE(psa_reset_key_attributes);

	/* Root key is destroyed */
	MOCK_IGNORE(psa_destroy_key);

	m_block_store = encrypted_block_store_init(&m_encrypted_store, CLIENT_ID, &m_partition_guid,
						   m_back_store);
	UNSIGNED_LONGLONGS_EQUAL(m_block_store, NULL);
}

TEST(EncryptedBlockStoreUnitTests,
     block_store_init_failure_decrypt_key__psa_key_derivation_output_key)
{
	expect_psa_crypto_init(PSA_SUCCESS);
	set_expect_for_importing_root_key();

	/* First derivation (encrypt key) fails */
	MOCK_IGNORE(psa_key_derivation_setup);
	MOCK_IGNORE(psa_key_derivation_input_bytes);
	MOCK_IGNORE(psa_key_derivation_input_key);
	MOCK_IGNORE(psa_key_derivation_input_bytes);
	MOCK_RETVAL_ONLY(psa_key_derivation_output_key, PSA_ERROR_GENERIC_ERROR);
	MOCK_IGNORE(psa_key_derivation_abort);
	MOCK_IGNORE(psa_reset_key_attributes);

	/* Root key is destroyed */
	MOCK_IGNORE(psa_destroy_key);

	m_block_store = encrypted_block_store_init(&m_encrypted_store, CLIENT_ID, &m_partition_guid,
						   m_back_store);
	UNSIGNED_LONGLONGS_EQUAL(m_block_store, NULL);
}

TEST(EncryptedBlockStoreUnitTests, block_store_init_failure_essiv_key__psa_key_derivation_setup)
{
	expect_psa_crypto_init(PSA_SUCCESS);
	set_expect_for_importing_root_key();
	set_expect_key_derivation(1);

	/* Second derivation (essiv key) fails */
	MOCK_RETVAL_ONLY(psa_key_derivation_setup, PSA_ERROR_GENERIC_ERROR);
	MOCK_IGNORE(psa_key_derivation_abort);
	MOCK_IGNORE(psa_reset_key_attributes);

	/* Root key is destroyed */
	MOCK_IGNORE(psa_destroy_key);

	m_block_store = encrypted_block_store_init(&m_encrypted_store, CLIENT_ID, &m_partition_guid,
						   m_back_store);
	UNSIGNED_LONGLONGS_EQUAL(m_block_store, NULL);
}

TEST(EncryptedBlockStoreUnitTests,
     block_store_init_failure_essiv_key__psa_key_derivation_input_bytes_salt)
{
	expect_psa_crypto_init(PSA_SUCCESS);
	set_expect_for_importing_root_key();
	set_expect_key_derivation(1);

	/* Second derivation (essiv key) fails */
	MOCK_IGNORE(psa_key_derivation_setup);
	MOCK_RETVAL_ONLY(psa_key_derivation_input_bytes, PSA_ERROR_GENERIC_ERROR);
	MOCK_IGNORE(psa_key_derivation_abort);
	MOCK_IGNORE(psa_reset_key_attributes);

	/* Root key is destroyed */
	MOCK_IGNORE(psa_destroy_key);

	m_block_store = encrypted_block_store_init(&m_encrypted_store, CLIENT_ID, &m_partition_guid,
						   m_back_store);
	UNSIGNED_LONGLONGS_EQUAL(m_block_store, NULL);
}

TEST(EncryptedBlockStoreUnitTests, block_store_init_failure_essiv_key__psa_key_derivation_input_key)
{
	expect_psa_crypto_init(PSA_SUCCESS);
	set_expect_for_importing_root_key();
	set_expect_key_derivation(1);

	/* Second derivation (essiv key) fails */
	MOCK_IGNORE(psa_key_derivation_setup);
	MOCK_IGNORE(psa_key_derivation_input_bytes);
	MOCK_RETVAL_ONLY(psa_key_derivation_input_key, PSA_ERROR_GENERIC_ERROR);
	MOCK_IGNORE(psa_key_derivation_abort);
	MOCK_IGNORE(psa_reset_key_attributes);

	/* Root key is destroyed */
	MOCK_IGNORE(psa_destroy_key);

	m_block_store = encrypted_block_store_init(&m_encrypted_store, CLIENT_ID, &m_partition_guid,
						   m_back_store);
	UNSIGNED_LONGLONGS_EQUAL(m_block_store, NULL);
}

TEST(EncryptedBlockStoreUnitTests,
     block_store_init_failure_essiv_key__psa_key_derivation_input_bytes_info)
{
	expect_psa_crypto_init(PSA_SUCCESS);
	set_expect_for_importing_root_key();
	set_expect_key_derivation(1);

	/* Second derivation (essiv key) fails */
	MOCK_IGNORE(psa_key_derivation_setup);
	MOCK_IGNORE(psa_key_derivation_input_bytes);
	MOCK_IGNORE(psa_key_derivation_input_key);
	MOCK_RETVAL_ONLY(psa_key_derivation_input_bytes, PSA_ERROR_GENERIC_ERROR);
	MOCK_IGNORE(psa_key_derivation_abort);
	MOCK_IGNORE(psa_reset_key_attributes);

	/* Root key is destroyed */
	MOCK_IGNORE(psa_destroy_key);

	m_block_store = encrypted_block_store_init(&m_encrypted_store, CLIENT_ID, &m_partition_guid,
						   m_back_store);
	UNSIGNED_LONGLONGS_EQUAL(m_block_store, NULL);
}

TEST(EncryptedBlockStoreUnitTests,
     block_store_init_failure_essiv_key__psa_key_derivation_output_key)
{
	expect_psa_crypto_init(PSA_SUCCESS);
	set_expect_for_importing_root_key();
	set_expect_key_derivation(1);

	/* Second derivation (essiv key) fails */
	MOCK_IGNORE(psa_key_derivation_setup);
	MOCK_IGNORE(psa_key_derivation_input_bytes);
	MOCK_IGNORE(psa_key_derivation_input_key);
	MOCK_IGNORE(psa_key_derivation_input_bytes);
	MOCK_RETVAL_ONLY(psa_key_derivation_output_key, PSA_ERROR_GENERIC_ERROR);
	MOCK_IGNORE(psa_key_derivation_abort);
	MOCK_IGNORE(psa_reset_key_attributes);

	/* Root key is destroyed */
	MOCK_IGNORE(psa_destroy_key);

	m_block_store = encrypted_block_store_init(&m_encrypted_store, CLIENT_ID, &m_partition_guid,
						   m_back_store);
	UNSIGNED_LONGLONGS_EQUAL(m_block_store, NULL);
}

TEST(EncryptedBlockStoreUnitTests,
     block_store_init_failure_essiv_key__block_store_get_partition_info)
{
	expect_psa_crypto_init(PSA_SUCCESS);
	set_expect_for_importing_root_key();
	set_expect_key_derivation(2);

	/* Root key is destroyed */
	MOCK_IGNORE(psa_destroy_key);

	MOCK_RETVAL_ONLY(block_store_get_partition_info, PSA_ERROR_GENERIC_ERROR);

	m_block_store = encrypted_block_store_init(&m_encrypted_store, CLIENT_ID, &m_partition_guid,
						   m_back_store);
	UNSIGNED_LONGLONGS_EQUAL(m_block_store, NULL);
}

TEST(EncryptedBlockStoreUnitTests, block_store_init_failure_essiv_key__block_store_open)
{
	psa_status_t status = PSA_SUCCESS;

	expect_psa_crypto_init(PSA_SUCCESS);
	set_expect_for_importing_root_key();
	set_expect_key_derivation(2);

	/* Root key is destroyed */
	MOCK_IGNORE(psa_destroy_key);

	MOCK_IGNORE(block_store_get_partition_info);

	MOCK_RETVAL_ONLY(block_store_open, PSA_ERROR_GENERIC_ERROR);

	m_block_store = encrypted_block_store_init(&m_encrypted_store, CLIENT_ID, &m_partition_guid,
						   m_back_store);
	CHECK(m_block_store != NULL);

	status = block_store_open(m_encrypted_store.back_store, CLIENT_ID, &m_partition_guid,
				  &m_encrypted_store.back_store_handle);
	LONGS_EQUAL(PSA_ERROR_GENERIC_ERROR, status);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests, block_store_deinit)
{
	mock_init_store();

	MOCK_IGNORE(block_store_close);

	/* Both derived keys shall be destroyed */
	MOCK_IGNORE_NCALL(psa_destroy_key, 2);

	encrypted_block_store_deinit(&m_encrypted_store);
}

TEST(EncryptedBlockStoreUnitTests, block_store_open)
{
	mock_init_store();

	/* Verify of block_store_open called through m_block_store calls the relevant API of the back store */
	expect_block_store_open(m_encrypted_store.back_store, CLIENT_ID, &m_partition_guid,
				&m_handle, PSA_ERROR_GENERIC_ERROR);
	m_status = block_store_open(m_block_store, CLIENT_ID, &m_partition_guid,
						  &m_handle);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests, block_store_close)
{
	mock_init_store();

	/* Verify of block_store_close called through m_block_store calls the relevant API of the back store */
	expect_block_store_close(m_encrypted_store.back_store, CLIENT_ID, m_handle,
				 PSA_ERROR_GENERIC_ERROR);
	m_status = block_store_close(m_block_store, CLIENT_ID, m_handle);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests, block_store_erase)
{
	mock_init_store();

	/* Verify of block_store_erase called through m_block_store calls the relevant API of the back store */
	expect_block_store_erase(m_encrypted_store.back_store, CLIENT_ID, m_handle, LBA, NUM_BLOCKS,
				 PSA_ERROR_GENERIC_ERROR);
	m_status = block_store_erase(m_block_store, CLIENT_ID, m_handle, LBA,
						   NUM_BLOCKS);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests, block_store_read_failure__block_store_read)
{
	size_t data_len = 0;

	mock_init_store();

	/*
	 * As a reasult of AES CBC whole block needs to be read or written, even if only a small part is requested.
	 * buffer argument is irrelevant, because the data is read into a local temporary buffer before decryption.
	 */
	mock().expectOneCall("block_store_read")
		.withUnsignedIntParameter("client_id", CLIENT_ID)
		.withUnsignedIntParameter("handle", m_handle)
		.withUnsignedIntParameter("lba", LBA)
		.withUnsignedIntParameter("offset", 0)
		.withUnsignedIntParameter("buffer_size", BLOCK_SIZE)
		.withOutputParameterReturning("data_len", &data_len, sizeof(data_len))
		.andReturnValue(PSA_ERROR_GENERIC_ERROR)
		.ignoreOtherParameters();

	m_status = block_store_read(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						  BLOCK_SIZE / 2, NULL, &data_len);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests, block_store_read_failure__partial_read)
{
	size_t data_len = BLOCK_SIZE - 1;

	mock_init_store();

	/* Only some part of the requested data is provided by the store */
	mock().expectOneCall("block_store_read")
		.withUnsignedIntParameter("client_id", CLIENT_ID)
		.withUnsignedIntParameter("handle", m_handle)
		.withUnsignedIntParameter("lba", LBA)
		.withUnsignedIntParameter("offset", 0)
		.withUnsignedIntParameter("buffer_size", BLOCK_SIZE)
		.withOutputParameterReturning("data_len", &data_len, sizeof(data_len))
		.andReturnValue(PSA_SUCCESS)
		.ignoreOtherParameters();

	m_status = block_store_read(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						  BLOCK_SIZE / 2, NULL, &data_len);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_INSUFFICIENT_DATA);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests, block_store_read_failure_calc_essiv__psa_cipher_encrypt_setup)
{
	size_t data_len = BLOCK_SIZE;

	mock_init_store();

	MOCK_OUTPUT_PARAMETER_ONLY(block_store_read, data_len, data_len);

	MOCK_UINT_PARAMETER_RETVAL(psa_cipher_encrypt_setup, alg, PSA_ALG_ECB_NO_PADDING, PSA_ERROR_GENERIC_ERROR);

	m_status = block_store_read(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						  BLOCK_SIZE, NULL, &data_len);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests, block_store_read_failure_calc_essiv__psa_cipher_update)
{
	size_t data_len = BLOCK_SIZE;

	mock_init_store();

	MOCK_OUTPUT_PARAMETER_ONLY(block_store_read, data_len, data_len);

	mock_ignore_decrypt_block(1);

	MOCK_RETVAL_ONLY(psa_cipher_update, PSA_ERROR_GENERIC_ERROR);

	MOCK_IGNORE(psa_cipher_abort);

	m_status = block_store_read(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						  BLOCK_SIZE, NULL, &data_len);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests,
     block_store_read_failure_calc_essiv__psa_cipher_finish_return_error)
{
	size_t data_len = BLOCK_SIZE;

	mock_init_store();

	MOCK_OUTPUT_PARAMETER_ONLY(block_store_read, data_len, data_len);

	mock_ignore_decrypt_block(2);

	MOCK_RETVAL_ONLY(psa_cipher_finish, PSA_ERROR_GENERIC_ERROR);

	MOCK_IGNORE(psa_cipher_abort);

	m_status = block_store_read(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						  BLOCK_SIZE, NULL, &data_len);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests,
     block_store_read_failure_calc_essiv__psa_cipher_finish_length_error)
{
	size_t data_len = BLOCK_SIZE;

	mock_init_store();

	MOCK_OUTPUT_PARAMETER_ONLY(block_store_read, data_len, data_len);

	mock_ignore_decrypt_block(2);

	MOCK_OUTPUT_PARAMETER_ONLY(psa_cipher_finish, output_length, BLK_AES_BLOCK_SIZE);

	MOCK_IGNORE(psa_cipher_abort);

	m_status = block_store_read(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						  BLOCK_SIZE, NULL, &data_len);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests, block_store_read_failure__psa_cipher_decrypt_setup)
{
	size_t data_len = BLOCK_SIZE;

	mock_init_store();

	MOCK_OUTPUT_PARAMETER_ONLY(block_store_read, data_len, data_len);

	mock_ignore_decrypt_block(3);

	/* Decrypt data read */
	MOCK_RETVAL_ONLY(psa_cipher_decrypt_setup, PSA_ERROR_GENERIC_ERROR);

	m_status = block_store_read(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						  BLOCK_SIZE, NULL, &data_len);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests, block_store_read_failure__psa_cipher_set_iv)
{
	size_t data_len = BLOCK_SIZE;

	mock_init_store();

	MOCK_OUTPUT_PARAMETER_ONLY(block_store_read, data_len, data_len);

	mock_ignore_decrypt_block(4);

	MOCK_RETVAL_ONLY(psa_cipher_set_iv, PSA_ERROR_GENERIC_ERROR);

	MOCK_IGNORE(psa_cipher_abort);

	m_status = block_store_read(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						  BLOCK_SIZE, NULL, &data_len);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests, block_store_read_failure__psa_cipher_update)
{
	size_t data_len = BLOCK_SIZE;

	mock_init_store();

	MOCK_OUTPUT_PARAMETER_ONLY(block_store_read, data_len, data_len);

	mock_ignore_decrypt_block(5);

	MOCK_RETVAL_ONLY(psa_cipher_update, PSA_ERROR_GENERIC_ERROR);

	MOCK_IGNORE(psa_cipher_abort);

	m_status = block_store_read(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						  BLOCK_SIZE, NULL, &data_len);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests, block_store_read_failure__psa_cipher_finish_return_error)
{
	size_t data_len = BLOCK_SIZE;

	mock_init_store();

	MOCK_OUTPUT_PARAMETER_ONLY(block_store_read, data_len, data_len);

	mock_ignore_decrypt_block(6);

	MOCK_RETVAL_ONLY(psa_cipher_finish, PSA_ERROR_GENERIC_ERROR);

	MOCK_IGNORE(psa_cipher_abort);

	m_status = block_store_read(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						  BLOCK_SIZE, NULL, &data_len);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests, block_store_read_failure__psa_cipher_finish_length_error)
{
	size_t data_len = BLOCK_SIZE;

	mock_init_store();

	MOCK_OUTPUT_PARAMETER_ONLY(block_store_read, data_len, data_len);

	mock_ignore_decrypt_block(6);

	MOCK_OUTPUT_PARAMETER_ONLY(psa_cipher_finish, output_length, BLK_AES_BLOCK_SIZE);

	MOCK_IGNORE(psa_cipher_abort);

	m_status = block_store_read(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						  BLOCK_SIZE, NULL, &data_len);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests, block_store_write_failure__block_store_read)
{
	size_t num_written = BLOCK_SIZE;
	size_t data_len = BLOCK_SIZE;
	uint8_t data[BLOCK_SIZE] = { 0 };

	mock_init_store();

	MOCK_OUTPUT_PARAMETER_RETVAL(block_store_read, data_len, data_len, PSA_ERROR_GENERIC_ERROR);

	m_status = block_store_write(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						   data, BLOCK_SIZE, &num_written);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests, block_store_write_failure__partial_read)
{
	size_t num_written = BLOCK_SIZE;
	size_t data_len = BLOCK_SIZE - 1;
	uint8_t data[BLOCK_SIZE] = { 0 };

	mock_init_store();

	/* Only some part of the requested data is provided by the store */
	MOCK_OUTPUT_PARAMETER_RETVAL(block_store_read, data_len, data_len, PSA_SUCCESS);

	m_status = block_store_write(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						   data, BLOCK_SIZE, &num_written);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_INSUFFICIENT_DATA);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests, block_store_write_failure_calc_essiv__psa_cipher_encrypt_setup)
{
	size_t num_written = BLOCK_SIZE;
	size_t data_len = BLOCK_SIZE;
	uint8_t data[BLOCK_SIZE] = { 0 };

	mock_init_store();

	MOCK_OUTPUT_PARAMETER_ONLY(block_store_read, data_len, data_len);

	mock_ignore_decrypt_block();

	MOCK_UINT_PARAMETER_RETVAL(psa_cipher_encrypt_setup, alg, PSA_ALG_ECB_NO_PADDING, PSA_ERROR_GENERIC_ERROR);

	m_status = block_store_write(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						   data, BLOCK_SIZE, &num_written);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests, block_store_write_failure_calc_essiv__psa_cipher_update)
{
	size_t num_written = BLOCK_SIZE;
	size_t data_len = BLOCK_SIZE;
	uint8_t data[BLOCK_SIZE] = { 0 };

	mock_init_store();

	MOCK_OUTPUT_PARAMETER_ONLY(block_store_read, data_len, data_len);

	mock_ignore_decrypt_block();
	mock_ignore_encrypt_block(1);

	MOCK_RETVAL_ONLY(psa_cipher_update, PSA_ERROR_GENERIC_ERROR);

	MOCK_IGNORE(psa_cipher_abort);

	m_status = block_store_write(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						   data, BLOCK_SIZE, &num_written);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests,
     block_store_write_failure_calc_essiv__psa_cipher_finish_return_error)
{
	size_t num_written = BLOCK_SIZE;
	size_t data_len = BLOCK_SIZE;
	uint8_t data[BLOCK_SIZE] = { 0 };

	mock_init_store();

	MOCK_OUTPUT_PARAMETER_ONLY(block_store_read, data_len, data_len);

	mock_ignore_decrypt_block();
	mock_ignore_encrypt_block(2);

	MOCK_RETVAL_ONLY(psa_cipher_finish, PSA_ERROR_GENERIC_ERROR);

	MOCK_IGNORE(psa_cipher_abort);

	m_status = block_store_write(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						   data, BLOCK_SIZE, &num_written);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests,
     block_store_write_failure_calc_essiv__psa_cipher_finish_length_error)
{
	size_t num_written = BLOCK_SIZE;
	size_t data_len = BLOCK_SIZE;
	uint8_t data[BLOCK_SIZE] = { 0 };

	mock_init_store();

	MOCK_OUTPUT_PARAMETER_ONLY(block_store_read, data_len, data_len);

	mock_ignore_decrypt_block();
	mock_ignore_encrypt_block(2);

	MOCK_OUTPUT_PARAMETER_ONLY(psa_cipher_finish, output_length, BLK_AES_BLOCK_SIZE);

	MOCK_IGNORE(psa_cipher_abort);

	m_status = block_store_write(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						   data, BLOCK_SIZE, &num_written);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests, block_store_write_failure__psa_cipher_encrypt_setup)
{
	size_t num_written = BLOCK_SIZE;
	size_t data_len = BLOCK_SIZE;
	uint8_t data[BLOCK_SIZE] = { 0 };

	mock_init_store();

	MOCK_OUTPUT_PARAMETER_ONLY(block_store_read, data_len, data_len);

	mock_ignore_decrypt_block();
	mock_ignore_encrypt_block(3);

	mock().expectOneCall("psa_cipher_encrypt_setup")
		.withUnsignedIntParameter("alg", PSA_ALG_CBC_NO_PADDING)
		.andReturnValue(PSA_ERROR_GENERIC_ERROR)
		.ignoreOtherParameters();

	m_status = block_store_write(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						   data, BLOCK_SIZE, &num_written);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests, block_store_write_failure__psa_cipher_set_iv)
{
	size_t num_written = BLOCK_SIZE;
	size_t data_len = BLOCK_SIZE;
	uint8_t data[BLOCK_SIZE] = { 0 };

	mock_init_store();

	MOCK_OUTPUT_PARAMETER_ONLY(block_store_read, data_len, data_len);

	mock_ignore_decrypt_block();
	mock_ignore_encrypt_block(4);

	MOCK_RETVAL_ONLY(psa_cipher_set_iv, PSA_ERROR_GENERIC_ERROR);

	MOCK_IGNORE(psa_cipher_abort);

	m_status = block_store_write(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						   data, BLOCK_SIZE, &num_written);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests, block_store_write_failure__psa_cipher_update)
{
	size_t num_written = BLOCK_SIZE;
	size_t data_len = BLOCK_SIZE;
	uint8_t data[BLOCK_SIZE] = { 0 };

	mock_init_store();

	MOCK_OUTPUT_PARAMETER_ONLY(block_store_read, data_len, data_len);

	mock_ignore_decrypt_block();
	mock_ignore_encrypt_block(5);

	MOCK_RETVAL_ONLY(psa_cipher_update, PSA_ERROR_GENERIC_ERROR);

	MOCK_IGNORE(psa_cipher_abort);

	m_status = block_store_write(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						   data, BLOCK_SIZE, &num_written);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests, block_store_write_failure__psa_cipher_finish_return_error)
{
	size_t num_written = BLOCK_SIZE;
	size_t data_len = BLOCK_SIZE;
	uint8_t data[BLOCK_SIZE] = { 0 };

	mock_init_store();

	MOCK_OUTPUT_PARAMETER_ONLY(block_store_read, data_len, data_len);

	mock_ignore_decrypt_block();
	mock_ignore_encrypt_block(6);

	MOCK_RETVAL_ONLY(psa_cipher_finish, PSA_ERROR_GENERIC_ERROR);

	MOCK_IGNORE(psa_cipher_abort);

	m_status = block_store_write(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						   data, BLOCK_SIZE, &num_written);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests, block_store_write_failure__psa_cipher_finish_length_error)
{
	size_t num_written = BLOCK_SIZE;
	size_t data_len = BLOCK_SIZE;
	uint8_t data[BLOCK_SIZE] = { 0 };

	mock_init_store();

	MOCK_OUTPUT_PARAMETER_ONLY(block_store_read, data_len, data_len);

	mock_ignore_decrypt_block();
	mock_ignore_encrypt_block(6);

	MOCK_OUTPUT_PARAMETER_ONLY(psa_cipher_finish, output_length, BLK_AES_BLOCK_SIZE);

	MOCK_IGNORE(psa_cipher_abort);

	m_status = block_store_write(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						   data, BLOCK_SIZE, &num_written);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests, block_store_write_failure__block_store_write)
{
	size_t num_written = BLOCK_SIZE;
	size_t data_len = BLOCK_SIZE;
	uint8_t data[BLOCK_SIZE] = { 0 };

	mock_init_store();

	MOCK_OUTPUT_PARAMETER_ONLY(block_store_read, data_len, data_len);

	mock_ignore_decrypt_block();
	mock_ignore_encrypt_block();

	MOCK_RETVAL_ONLY(block_store_write, PSA_ERROR_GENERIC_ERROR);

	m_status = block_store_write(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						   data, BLOCK_SIZE, &num_written);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_GENERIC_ERROR);

	mock_deinit_store();
}

TEST(EncryptedBlockStoreUnitTests, block_store_write_failure__partial_write)
{
	size_t num_written = BLOCK_SIZE - 1;
	size_t data_len = BLOCK_SIZE;
	uint8_t data[BLOCK_SIZE] = { 0 };

	mock_init_store();

	MOCK_OUTPUT_PARAMETER_ONLY(block_store_read, data_len, data_len);

	mock_ignore_decrypt_block();
	mock_ignore_encrypt_block();

	MOCK_OUTPUT_PARAMETER_RETVAL(block_store_write, num_written, num_written, PSA_SUCCESS);

	m_status = block_store_write(m_block_store, CLIENT_ID, m_handle, LBA, 0,
						   data, BLOCK_SIZE, &num_written);
	UNSIGNED_LONGLONGS_EQUAL(m_status, PSA_ERROR_INSUFFICIENT_DATA);

	mock_deinit_store();
}
