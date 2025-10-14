/*
 * Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstring>
#include <cstdint>
#include <CppUTest/TestHarness.h>
#include "mac_service_scenarios.h"


mac_service_scenarios::mac_service_scenarios(crypto_client *crypto_client) :
	m_crypto_client(crypto_client),
	m_ref_input(NULL),
	m_keyid(0)
{

}

mac_service_scenarios::~mac_service_scenarios()
{
	destroy_key();

	delete m_crypto_client;
	m_crypto_client = NULL;

	delete [] m_ref_input;
	m_ref_input = NULL;
}

void mac_service_scenarios::create_ref_input(size_t size)
{
	m_ref_input = new uint8_t[size];
	memset(m_ref_input, 0x61, size);
}

psa_status_t mac_service_scenarios::generate_key()
{
	destroy_key();

	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH);
	psa_set_key_algorithm(&attributes, PSA_ALG_CMAC);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attributes, 256);

	psa_status_t status = m_crypto_client->generate_key(&attributes, &m_keyid);

	psa_reset_key_attributes(&attributes);

	return status;
}

void mac_service_scenarios::destroy_key()
{
	if (m_keyid) {

		m_crypto_client->destroy_key(m_keyid);
		m_keyid = 0;
	}
}

void mac_service_scenarios::signAndVerify()
{
	size_t max_payload = m_crypto_client->mac_max_update_size();
	static const size_t input_size = 19999;
	psa_status_t status;

	create_ref_input(input_size);

	status = generate_key();
	CHECK_EQUAL(PSA_SUCCESS, status);

	/* Calculate MAC over input data */
	uint32_t op_handle = 0;
	size_t input_byte_count = 0;

	status = m_crypto_client->mac_sign_setup(&op_handle, m_keyid, PSA_ALG_CMAC);
	CHECK_EQUAL(PSA_SUCCESS, status);

	while (input_byte_count < input_size) {

		size_t bytes_left = input_size - input_byte_count;
		size_t update_len = (bytes_left < max_payload) ? bytes_left : max_payload;

		status = m_crypto_client->mac_update(op_handle,
			&m_ref_input[input_byte_count], update_len);
		CHECK_EQUAL(PSA_SUCCESS, status);

		input_byte_count += update_len;
	}

	size_t mac_len;
	uint8_t mac[PSA_MAC_MAX_SIZE];

	status = m_crypto_client->mac_sign_finish(&op_handle, mac, sizeof(mac), &mac_len);
	CHECK_EQUAL(PSA_SUCCESS, status);
	CHECK_EQUAL(0, op_handle);

	/* Verify MAC */
	input_byte_count = 0;

	status = m_crypto_client->mac_verify_setup(&op_handle, m_keyid, PSA_ALG_CMAC);
	CHECK_EQUAL(PSA_SUCCESS, status);

	while (input_byte_count < input_size) {

		size_t bytes_left = input_size - input_byte_count;
		size_t update_len = (bytes_left < max_payload) ? bytes_left : max_payload;

		status = m_crypto_client->mac_update(op_handle,
			&m_ref_input[input_byte_count], update_len);
		CHECK_EQUAL(PSA_SUCCESS, status);

		input_byte_count += update_len;
	}

	status = m_crypto_client->mac_verify_finish(&op_handle, mac, mac_len);
	CHECK_EQUAL(PSA_SUCCESS, status);
}

void mac_service_scenarios::macAbort()
{
	/* Aborts a mac operation prior to the first update */
	uint32_t op_handle = 0;
	psa_status_t status = generate_key();
	CHECK_EQUAL(PSA_SUCCESS, status);

	status = m_crypto_client->mac_sign_setup(&op_handle, m_keyid, PSA_ALG_CMAC);
	CHECK_EQUAL(PSA_SUCCESS, status);

	status = m_crypto_client->mac_abort(&op_handle);
	CHECK_EQUAL(PSA_SUCCESS, status);

	size_t mac_len;
	uint8_t mac[PSA_MAC_MAX_SIZE];

	status = m_crypto_client->mac_sign_finish(&op_handle, mac, sizeof(mac), &mac_len);
	CHECK_EQUAL(PSA_ERROR_BAD_STATE, status);
}
