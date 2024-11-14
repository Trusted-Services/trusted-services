/*
 * Copyright (c) 2020-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstring>
#include <cstdlib>
#include "protobuf_crypto_client.h"
#include <protocols/rpc/common/packed-c/status.h>
#include <service/common/serializer/protobuf/pb_helper.h>
#include <service/crypto/protobuf/opcodes.pb.h>
#include <service/crypto/protobuf/generate_key.pb.h>
#include <service/crypto/protobuf/destroy_key.pb.h>
#include <service/crypto/protobuf/import_key.pb.h>
#include <service/crypto/protobuf/export_key.pb.h>
#include <service/crypto/protobuf/export_public_key.pb.h>
#include <service/crypto/protobuf/sign_hash.pb.h>
#include <service/crypto/protobuf/verify_hash.pb.h>
#include <service/crypto/protobuf/asymmetric_encrypt.pb.h>
#include <service/crypto/protobuf/asymmetric_decrypt.pb.h>
#include <service/crypto/protobuf/generate_random.pb.h>
#include <pb_encode.h>
#include <pb_decode.h>

protobuf_crypto_client::protobuf_crypto_client() :
	crypto_client()
{

}

protobuf_crypto_client::protobuf_crypto_client(struct rpc_caller_session *session) :
	crypto_client(session)
{

}

protobuf_crypto_client::~protobuf_crypto_client()
{

}

void protobuf_crypto_client::translate_key_attributes(ts_crypto_KeyAttributes &proto_attributes,
							const psa_key_attributes_t &psa_attributes)
{
	proto_attributes.type = psa_get_key_type(&psa_attributes);
	proto_attributes.key_bits = psa_get_key_bits(&psa_attributes);
	proto_attributes.lifetime = psa_get_key_lifetime(&psa_attributes);
	proto_attributes.id = psa_get_key_id(&psa_attributes);

	proto_attributes.has_policy = true;
	proto_attributes.policy.usage = psa_get_key_usage_flags(&psa_attributes);
	proto_attributes.policy.alg = psa_get_key_algorithm(&psa_attributes);
 }

psa_status_t protobuf_crypto_client::generate_key(const psa_key_attributes_t *attributes,
	psa_key_id_t *id)
{
	size_t req_len;
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	ts_crypto_GenerateKeyIn req_msg = ts_crypto_GenerateKeyIn_init_default;

	translate_key_attributes(req_msg.attributes, *attributes);
	req_msg.has_attributes = true;

	if (pb_get_encoded_size(&req_len, ts_crypto_GenerateKeyIn_fields, &req_msg)) {

		rpc_call_handle call_handle;
		uint8_t *req_buf;

		call_handle = rpc_caller_session_begin(m_client.session, &req_buf, req_len, 0);

		if (call_handle) {

			uint8_t *resp_buf;
			size_t resp_len;
			service_status_t service_status;

			pb_ostream_t ostream = pb_ostream_from_buffer(req_buf, req_len);
			pb_encode(&ostream, ts_crypto_GenerateKeyIn_fields, &req_msg);

			m_client.rpc_status =
				rpc_caller_session_invoke(call_handle,
							  ts_crypto_Opcode_GENERATE_KEY,
							  &resp_buf, &resp_len, &service_status);

			if (m_client.rpc_status == RPC_SUCCESS) {

				psa_status = service_status;

				if (psa_status == PSA_SUCCESS) {

					ts_crypto_GenerateKeyOut resp_msg = ts_crypto_GenerateKeyOut_init_default;
					pb_istream_t istream = pb_istream_from_buffer(resp_buf, resp_len);

					if (pb_decode(&istream, ts_crypto_GenerateKeyOut_fields, &resp_msg)) {

						*id = resp_msg.id;
					}
					else {
						/* Failed to decode response message */
						psa_status = PSA_ERROR_GENERIC_ERROR;
					}
				}
			}

			rpc_caller_session_end(call_handle);
		}
	}

	return psa_status;
}

