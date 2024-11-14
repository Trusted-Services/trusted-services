/*
 * Copyright (c) 2020-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "packedc_crypto_client.h"
#include <service/crypto/client/caller/packed-c/crypto_caller.h>
#include <rpc_caller.h>


packedc_crypto_client::packedc_crypto_client() :
	crypto_client()
{

}

packedc_crypto_client::packedc_crypto_client(struct rpc_caller_session *session) :
	crypto_client(session)
{

}

packedc_crypto_client::~packedc_crypto_client()
{

}

/* Core crypto methods */
psa_status_t packedc_crypto_client::generate_key(
	const psa_key_attributes_t *attributes,
	psa_key_id_t *id)
{
	return crypto_caller_generate_key(&m_client, attributes, id);
}

psa_status_t packedc_crypto_client::destroy_key(
	psa_key_id_t id)
{
	return crypto_caller_destroy_key(&m_client, id);
}

psa_status_t packedc_crypto_client::copy_key(
	psa_key_id_t source_key,
	const psa_key_attributes_t *attributes,
	psa_key_id_t *target_key)
{
	return crypto_caller_copy_key(&m_client, source_key, attributes, target_key);
}

psa_status_t packedc_crypto_client::purge_key(
	psa_key_id_t id)
{
	return crypto_caller_purge_key(&m_client, id);
}

psa_status_t packedc_crypto_client::get_key_attributes(
	psa_key_id_t id,
	psa_key_attributes_t *attributes)
{
	return crypto_caller_get_key_attributes(&m_client, id, attributes);
}

psa_status_t packedc_crypto_client::import_key(
	const psa_key_attributes_t *attributes,
	const uint8_t *data, size_t data_length,
	psa_key_id_t *id)
{
	return crypto_caller_import_key(&m_client, attributes,
		data, data_length, id);
}

psa_status_t packedc_crypto_client::export_key(
	psa_key_id_t id,
	uint8_t *data, size_t data_size,
	size_t *data_length)
{
	return crypto_caller_export_key(&m_client, id,
		data, data_size, data_length);
}

psa_status_t packedc_crypto_client::export_public_key(
	psa_key_id_t id,
	uint8_t *data, size_t data_size, size_t *data_length)
{
	return crypto_caller_export_public_key(&m_client, id,
		data, data_size, data_length);
}

psa_status_t packedc_crypto_client::sign_hash(
	psa_key_id_t id, psa_algorithm_t alg,
	const uint8_t *hash, size_t hash_length,
	uint8_t *signature, size_t signature_size, size_t *signature_length)
{
	return crypto_caller_sign_hash(&m_client, id, alg,
		hash, hash_length,
		signature, signature_size, signature_length);
}

psa_status_t packedc_crypto_client::verify_hash(
	psa_key_id_t id, psa_algorithm_t alg,
	const uint8_t *hash, size_t hash_length,
	const uint8_t *signature, size_t signature_length)
{
	return crypto_caller_verify_hash(&m_client, id, alg,
		hash, hash_length,
		signature, signature_length);
}

psa_status_t packedc_crypto_client::sign_message(
	psa_key_id_t id, psa_algorithm_t alg,
	const uint8_t *message, size_t message_length,
	uint8_t *signature, size_t signature_size, size_t *signature_length)
{
	return crypto_caller_sign_message(&m_client, id, alg,
		message, message_length,
		signature, signature_size, signature_length);
}

psa_status_t packedc_crypto_client::verify_message(
	psa_key_id_t id, psa_algorithm_t alg,
	const uint8_t *message, size_t message_length,
	const uint8_t *signature, size_t signature_length)
{
	return crypto_caller_verify_message(&m_client, id, alg,
		message, message_length,
		signature, signature_length);
}

psa_status_t packedc_crypto_client::asymmetric_encrypt(
	psa_key_id_t id, psa_algorithm_t alg,
	const uint8_t *input, size_t input_length,
	const uint8_t *salt, size_t salt_length,
	uint8_t *output, size_t output_size, size_t *output_length)
{
	return crypto_caller_asymmetric_encrypt(&m_client, id, alg,
		input, input_length,
		salt, salt_length,
		output, output_size, output_length);
}

psa_status_t packedc_crypto_client::asymmetric_decrypt(
	psa_key_id_t id, psa_algorithm_t alg,
	const uint8_t *input, size_t input_length,
	const uint8_t *salt, size_t salt_length,
	uint8_t *output, size_t output_size, size_t *output_length)
{
	return crypto_caller_asymmetric_decrypt(&m_client, id, alg,
		input, input_length,
		salt, salt_length,
		output, output_size, output_length);
}

