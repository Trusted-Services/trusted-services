/*
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string>
#include <cstring>
#include <cstdint>
#include <vector>
#include <CppUTest/TestHarness.h>
#include "hash_service_scenarios.h"
#include "hash_test_vectors.h"

hash_service_scenarios::hash_service_scenarios(crypto_client *crypto_client) :
	m_crypto_client(crypto_client),
	m_ref_input(NULL)
{

}

hash_service_scenarios::~hash_service_scenarios()
{
	delete m_crypto_client;
	m_crypto_client = NULL;

	delete [] m_ref_input;
	m_ref_input = NULL;
}

void hash_service_scenarios::create_ref_input(size_t size)
{
	m_ref_input = new uint8_t[size];
	memset(m_ref_input, 0x51, size);
}

void hash_service_scenarios::calculateHash()
{
	/* Calculates a hash and compares result against test vector */
	psa_status_t status;
	std::vector<uint8_t> input;
	std::vector<uint8_t> expected_output;
	uint8_t output[PSA_HASH_MAX_SIZE];
	size_t output_len;

	hash_test_vectors::plaintext_1_len_610(input);
	hash_test_vectors::sha256_1(expected_output);

	uint32_t op_handle = 0;

	status = m_crypto_client->hash_setup(&op_handle, PSA_ALG_SHA_256);
	CHECK_EQUAL(PSA_SUCCESS, status);

	status = m_crypto_client->hash_update(op_handle, &input[0], input.size());
	CHECK_EQUAL(PSA_SUCCESS, status);

	status = m_crypto_client->hash_finish(&op_handle, output, sizeof(output), &output_len);
	CHECK_EQUAL(PSA_SUCCESS, status);

	UNSIGNED_LONGS_EQUAL(expected_output.size(), output_len);
	MEMCMP_EQUAL(&expected_output[0], &output[0], output_len);
}

void hash_service_scenarios::hashAndVerify()
{
	/* Calculates and verifies hash over a large reference input */
	size_t max_payload = m_crypto_client->hash_max_update_size();
	static const size_t input_size = 9999;
	size_t byte_count = 0;

	create_ref_input(input_size);

	uint8_t hash[PSA_HASH_MAX_SIZE];
	size_t hash_len;

	uint32_t op_handle = 0;
	psa_status_t status;

	status = m_crypto_client->hash_setup(&op_handle, PSA_ALG_SHA_256);
	CHECK_EQUAL(PSA_SUCCESS, status);

	while (byte_count < input_size) {

		size_t bytes_left = input_size - byte_count;
		size_t update_size = (bytes_left < max_payload) ?
			bytes_left :
			max_payload;

		status = m_crypto_client->hash_update(op_handle, &m_ref_input[byte_count], update_size);
		CHECK_EQUAL(PSA_SUCCESS, status);

		byte_count += update_size;
	}

	uint32_t clone_op_handle = 0;

	status = m_crypto_client->hash_clone(op_handle, &clone_op_handle);
	CHECK_EQUAL(PSA_SUCCESS, status);

	status = m_crypto_client->hash_finish(&op_handle, hash, sizeof(hash), &hash_len);
	CHECK_EQUAL(PSA_SUCCESS, status);

	status = m_crypto_client->hash_verify(&clone_op_handle, hash, hash_len);
	CHECK_EQUAL(PSA_SUCCESS, status);
}

void hash_service_scenarios::hashAbort()
{
	/* Aborts a hash operation after the first update */
	size_t max_payload = m_crypto_client->hash_max_update_size();
	static const size_t input_size = 15999;

	create_ref_input(input_size);

	uint32_t op_handle = 0;
	psa_status_t status;

	status = m_crypto_client->hash_setup(&op_handle, PSA_ALG_SHA_256);
	CHECK_EQUAL(PSA_SUCCESS, status);

	size_t update_size = (input_size < max_payload) ?
			input_size :
			max_payload;

	status = m_crypto_client->hash_update(op_handle, &m_ref_input[0], update_size);
	CHECK_EQUAL(PSA_SUCCESS, status);

	status = m_crypto_client->hash_abort(&op_handle);
	CHECK_EQUAL(PSA_SUCCESS, status);

	status = m_crypto_client->hash_update(op_handle, &m_ref_input[0], update_size);
	CHECK_EQUAL(PSA_ERROR_BAD_STATE, status);
}
