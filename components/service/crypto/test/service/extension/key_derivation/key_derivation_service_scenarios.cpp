/*
 * Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstring>
#include <cstdint>
#include <CppUTest/TestHarness.h>
#include "key_derivation_service_scenarios.h"


key_derivation_service_scenarios::key_derivation_service_scenarios(crypto_client *crypto_client) :
	m_crypto_client(crypto_client),
	m_secret_keyid(0)
{

}

key_derivation_service_scenarios::~key_derivation_service_scenarios()
{
	destroySecretKey();

	delete m_crypto_client;
	m_crypto_client = NULL;
}

psa_status_t key_derivation_service_scenarios::generateSecretKey()
{
	destroySecretKey();

	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attributes, PSA_ALG_HKDF(PSA_ALG_SHA_256));
	psa_set_key_type(&attributes, PSA_KEY_TYPE_DERIVE);
	psa_set_key_bits(&attributes, 256);

	psa_status_t status = m_crypto_client->generate_key(&attributes, &m_secret_keyid);

	psa_reset_key_attributes(&attributes);

	return status;
}

void key_derivation_service_scenarios::destroySecretKey()
{
	if (m_secret_keyid) {

		m_crypto_client->destroy_key(m_secret_keyid);
		m_secret_keyid = 0;
	}
}

void key_derivation_service_scenarios::hkdfDeriveKey()
{
	psa_status_t status;
	uint32_t op_handle = 0;

	status = generateSecretKey();
	CHECK_EQUAL(PSA_SUCCESS, status);

	status = m_crypto_client->key_derivation_setup(&op_handle, PSA_ALG_HKDF(PSA_ALG_SHA_256));
	CHECK_EQUAL(PSA_SUCCESS, status);

	static const uint8_t salt[] = {0,1,2,3,4,5,6,7,8,9};
	status = m_crypto_client->key_derivation_input_bytes(op_handle,
		PSA_KEY_DERIVATION_INPUT_SALT, salt, sizeof(salt));
	CHECK_EQUAL(PSA_SUCCESS, status);

	status = m_crypto_client->key_derivation_input_key(op_handle,
		PSA_KEY_DERIVATION_INPUT_SECRET, m_secret_keyid);
	CHECK_EQUAL(PSA_SUCCESS, status);

	static const uint8_t info[] = {50,51,52,53,54,55,56,57,58};
	status = m_crypto_client->key_derivation_input_bytes(op_handle,
		PSA_KEY_DERIVATION_INPUT_INFO, info, sizeof(info));
	CHECK_EQUAL(PSA_SUCCESS, status);

	/* Derive a key that could be used for symmetric encryption */
	psa_key_id_t derived_keyid;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT);
	psa_set_key_algorithm(&attributes, PSA_ALG_CTR);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attributes, 256);

	status = m_crypto_client->key_derivation_output_key(&attributes, op_handle, &derived_keyid);
	psa_reset_key_attributes(&attributes);
	CHECK_EQUAL(PSA_SUCCESS, status);

	status = m_crypto_client->key_derivation_abort(&op_handle);
	CHECK_EQUAL(PSA_SUCCESS, status);

	status = m_crypto_client->destroy_key(derived_keyid);
	CHECK_EQUAL(PSA_SUCCESS, status);
}

void key_derivation_service_scenarios::hkdfDeriveBytes()
{
	psa_status_t status;
	uint32_t op_handle = 0;

	status = generateSecretKey();
	CHECK_EQUAL(PSA_SUCCESS, status);

	status = m_crypto_client->key_derivation_setup(&op_handle, PSA_ALG_HKDF(PSA_ALG_SHA_256));
	CHECK_EQUAL(PSA_SUCCESS, status);

	status = m_crypto_client->key_derivation_input_key(op_handle,
		PSA_KEY_DERIVATION_INPUT_SECRET, m_secret_keyid);
	CHECK_EQUAL(PSA_SUCCESS, status);

	static const uint8_t info[] = {50,51,52,53,54,55,56,57,58};
	status = m_crypto_client->key_derivation_input_bytes(op_handle,
		PSA_KEY_DERIVATION_INPUT_INFO, info, sizeof(info));
	CHECK_EQUAL(PSA_SUCCESS, status);

	/* Derive some output bytes */
	uint8_t derived_bytes[99];
	status = m_crypto_client->key_derivation_output_bytes(op_handle,
		derived_bytes, sizeof(derived_bytes));
	CHECK_EQUAL(PSA_SUCCESS, status);

	status = m_crypto_client->key_derivation_abort(&op_handle);
	CHECK_EQUAL(PSA_SUCCESS, status);
}

void key_derivation_service_scenarios::deriveAbort()
{
	psa_status_t status;
	uint32_t op_handle = 0;

	status = generateSecretKey();
	CHECK_EQUAL(PSA_SUCCESS, status);

	status = m_crypto_client->key_derivation_setup(&op_handle, PSA_ALG_HKDF(PSA_ALG_SHA_256));
	CHECK_EQUAL(PSA_SUCCESS, status);

	status = m_crypto_client->key_derivation_input_key(op_handle,
		PSA_KEY_DERIVATION_INPUT_SECRET, m_secret_keyid);
	CHECK_EQUAL(PSA_SUCCESS, status);

	status = m_crypto_client->key_derivation_abort(&op_handle);
	CHECK_EQUAL(PSA_SUCCESS, status);

	/* Expect operation to have been aborted */
	status = m_crypto_client->key_derivation_set_capacity(op_handle, 100);
	CHECK_EQUAL(PSA_ERROR_BAD_STATE, status);
}