psa_status_t packedc_crypto_client::generate_random(
	uint8_t *output, size_t output_size)
{
	return crypto_caller_generate_random(&m_client,
		output, output_size);
}

/* Hash methods */
size_t packedc_crypto_client::hash_max_update_size() const
{
	return crypto_caller_hash_max_update_size(&m_client);
}

psa_status_t packedc_crypto_client::hash_setup(
	uint32_t *op_handle,
	psa_algorithm_t alg)
{
	return crypto_caller_hash_setup(&m_client,
		op_handle, alg);
}

psa_status_t packedc_crypto_client::hash_update(
	uint32_t op_handle,
	const uint8_t *input, size_t input_length)
{
	return crypto_caller_hash_update(&m_client,
		op_handle, input, input_length);
}

psa_status_t packedc_crypto_client::hash_finish(
	uint32_t op_handle,
	uint8_t *hash, size_t hash_size, size_t *hash_length)
{
	return crypto_caller_hash_finish(&m_client,
		op_handle, hash, hash_size, hash_length);
}

psa_status_t packedc_crypto_client::hash_abort(
	uint32_t op_handle)
{
	return crypto_caller_hash_abort(&m_client,
		op_handle);
}

psa_status_t packedc_crypto_client::hash_verify(
	uint32_t op_handle,
	const uint8_t *hash, size_t hash_length)
{
	return crypto_caller_hash_verify(&m_client,
		op_handle, hash, hash_length);
}

psa_status_t packedc_crypto_client::hash_clone(
	uint32_t source_op_handle,
	uint32_t *target_op_handle)
{
	return crypto_caller_hash_clone(&m_client,
		source_op_handle, target_op_handle);
}

/* Cipher methods */
size_t packedc_crypto_client::cipher_max_update_size() const
{
	return crypto_caller_cipher_max_update_size(&m_client);
}

psa_status_t packedc_crypto_client::cipher_encrypt_setup(
	uint32_t *op_handle,
	psa_key_id_t key,
	psa_algorithm_t alg)
{
	return crypto_caller_cipher_encrypt_setup(&m_client,
		op_handle, key, alg);
}

psa_status_t packedc_crypto_client::cipher_decrypt_setup(
	uint32_t *op_handle,
	psa_key_id_t key,
	psa_algorithm_t alg)
{
	return crypto_caller_cipher_decrypt_setup(&m_client,
		op_handle, key, alg);
}

psa_status_t packedc_crypto_client::cipher_generate_iv(
	uint32_t op_handle,
	uint8_t *iv, size_t iv_size, size_t *iv_length)
{
	return crypto_caller_cipher_generate_iv(&m_client,
		op_handle, iv, iv_size, iv_length);
}

psa_status_t packedc_crypto_client::cipher_set_iv(
	uint32_t op_handle,
	const uint8_t *iv, size_t iv_length)
{
	return crypto_caller_cipher_set_iv(&m_client,
		op_handle, iv, iv_length);
}

psa_status_t packedc_crypto_client::cipher_update(
	uint32_t op_handle,
	const uint8_t *input, size_t input_length,
	uint8_t *output, size_t output_size, size_t *output_length)
{
	return crypto_caller_cipher_update(&m_client,
		op_handle, input, input_length,
		output, output_size, output_length);
}

psa_status_t packedc_crypto_client::cipher_finish(
	uint32_t op_handle,
	uint8_t *output, size_t output_size, size_t *output_length)
{
	return crypto_caller_cipher_finish(&m_client,
		op_handle, output, output_size, output_length);
}

psa_status_t packedc_crypto_client::cipher_abort(
	uint32_t op_handle)
{
	return crypto_caller_cipher_abort(&m_client,
		op_handle);
}

/* MAC methods */
size_t packedc_crypto_client::mac_max_update_size() const
{
	return crypto_caller_mac_max_update_size(&m_client);
}

psa_status_t packedc_crypto_client::mac_sign_setup(
	uint32_t *op_handle,
	psa_key_id_t key,
	psa_algorithm_t alg)
{
	return crypto_caller_mac_sign_setup(&m_client,
		op_handle, key, alg);
}

psa_status_t packedc_crypto_client::mac_verify_setup(
	uint32_t *op_handle,
	psa_key_id_t key,
	psa_algorithm_t alg)
{
	return crypto_caller_mac_verify_setup(&m_client,
		op_handle, key, alg);
}