psa_status_t protobuf_crypto_client::destroy_key(psa_key_id_t id)
{
	size_t req_len;
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	ts_crypto_DestroyKeyIn req_msg = ts_crypto_DestroyKeyIn_init_default;

	req_msg.id = id;

	if (pb_get_encoded_size(&req_len, ts_crypto_DestroyKeyIn_fields, &req_msg)) {

		rpc_call_handle call_handle;
		uint8_t *req_buf;

		call_handle = rpc_caller_session_begin(m_client.session, &req_buf, req_len, 0);

		if (call_handle) {

			uint8_t *resp_buf;
			size_t resp_len;
			service_status_t service_status;

			pb_ostream_t ostream = pb_ostream_from_buffer(req_buf, req_len);
			pb_encode(&ostream, ts_crypto_DestroyKeyIn_fields, &req_msg);

			m_client.rpc_status =
				rpc_caller_session_invoke(call_handle, ts_crypto_Opcode_DESTROY_KEY,
							  &resp_buf, &resp_len, &service_status);

			if (m_client.rpc_status == RPC_SUCCESS) psa_status = service_status;

			rpc_caller_session_end(call_handle);
		}
	}

	return psa_status;
}

psa_status_t protobuf_crypto_client::import_key(const psa_key_attributes_t *attributes,
						const uint8_t *data, size_t data_length, psa_key_id_t *id)
{
	size_t req_len;
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	ts_crypto_ImportKeyIn req_msg = ts_crypto_ImportKeyIn_init_default;
	pb_bytes_array_t *key_byte_array =
		pb_malloc_byte_array_containing_bytes(data, data_length);

	translate_key_attributes(req_msg.attributes, *attributes);
	req_msg.has_attributes = true;
	req_msg.data = pb_out_byte_array(key_byte_array);

	if (pb_get_encoded_size(&req_len, ts_crypto_ImportKeyIn_fields, &req_msg)) {

		rpc_call_handle call_handle;
		uint8_t *req_buf;

		call_handle = rpc_caller_session_begin(m_client.session, &req_buf, req_len, 0);

		if (call_handle) {

			uint8_t *resp_buf;
			size_t resp_len;
			service_status_t service_status;

			pb_ostream_t ostream = pb_ostream_from_buffer(req_buf, req_len);
			pb_encode(&ostream, ts_crypto_ImportKeyIn_fields, &req_msg);

			m_client.rpc_status =
				rpc_caller_session_invoke(call_handle, ts_crypto_Opcode_IMPORT_KEY,
							  &resp_buf, &resp_len, &service_status);

			if (m_client.rpc_status == RPC_SUCCESS) {

				psa_status = service_status;

				if (psa_status == PSA_SUCCESS) {

					ts_crypto_ImportKeyOut resp_msg = ts_crypto_ImportKeyOut_init_default;
					pb_istream_t istream = pb_istream_from_buffer(resp_buf, resp_len);

					if (pb_decode(&istream, ts_crypto_ImportKeyOut_fields, &resp_msg)) {

						*id = resp_msg.id;
					}
					else {
						/* Failed to decode response message */
						psa_status = PSA_ERROR_GENERIC_ERROR;
					}
				}
			}

			rpc_caller_session_end(call_handle);
		}
	}

	::free(key_byte_array);

	return psa_status;
}

