/*
 * Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstring>
#include <cstdint>
#include <CppUTest/TestHarness.h>
#include "cipher_service_scenarios.h"


cipher_service_scenarios::cipher_service_scenarios(crypto_client *crypto_client) :
	m_crypto_client(crypto_client),
	m_ref_plaintext(NULL),
	m_keyid(0)
{

}

cipher_service_scenarios::~cipher_service_scenarios()
{
	destroy_key();

	delete m_crypto_client;
	m_crypto_client = NULL;

	delete [] m_ref_plaintext;
	m_ref_plaintext = NULL;
}

void cipher_service_scenarios::create_ref_plaintext(size_t size)
{
	m_ref_plaintext = new uint8_t[size];
	memset(m_ref_plaintext, 0x42, size);
}

psa_status_t cipher_service_scenarios::generate_key()
{
	destroy_key();

	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attributes, PSA_ALG_CTR);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attributes, 256);

	psa_status_t status = m_crypto_client->generate_key(&attributes, &m_keyid);

	psa_reset_key_attributes(&attributes);

	return status;
}

void cipher_service_scenarios::destroy_key()
{
	if (m_keyid) {

		m_crypto_client->destroy_key(m_keyid);
		m_keyid = 0;
	}
}

void cipher_service_scenarios::encryptDecryptRoundtrip()
{
	size_t max_payload = m_crypto_client->cipher_max_update_size();
	static const size_t plaintext_size = 9999;
	psa_status_t status;

	create_ref_plaintext(plaintext_size);

	status = generate_key();
	CHECK_EQUAL(PSA_SUCCESS, status);

	uint8_t ciphertext[PSA_CIPHER_ENCRYPT_OUTPUT_MAX_SIZE(plaintext_size)];

	uint32_t op_handle = 0;

	/* Encrypt the input plaintext */
	size_t input_byte_count = 0;
	size_t output_byte_count = 0;

	status = m_crypto_client->cipher_encrypt_setup(&op_handle, m_keyid, PSA_ALG_CTR);
	CHECK_EQUAL(PSA_SUCCESS, status);

	size_t iv_len;
	uint8_t iv[PSA_CIPHER_IV_MAX_SIZE];
	status = m_crypto_client->cipher_generate_iv(op_handle, iv, sizeof(iv), &iv_len);
	CHECK_EQUAL(PSA_SUCCESS, status);

	while (input_byte_count < plaintext_size) {

		size_t update_output_len = 0;
		size_t output_space = sizeof(ciphertext) - output_byte_count;
		size_t bytes_left = plaintext_size - input_byte_count;
		size_t update_len = (bytes_left < max_payload) ? bytes_left : max_payload;

		status = m_crypto_client->cipher_update(op_handle,
			&m_ref_plaintext[input_byte_count], update_len,
			&ciphertext[output_byte_count], output_space, &update_output_len);
		CHECK_EQUAL(PSA_SUCCESS, status);

		input_byte_count += update_len;
		output_byte_count += update_output_len;
	}

	size_t finish_output_len = 0;
	size_t finish_output_space = sizeof(ciphertext) - output_byte_count;

	status = m_crypto_client->cipher_finish(&op_handle,
		&ciphertext[output_byte_count], finish_output_space, &finish_output_len);
	CHECK_EQUAL(PSA_SUCCESS, status);
	CHECK_EQUAL(0, op_handle);

	output_byte_count += finish_output_len;

	/* Decrypt the ciphertext */
	uint8_t decrypted_plaintext[plaintext_size];

	size_t ciphertext_size = output_byte_count;
	input_byte_count = 0;
	output_byte_count = 0;

	status = m_crypto_client->cipher_decrypt_setup(&op_handle, m_keyid, PSA_ALG_CTR);
	CHECK_EQUAL(PSA_SUCCESS, status);

	status = m_crypto_client->cipher_set_iv(op_handle, iv, iv_len);
	CHECK_EQUAL(PSA_SUCCESS, status);

	while (input_byte_count < ciphertext_size) {

		size_t update_output_len = 0;
		size_t output_space = sizeof(decrypted_plaintext) - output_byte_count;
		size_t bytes_left = ciphertext_size - input_byte_count;
		size_t update_len = (bytes_left < max_payload) ? bytes_left : max_payload;

		status = m_crypto_client->cipher_update(op_handle,
			&ciphertext[input_byte_count], update_len,
			&decrypted_plaintext[output_byte_count], output_space, &update_output_len);
		CHECK_EQUAL(PSA_SUCCESS, status);

		input_byte_count += update_len;
		output_byte_count += update_output_len;
	}

	finish_output_len = 0;
	finish_output_space = sizeof(decrypted_plaintext) - output_byte_count;

	status = m_crypto_client->cipher_finish(&op_handle,
		&ciphertext[output_byte_count], finish_output_space, &finish_output_len);
	CHECK_EQUAL(PSA_SUCCESS, status);

	output_byte_count += finish_output_len;

	/* Expect decrypted plaintext to be identical to reference plaintext */
	UNSIGNED_LONGS_EQUAL(plaintext_size, output_byte_count);
	MEMCMP_EQUAL(m_ref_plaintext, decrypted_plaintext, plaintext_size);
}

void cipher_service_scenarios::cipherAbort()
{
	/* Aborts a cipher operation prior to the first update */
	uint32_t op_handle = 0;
	psa_status_t status = generate_key();
	CHECK_EQUAL(PSA_SUCCESS, status);

	status = m_crypto_client->cipher_encrypt_setup(&op_handle, m_keyid, PSA_ALG_CTR);
	CHECK_EQUAL(PSA_SUCCESS, status);

	status = m_crypto_client->cipher_abort(&op_handle);
	CHECK_EQUAL(PSA_SUCCESS, status);

	size_t iv_len;
	uint8_t iv[PSA_CIPHER_IV_MAX_SIZE];
	status = m_crypto_client->cipher_generate_iv(op_handle, iv, sizeof(iv), &iv_len);
	CHECK_EQUAL(PSA_ERROR_BAD_STATE, status);
}