psa_status_t packedc_crypto_client::mac_update(
	uint32_t op_handle,
	const uint8_t *input, size_t input_length)
{
	return crypto_caller_mac_update(&m_client,
		op_handle, input, input_length);
}

psa_status_t packedc_crypto_client::mac_sign_finish(
	uint32_t op_handle,
	uint8_t *mac, size_t mac_size, size_t *mac_length)
{
	return crypto_caller_mac_sign_finish(&m_client,
		op_handle, mac, mac_size, mac_length);
}

psa_status_t packedc_crypto_client::mac_verify_finish(
	uint32_t op_handle,
	const uint8_t *mac, size_t mac_length)
{
	return crypto_caller_mac_verify_finish(&m_client,
		op_handle, mac, mac_length);
}

psa_status_t packedc_crypto_client::mac_abort(
	uint32_t op_handle)
{
	return crypto_caller_mac_abort(&m_client,
		op_handle);
}

/* Key derivation methods */
psa_status_t packedc_crypto_client::key_derivation_setup(
	uint32_t *op_handle,
	psa_algorithm_t alg)
{
	return crypto_caller_key_derivation_setup(&m_client,
		op_handle, alg);
}

psa_status_t packedc_crypto_client::key_derivation_get_capacity(
	const uint32_t op_handle,
	size_t *capacity)
{
	return crypto_caller_key_derivation_get_capacity(&m_client,
		op_handle, capacity);
}

psa_status_t packedc_crypto_client::key_derivation_set_capacity(
	uint32_t op_handle,
	size_t capacity)
{
	return crypto_caller_key_derivation_set_capacity(&m_client,
		op_handle, capacity);
}

psa_status_t packedc_crypto_client::key_derivation_input_bytes(
	uint32_t op_handle,
	psa_key_derivation_step_t step,
	const uint8_t *data, size_t data_length)
{
	return crypto_caller_key_derivation_input_bytes(&m_client,
		op_handle, step, data, data_length);
}

psa_status_t packedc_crypto_client::key_derivation_input_key(
	uint32_t op_handle,
	psa_key_derivation_step_t step,
	psa_key_id_t key)
{
	return crypto_caller_key_derivation_input_key(&m_client,
		op_handle, step, key);
}

psa_status_t packedc_crypto_client::key_derivation_output_bytes(
	uint32_t op_handle,
	uint8_t *output, size_t output_length)
{
	return crypto_caller_key_derivation_output_bytes(&m_client,
		op_handle, output, output_length);
}

psa_status_t packedc_crypto_client::key_derivation_output_key(
	const psa_key_attributes_t *attributes,
	uint32_t op_handle,
	psa_key_id_t *key)
{
	return crypto_caller_key_derivation_output_key(&m_client,
		attributes, op_handle, key);
}

psa_status_t packedc_crypto_client::key_derivation_abort(
	uint32_t op_handle)
{
	return crypto_caller_key_derivation_abort(&m_client,
		op_handle);
}

psa_status_t packedc_crypto_client::key_derivation_key_agreement(
	uint32_t op_handle,
	psa_key_derivation_step_t step,
	psa_key_id_t private_key,
	const uint8_t *peer_key, size_t peer_key_length)
{
	return crypto_caller_key_derivation_key_agreement(&m_client,
		op_handle, step, private_key,
		peer_key, peer_key_length);
}

psa_status_t packedc_crypto_client::raw_key_agreement(psa_algorithm_t alg,
	psa_key_id_t private_key,
	const uint8_t *peer_key, size_t peer_key_length,
	uint8_t *output, size_t output_size, size_t *output_length)
{
	return crypto_caller_raw_key_agreement(&m_client,
		alg, private_key, peer_key, peer_key_length,
		output, output_size, output_length);
}

int packedc_crypto_client::verify_pkcs7_signature(const uint8_t *signature_cert,
						  uint64_t signature_cert_len, const uint8_t *hash,
						  uint64_t hash_len, const uint8_t *public_key_cert,
						  uint64_t public_key_cert_len)
{
	return crypto_caller_verify_pkcs7_signature(&m_client, signature_cert, signature_cert_len,
						    hash, hash_len, public_key_cert,
						    public_key_cert_len);
}

int packedc_crypto_client::get_uefi_priv_auth_var_fingerprint(const uint8_t *signature_cert,
						  uint64_t signature_cert_len,
						  uint8_t *output)
{
	return crypto_caller_get_uefi_priv_auth_var_fingerprint(&m_client, signature_cert,
								signature_cert_len,
								output);
}