psa_status_t protobuf_crypto_client::copy_key(
	psa_key_id_t source_key,
	const psa_key_attributes_t *attributes,
	psa_key_id_t *target_key)
{
	(void)source_key;
	(void)attributes;
	(void)target_key;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::purge_key(
	psa_key_id_t id)
{
	(void)id;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::get_key_attributes(
	psa_key_id_t id,
	psa_key_attributes_t *attributes)
{
	(void)id;
	(void)attributes;

	return PSA_ERROR_NOT_SUPPORTED;
}


psa_status_t protobuf_crypto_client::export_key(psa_key_id_t id,
						uint8_t *data, size_t data_size,
						size_t *data_length)
{
	size_t req_len;
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	ts_crypto_ExportKeyIn req_msg = ts_crypto_ExportKeyIn_init_default;
	req_msg.id = id;

	*data_length = 0; /* For failure case */

	if (pb_get_encoded_size(&req_len, ts_crypto_ExportKeyIn_fields, &req_msg)) {

		rpc_call_handle call_handle;
		uint8_t *req_buf;

		call_handle = rpc_caller_session_begin(m_client.session, &req_buf, req_len,
						       PB_PACKET_LENGTH(data_size));

		if (call_handle) {

			uint8_t *resp_buf;
			size_t resp_len;
			service_status_t service_status;

			pb_ostream_t ostream = pb_ostream_from_buffer(req_buf, req_len);
			pb_encode(&ostream, ts_crypto_ExportKeyIn_fields, &req_msg);

			m_client.rpc_status =
				rpc_caller_session_invoke(call_handle, ts_crypto_Opcode_EXPORT_KEY,
							  &resp_buf, &resp_len, &service_status);

			if (m_client.rpc_status == RPC_SUCCESS) {

				psa_status = service_status;

				if (psa_status == PSA_SUCCESS) {

					ts_crypto_ExportKeyOut resp_msg = ts_crypto_ExportKeyOut_init_default;
					pb_bytes_array_t *exported_key = pb_malloc_byte_array(resp_len);

					if (exported_key) {

						resp_msg.data = pb_in_byte_array(exported_key);
						pb_istream_t istream = pb_istream_from_buffer(resp_buf, resp_len);

						if (pb_decode(&istream, ts_crypto_ExportKeyOut_fields, &resp_msg)) {

							if (exported_key->size <= data_size) {

								memcpy(data, exported_key->bytes, exported_key->size);
								*data_length = exported_key->size;
							}
							else {
								/* Provided buffer is too small */
								psa_status = PSA_ERROR_BUFFER_TOO_SMALL;
							}
						}
						else {
							/* Failed to decode response message */
							psa_status = PSA_ERROR_GENERIC_ERROR;
						}

						::free(exported_key);
					}
					else {
						/* Failed to allocate buffer for exported key */
						psa_status = PSA_ERROR_INSUFFICIENT_MEMORY;
					}
				}
			}

			rpc_caller_session_end(call_handle);
		}
	}

	return psa_status;
}

psa_status_t protobuf_crypto_client::export_public_key(psa_key_id_t id,
								uint8_t *data, size_t data_size, size_t *data_length)
{
	size_t req_len;
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	ts_crypto_ExportPublicKeyIn req_msg = ts_crypto_ExportPublicKeyIn_init_default;
	req_msg.id = id;

	*data_length = 0; /* For failure case */

	if (pb_get_encoded_size(&req_len, ts_crypto_ExportPublicKeyIn_fields, &req_msg)) {

		rpc_call_handle call_handle;
		uint8_t *req_buf;

		call_handle = rpc_caller_session_begin(m_client.session, &req_buf, req_len,
						       PB_PACKET_LENGTH(data_size));

		if (call_handle) {

			uint8_t *resp_buf;
			size_t resp_len;
			service_status_t service_status;

			pb_ostream_t ostream = pb_ostream_from_buffer(req_buf, req_len);
			pb_encode(&ostream, ts_crypto_ExportPublicKeyIn_fields, &req_msg);

			m_client.rpc_status =
				rpc_caller_session_invoke(call_handle,
							  ts_crypto_Opcode_EXPORT_PUBLIC_KEY,
							  &resp_buf, &resp_len, &service_status);

			if (m_client.rpc_status == RPC_SUCCESS) {

				psa_status = service_status;

				if (psa_status == PSA_SUCCESS) {

					ts_crypto_ExportPublicKeyOut resp_msg =
						ts_crypto_ExportPublicKeyOut_init_default;
					pb_bytes_array_t *exported_key = pb_malloc_byte_array(resp_len);

					if (exported_key) {

						resp_msg.data = pb_in_byte_array(exported_key);
						pb_istream_t istream = pb_istream_from_buffer(resp_buf, resp_len);

						if (pb_decode(&istream, ts_crypto_ExportPublicKeyOut_fields, &resp_msg)) {

							if (exported_key->size <= data_size) {

								memcpy(data, exported_key->bytes, exported_key->size);
								*data_length = exported_key->size;
							}
							else {
								/* Provided buffer is too small */
								psa_status = PSA_ERROR_BUFFER_TOO_SMALL;
							}
						}
						else {
							/* Failed to decode response message */
							psa_status = PSA_ERROR_GENERIC_ERROR;
						}

						::free(exported_key);
					}
					else {
						/* Failed to alloocate buffer for exported key */
						psa_status = PSA_ERROR_INSUFFICIENT_MEMORY;
					}
				}
			}

			rpc_caller_session_end(call_handle);
		}
	}

	return psa_status;
}

psa_status_t protobuf_crypto_client::sign_hash(psa_key_id_t id, psa_algorithm_t alg,
							const uint8_t *hash, size_t hash_length,
							uint8_t *signature, size_t signature_size, size_t *signature_length)
{
	return asym_sign(ts_crypto_Opcode_SIGN_HASH, id, alg,
				hash, hash_length,
				signature, signature_size, signature_length);
}

psa_status_t protobuf_crypto_client::sign_message(psa_key_id_t id, psa_algorithm_t alg,
							const uint8_t *message, size_t message_length,
							uint8_t *signature, size_t signature_size, size_t *signature_length)
{
	return asym_sign(ts_crypto_Opcode_SIGN_MESSAGE, id, alg,
				message, message_length,
				signature, signature_size, signature_length);
}

psa_status_t protobuf_crypto_client::asym_sign(uint32_t opcode,
							psa_key_id_t id, psa_algorithm_t alg,
							const uint8_t *hash, size_t hash_length,
							uint8_t *signature, size_t signature_size, size_t *signature_length)
{
	size_t req_len;
	pb_bytes_array_t *hash_byte_array =
		pb_malloc_byte_array_containing_bytes(hash, hash_length);
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	ts_crypto_SignHashIn req_msg = ts_crypto_SignHashIn_init_default;

	*signature_length = 0;  /* For failure case */

	req_msg.id = id;
	req_msg.alg = alg;
	req_msg.hash = pb_out_byte_array(hash_byte_array);

	if (pb_get_encoded_size(&req_len, ts_crypto_SignHashIn_fields, &req_msg)) {

		rpc_call_handle call_handle;
		uint8_t *req_buf;

		call_handle = rpc_caller_session_begin(m_client.session, &req_buf, req_len,
						       PB_PACKET_LENGTH(signature_size));

		if (call_handle) {

			uint8_t *resp_buf;
			size_t resp_len;
			service_status_t service_status;

			pb_ostream_t ostream = pb_ostream_from_buffer(req_buf, req_len);
			pb_encode(&ostream, ts_crypto_SignHashIn_fields, &req_msg);

			m_client.rpc_status =
				rpc_caller_session_invoke(call_handle, opcode, &resp_buf, &resp_len,
							  &service_status);

			if (m_client.rpc_status == RPC_SUCCESS) {

				psa_status = service_status;

				if (psa_status == PSA_SUCCESS) {

					pb_bytes_array_t *sig_byte_array =
						pb_malloc_byte_array(PSA_SIGNATURE_MAX_SIZE);
					ts_crypto_SignHashOut resp_msg = ts_crypto_SignHashOut_init_default;
					resp_msg.signature = pb_in_byte_array(sig_byte_array);

					pb_istream_t istream = pb_istream_from_buffer(resp_buf, resp_len);

					if (pb_decode(&istream, ts_crypto_SignHashOut_fields, &resp_msg)) {

						if (sig_byte_array->size <= signature_size) {

							memcpy(signature,
								sig_byte_array->bytes, sig_byte_array->size);
							*signature_length = sig_byte_array->size;
						}
						else {
							/* Provided buffer is too small */
							psa_status = PSA_ERROR_BUFFER_TOO_SMALL;
						}
					}
					else {
						/* Failed to decode response message */
						psa_status = PSA_ERROR_GENERIC_ERROR;
					}

					::free(sig_byte_array);
				}
			}

			rpc_caller_session_end(call_handle);
		}
	}

	::free(hash_byte_array);

	return psa_status;
}

psa_status_t protobuf_crypto_client::verify_hash(psa_key_id_t id, psa_algorithm_t alg,
						const uint8_t *hash, size_t hash_length,
						const uint8_t *signature, size_t signature_length)
{
	return asym_verify(ts_crypto_Opcode_VERIFY_HASH, id, alg,
				hash, hash_length,
				signature, signature_length);
}

psa_status_t protobuf_crypto_client::verify_message(psa_key_id_t id, psa_algorithm_t alg,
						const uint8_t *message, size_t message_length,
						const uint8_t *signature, size_t signature_length)
{
	return asym_verify(ts_crypto_Opcode_VERIFY_MESSAGE, id, alg,
				message, message_length,
				signature, signature_length);
}

psa_status_t protobuf_crypto_client::asym_verify(uint32_t opcode,
						psa_key_id_t id, psa_algorithm_t alg,
						const uint8_t *hash, size_t hash_length,
						const uint8_t *signature, size_t signature_length)
{
	size_t req_len;
	pb_bytes_array_t *hash_byte_array =
		pb_malloc_byte_array_containing_bytes(hash, hash_length);
	pb_bytes_array_t *sig_byte_array =
		pb_malloc_byte_array_containing_bytes(signature, signature_length);
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	ts_crypto_VerifyHashIn req_msg = ts_crypto_VerifyHashIn_init_default;

	req_msg.id = id;
	req_msg.alg = alg;
	req_msg.hash = pb_out_byte_array(hash_byte_array);
	req_msg.signature = pb_out_byte_array(sig_byte_array);

	if (pb_get_encoded_size(&req_len, ts_crypto_VerifyHashIn_fields, &req_msg)) {

		rpc_call_handle call_handle;
		uint8_t *req_buf;

		call_handle = rpc_caller_session_begin(m_client.session, &req_buf, req_len, 0);

		if (call_handle) {

			uint8_t *resp_buf;
			size_t resp_len;
			service_status_t service_status;

			pb_ostream_t ostream = pb_ostream_from_buffer(req_buf, req_len);
			pb_encode(&ostream, ts_crypto_VerifyHashIn_fields, &req_msg);

			m_client.rpc_status =
				rpc_caller_session_invoke(call_handle, opcode, &resp_buf, &resp_len,
							  &service_status);

			if (m_client.rpc_status == RPC_SUCCESS) psa_status = service_status;

			rpc_caller_session_end(call_handle);
		}
	}

	::free(hash_byte_array);
	::free(sig_byte_array);

	return psa_status;
}

psa_status_t protobuf_crypto_client::asymmetric_encrypt(psa_key_id_t id, psa_algorithm_t alg,
						const uint8_t *input, size_t input_length,
						const uint8_t *salt, size_t salt_length,
						uint8_t *output, size_t output_size, size_t *output_length)
{
	size_t req_len;
	pb_bytes_array_t *plaintext_byte_array =
		pb_malloc_byte_array_containing_bytes(input, input_length);
	pb_bytes_array_t *salt_byte_array = NULL;
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	ts_crypto_AsymmetricEncryptIn req_msg = ts_crypto_AsymmetricEncryptIn_init_default;

	*output_length = 0;  /* For failure case */

	req_msg.id = id;
	req_msg.alg = alg;
	req_msg.plaintext = pb_out_byte_array(plaintext_byte_array);

	if (salt && salt_length) {
		/* Optional parameter */
		salt_byte_array = pb_malloc_byte_array_containing_bytes(salt, salt_length);
		req_msg.salt = pb_out_byte_array(salt_byte_array);
	}

	if (pb_get_encoded_size(&req_len, ts_crypto_AsymmetricEncryptIn_fields, &req_msg)) {

		rpc_call_handle call_handle;
		uint8_t *req_buf;

		call_handle = rpc_caller_session_begin(m_client.session, &req_buf, req_len,
						       PB_PACKET_LENGTH(output_size));

		if (call_handle) {

			uint8_t *resp_buf;
			size_t resp_len;
			service_status_t service_status = PSA_ERROR_GENERIC_ERROR;

			pb_ostream_t ostream = pb_ostream_from_buffer(req_buf, req_len);
			pb_encode(&ostream, ts_crypto_AsymmetricEncryptIn_fields, &req_msg);

			m_client.rpc_status =
				rpc_caller_session_invoke(call_handle,
							  ts_crypto_Opcode_ASYMMETRIC_ENCRYPT,
							  &resp_buf, &resp_len, &service_status);

			if (m_client.rpc_status == RPC_SUCCESS) {

				psa_status = service_status;

				if (psa_status == PSA_SUCCESS) {

					pb_bytes_array_t *ciphertext_byte_array = pb_malloc_byte_array(output_size);
					ts_crypto_AsymmetricEncryptOut resp_msg =
						ts_crypto_AsymmetricEncryptOut_init_default;
					resp_msg.ciphertext = pb_in_byte_array(ciphertext_byte_array);

					pb_istream_t istream = pb_istream_from_buffer(resp_buf, resp_len);

					if (pb_decode(&istream, ts_crypto_AsymmetricEncryptOut_fields, &resp_msg)) {

						if (ciphertext_byte_array->size <= output_size) {

							memcpy(output,
								ciphertext_byte_array->bytes, ciphertext_byte_array->size);
							*output_length = ciphertext_byte_array->size;
						}
						else {
							/* Provided buffer is too small */
							psa_status = PSA_ERROR_BUFFER_TOO_SMALL;
						}
					}
					else {
						/* Failed to decode response message */
						psa_status = PSA_ERROR_GENERIC_ERROR;
					}

					::free(ciphertext_byte_array);
				}
			}

			rpc_caller_session_end(call_handle);
		}
	}

	::free(plaintext_byte_array);
	::free(salt_byte_array);

	return psa_status;
}

psa_status_t protobuf_crypto_client::asymmetric_decrypt(psa_key_id_t id, psa_algorithm_t alg,
						const uint8_t *input, size_t input_length,
						const uint8_t *salt, size_t salt_length,
						uint8_t *output, size_t output_size, size_t *output_length)
{
	size_t req_len;
	pb_bytes_array_t *ciphertext_byte_array =
		pb_malloc_byte_array_containing_bytes(input, input_length);
	pb_bytes_array_t *salt_byte_array = NULL;
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	ts_crypto_AsymmetricDecryptIn req_msg = ts_crypto_AsymmetricDecryptIn_init_default;

	*output_length = 0;  /* For failure case */

	req_msg.id = id;
	req_msg.alg = alg;
	req_msg.ciphertext = pb_out_byte_array(ciphertext_byte_array);

	if (salt && salt_length) {
		/* Optional parameter */
		salt_byte_array = pb_malloc_byte_array_containing_bytes(salt, salt_length);
		req_msg.salt = pb_out_byte_array(salt_byte_array);
	}

	if (pb_get_encoded_size(&req_len, ts_crypto_AsymmetricDecryptIn_fields, &req_msg)) {

		rpc_call_handle call_handle;
		uint8_t *req_buf;

		call_handle = rpc_caller_session_begin(m_client.session, &req_buf, req_len, 0);

		if (call_handle) {

			uint8_t *resp_buf;
			size_t resp_len;
			service_status_t service_status;

			pb_ostream_t ostream = pb_ostream_from_buffer(req_buf, req_len);
			pb_encode(&ostream, ts_crypto_AsymmetricDecryptIn_fields, &req_msg);

			m_client.rpc_status =
				rpc_caller_session_invoke(call_handle,
							  ts_crypto_Opcode_ASYMMETRIC_DECRYPT,
							  &resp_buf, &resp_len, &service_status);

			if (m_client.rpc_status == RPC_SUCCESS) {

				psa_status = service_status;

				if (psa_status == PSA_SUCCESS) {

					pb_bytes_array_t *plaintext_byte_array = pb_malloc_byte_array(output_size);
					ts_crypto_AsymmetricDecryptOut resp_msg =
						ts_crypto_AsymmetricDecryptOut_init_default;
					resp_msg.plaintext = pb_in_byte_array(plaintext_byte_array);

					pb_istream_t istream = pb_istream_from_buffer(resp_buf, resp_len);

					if (pb_decode(&istream, ts_crypto_AsymmetricDecryptOut_fields, &resp_msg)) {

						if (plaintext_byte_array->size <= output_size) {

							memcpy(output,
								plaintext_byte_array->bytes, plaintext_byte_array->size);
							*output_length = plaintext_byte_array->size;
						}
						else {
							/* Provided buffer is too small */
							m_client.rpc_status = PSA_ERROR_BUFFER_TOO_SMALL;
						}
					}
					else {
						/* Failed to decode response message */
						m_client.rpc_status = PSA_ERROR_GENERIC_ERROR;
					}

					::free(plaintext_byte_array);
				}
			}

			rpc_caller_session_end(call_handle);
		}
	}

	::free(ciphertext_byte_array);
	::free(salt_byte_array);

	return psa_status;
}

psa_status_t protobuf_crypto_client::generate_random(uint8_t *output, size_t output_size)
{
	size_t req_len;
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	ts_crypto_GenerateRandomIn req_msg = ts_crypto_GenerateRandomIn_init_default;

	req_msg.size = output_size;

	if (pb_get_encoded_size(&req_len, ts_crypto_GenerateRandomIn_fields, &req_msg)) {

		rpc_call_handle call_handle;
		uint8_t *req_buf;

		call_handle = rpc_caller_session_begin(m_client.session, &req_buf, req_len, output_size + 8);

		if (call_handle) {

			uint8_t *resp_buf;
			size_t resp_len;
			service_status_t service_status;

			pb_ostream_t ostream = pb_ostream_from_buffer(req_buf, req_len);
			pb_encode(&ostream, ts_crypto_GenerateRandomIn_fields, &req_msg);

			m_client.rpc_status =
				rpc_caller_session_invoke(call_handle,
							  ts_crypto_Opcode_GENERATE_RANDOM,
							  &resp_buf, &resp_len, &service_status);

			if (m_client.rpc_status == RPC_SUCCESS) {

				psa_status = service_status;

				if (psa_status == PSA_SUCCESS) {

					pb_bytes_array_t *output_byte_array = pb_malloc_byte_array(output_size);
					ts_crypto_GenerateRandomOut resp_msg = ts_crypto_GenerateRandomOut_init_default;
					resp_msg.random_bytes = pb_in_byte_array(output_byte_array);

					pb_istream_t istream = pb_istream_from_buffer(resp_buf, resp_len);

					if (pb_decode(&istream, ts_crypto_GenerateRandomOut_fields, &resp_msg)) {

						if (output_byte_array->size == output_size) {

							memcpy(output, output_byte_array->bytes, output_byte_array->size);
						}
						else {
							/* Mismatch between requested and generated length */
							psa_status = PSA_ERROR_GENERIC_ERROR;
						}
					}
					else {
						/* Failed to decode response message */
						psa_status = PSA_ERROR_GENERIC_ERROR;
					}

					::free(output_byte_array);
				}
			}

			rpc_caller_session_end(call_handle);
		}
	}

	return psa_status;
}

size_t protobuf_crypto_client::hash_max_update_size() const
{
	return 0;
}

psa_status_t protobuf_crypto_client::hash_setup(uint32_t *op_handle,
							psa_algorithm_t alg)
{
	(void)op_handle;
	(void)alg;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::hash_update(uint32_t op_handle,
							const uint8_t *input, size_t input_length)
{
	(void)op_handle;
	(void)input;
	(void)input_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::hash_finish(uint32_t op_handle,
							uint8_t *hash, size_t hash_size, size_t *hash_length)
{
	(void)op_handle;
	(void)hash;
	(void)hash_size;
	(void)hash_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::hash_abort(
	uint32_t op_handle)
{
	(void)op_handle;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::hash_verify(
	uint32_t op_handle,
	const uint8_t *hash, size_t hash_length)
{
	(void)op_handle;
	(void)hash;
	(void)hash_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::hash_clone(
	uint32_t source_op_handle,
	uint32_t *target_op_handle)
{
	(void)source_op_handle;
	(void)target_op_handle;

	return PSA_ERROR_NOT_SUPPORTED;
}

/* Cipher methods */
size_t protobuf_crypto_client::cipher_max_update_size() const
{
	return 0;
}

psa_status_t protobuf_crypto_client::cipher_encrypt_setup(
	uint32_t *op_handle,
	psa_key_id_t key,
	psa_algorithm_t alg)
{
	(void)op_handle;
	(void)key;
	(void)alg;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::cipher_decrypt_setup(
	uint32_t *op_handle,
	psa_key_id_t key,
	psa_algorithm_t alg)
{
	(void)op_handle;
	(void)key;
	(void)alg;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::cipher_generate_iv(
	uint32_t op_handle,
	uint8_t *iv, size_t iv_size, size_t *iv_length)
{
	(void)op_handle;
	(void)iv;
	(void)iv_size;
	(void)iv_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::cipher_set_iv(
	uint32_t op_handle,
	const uint8_t *iv, size_t iv_length)
{
	(void)op_handle;
	(void)iv;
	(void)iv_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::cipher_update(
	uint32_t op_handle,
	const uint8_t *input, size_t input_length,
	uint8_t *output, size_t output_size, size_t *output_length)
{
	(void)op_handle;
	(void)input;
	(void)input_length;
	(void)output;
	(void)output_size;
	(void)output_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::cipher_finish(
	uint32_t op_handle,
	uint8_t *output, size_t output_size, size_t *output_length)
{
	(void)op_handle;
	(void)output;
	(void)output_size;
	(void)output_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::cipher_abort(
	uint32_t op_handle)
{
	(void)op_handle;

	return PSA_ERROR_NOT_SUPPORTED;
}

/* MAC methods */
size_t protobuf_crypto_client::mac_max_update_size() const
{
	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::mac_sign_setup(
	uint32_t *op_handle,
	psa_key_id_t key,
	psa_algorithm_t alg)
{
	(void)op_handle;
	(void)key;
	(void)alg;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::mac_verify_setup(
	uint32_t *op_handle,
	psa_key_id_t key,
	psa_algorithm_t alg)
{
	(void)op_handle;
	(void)key;
	(void)alg;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::mac_update(
	uint32_t op_handle,
	const uint8_t *input, size_t input_length)
{
	(void)op_handle;
	(void)input;
	(void)input_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::mac_sign_finish(
	uint32_t op_handle,
	uint8_t *mac, size_t mac_size, size_t *mac_length)
{
	(void)op_handle;
	(void)mac;
	(void)mac_size;
	(void)mac_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::mac_verify_finish(
	uint32_t op_handle,
	const uint8_t *mac, size_t mac_length)
{
	(void)op_handle;
	(void)mac;
	(void)mac_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::mac_abort(
	uint32_t op_handle)
{
	(void)op_handle;

	return PSA_ERROR_NOT_SUPPORTED;
}

/* Key derivation methods */
psa_status_t protobuf_crypto_client::key_derivation_setup(
	uint32_t *op_handle,
	psa_algorithm_t alg)
{
	(void)op_handle;
	(void)alg;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::key_derivation_get_capacity(
	const uint32_t op_handle,
	size_t *capacity)
{
	(void)op_handle;
	(void)capacity;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::key_derivation_set_capacity(
	uint32_t op_handle,
	size_t capacity)
{
	(void)op_handle;
	(void)capacity;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::key_derivation_input_bytes(
	uint32_t op_handle,
	psa_key_derivation_step_t step,
	const uint8_t *data, size_t data_length)
{
	(void)op_handle;
	(void)step;
	(void)data;
	(void)data_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::key_derivation_input_key(
	uint32_t op_handle,
	psa_key_derivation_step_t step,
	psa_key_id_t key)
{
	(void)op_handle;
	(void)step;
	(void)key;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::key_derivation_output_bytes(
	uint32_t op_handle,
	uint8_t *output, size_t output_length)
{
	(void)op_handle;
	(void)output;
	(void)output_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::key_derivation_output_key(
	const psa_key_attributes_t *attributes,
	uint32_t op_handle,
	psa_key_id_t *key)
{
	(void)attributes;
	(void)op_handle;
	(void)key;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::key_derivation_abort(
	uint32_t op_handle)
{
	(void)op_handle;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::key_derivation_key_agreement(
	uint32_t op_handle,
	psa_key_derivation_step_t step,
	psa_key_id_t private_key,
	const uint8_t *peer_key, size_t peer_key_length)
{
	(void)op_handle;
	(void)step;
	(void)private_key;
	(void)peer_key;
	(void)peer_key_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t protobuf_crypto_client::raw_key_agreement(psa_algorithm_t alg,
	psa_key_id_t private_key,
	const uint8_t *peer_key, size_t peer_key_length,
	uint8_t *output, size_t output_size, size_t *output_length)
{
	(void)alg;
	(void)private_key;
	(void)peer_key;
	(void)peer_key_length;
	(void)output;
	(void)output_size;
	(void)output_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

int protobuf_crypto_client::verify_pkcs7_signature(const uint8_t *signature_cert,
						   uint64_t signature_cert_len, const uint8_t *hash,
						   uint64_t hash_len,
						   const uint8_t *public_key_cert,
						   uint64_t public_key_cert_len)
{
	(void)signature_cert;
	(void)signature_cert_len;
	(void)hash;
	(void)hash_len;
	(void)public_key_cert;
	(void)public_key_cert_len;

	return PSA_ERROR_NOT_SUPPORTED;
}

int protobuf_crypto_client::get_uefi_priv_auth_var_fingerprint(const uint8_t *signature_cert,
							       uint64_t signature_cert_len,
							       uint8_t *output)
{
	(void)signature_cert;
	(void)signature_cert_len;
	(void)output;

	return PSA_ERROR_NOT_SUPPORTED;
}
