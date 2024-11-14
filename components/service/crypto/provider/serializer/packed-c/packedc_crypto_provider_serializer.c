/*
 * Copyright (c) 2020-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "packedc_crypto_provider_serializer.h"

#include <common/tlv/tlv.h>
#include <protocols/rpc/common/packed-c/status.h>
#include <protocols/service/crypto/packed-c/asymmetric_decrypt.h>
#include <protocols/service/crypto/packed-c/asymmetric_encrypt.h>
#include <protocols/service/crypto/packed-c/copy_key.h>
#include <protocols/service/crypto/packed-c/destroy_key.h>
#include <protocols/service/crypto/packed-c/export_key.h>
#include <protocols/service/crypto/packed-c/export_public_key.h>
#include <protocols/service/crypto/packed-c/generate_key.h>
#include <protocols/service/crypto/packed-c/generate_random.h>
#include <protocols/service/crypto/packed-c/get_key_attributes.h>
#include <protocols/service/crypto/packed-c/import_key.h>
#include <protocols/service/crypto/packed-c/key_attributes.h>
#include <protocols/service/crypto/packed-c/purge_key.h>
#include <protocols/service/crypto/packed-c/sign_hash.h>
#include <protocols/service/crypto/packed-c/verify_hash.h>
#include <protocols/service/crypto/packed-c/verify_pkcs7_signature.h>
#include <protocols/service/crypto/packed-c/get_uefi_priv_auth_var_fingerprint.h>
#include <service/crypto/backend/crypto_backend.h>
#include <stdlib.h>
#include <string.h>

#include "packedc_key_attributes_translator.h"

/* Returns the maximum possible deserialized parameter size for a packed-c encoded message. */
static size_t max_deserialised_parameter_size(const struct rpc_buffer *req_buf)
{
	/*
	 * Assume that a deserialized parameter must be the same size or smaller than the
	 * entire serialized message.
	 */
	return req_buf->data_length;
}

/* Operation: generate_key */
static rpc_status_t deserialize_generate_key_req(const struct rpc_buffer *req_buf,
						 psa_key_attributes_t *attributes)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;
	struct ts_crypto_generate_key_in recv_msg;
	size_t expected_fixed_len = sizeof(struct ts_crypto_generate_key_in);

	if (expected_fixed_len <= req_buf->data_length) {
		memcpy(&recv_msg, req_buf->data, expected_fixed_len);
		packedc_crypto_provider_translate_key_attributes_from_proto(attributes,
									    &recv_msg.attributes);

		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

static rpc_status_t serialize_generate_key_resp(struct rpc_buffer *resp_buf, psa_key_id_t id)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct ts_crypto_generate_key_out resp_msg;
	size_t fixed_len = sizeof(struct ts_crypto_generate_key_out);

	resp_msg.id = id;

	if (fixed_len <= resp_buf->size) {
		memcpy(resp_buf->data, &resp_msg, fixed_len);
		resp_buf->data_length = fixed_len;
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

/* Operation: destroy_key */
static rpc_status_t deserialize_destroy_key_req(const struct rpc_buffer *req_buf, psa_key_id_t *id)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;
	struct ts_crypto_destroy_key_in recv_msg;
	size_t expected_fixed_len = sizeof(struct ts_crypto_destroy_key_in);

	if (expected_fixed_len <= req_buf->data_length) {
		memcpy(&recv_msg, req_buf->data, expected_fixed_len);
		*id = recv_msg.id;
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

/* Operation: export_key */
static rpc_status_t deserialize_export_key_req(const struct rpc_buffer *req_buf, psa_key_id_t *id)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;
	struct ts_crypto_export_key_in recv_msg;
	size_t expected_fixed_len = sizeof(struct ts_crypto_export_key_in);

	if (expected_fixed_len <= req_buf->data_length) {
		memcpy(&recv_msg, req_buf->data, expected_fixed_len);
		*id = recv_msg.id;
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

static rpc_status_t serialize_export_key_resp(struct rpc_buffer *resp_buf, const uint8_t *data,
					      size_t data_length)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct tlv_iterator resp_iter;
	struct tlv_record key_record;

	key_record.tag = TS_CRYPTO_EXPORT_KEY_OUT_TAG_DATA;
	key_record.length = data_length;
	key_record.value = data;

	tlv_iterator_begin(&resp_iter, resp_buf->data, resp_buf->size);

	if (tlv_encode(&resp_iter, &key_record)) {
		resp_buf->data_length = tlv_required_space(data_length);
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

/* Operation: export_public_key */
static rpc_status_t deserialize_export_public_key_req(const struct rpc_buffer *req_buf,
						      psa_key_id_t *id)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;
	struct ts_crypto_export_public_key_in recv_msg;
	size_t expected_fixed_len = sizeof(struct ts_crypto_export_public_key_in);

	if (expected_fixed_len <= req_buf->data_length) {
		memcpy(&recv_msg, req_buf->data, expected_fixed_len);
		*id = recv_msg.id;
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

static rpc_status_t serialize_export_public_key_resp(struct rpc_buffer *resp_buf,
						     const uint8_t *data, size_t data_length)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct tlv_iterator resp_iter;
	struct tlv_record key_record;

	key_record.tag = TS_CRYPTO_EXPORT_PUBLIC_KEY_OUT_TAG_DATA;
	key_record.length = data_length;
	key_record.value = data;

	tlv_iterator_begin(&resp_iter, resp_buf->data, resp_buf->size);

	if (tlv_encode(&resp_iter, &key_record)) {
		resp_buf->data_length = tlv_required_space(data_length);
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

/* Operation: import_key */
static rpc_status_t deserialize_import_key_req(const struct rpc_buffer *req_buf,
					       psa_key_attributes_t *attributes, uint8_t *data,
					       size_t *data_length)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;
	struct ts_crypto_import_key_in recv_msg;
	size_t expected_fixed_len = sizeof(struct ts_crypto_import_key_in);

	if (expected_fixed_len <= req_buf->data_length) {
		struct tlv_const_iterator req_iter;
		struct tlv_record decoded_record;

		rpc_status = RPC_SUCCESS;

		memcpy(&recv_msg, req_buf->data, expected_fixed_len);
		packedc_crypto_provider_translate_key_attributes_from_proto(attributes,
									    &recv_msg.attributes);

		tlv_const_iterator_begin(&req_iter, (uint8_t *)req_buf->data + expected_fixed_len,
					 req_buf->data_length - expected_fixed_len);

		if (tlv_find_decode(&req_iter, TS_CRYPTO_IMPORT_KEY_IN_TAG_DATA, &decoded_record)) {
			if (decoded_record.length <= *data_length) {
				memcpy(data, decoded_record.value, decoded_record.length);
				*data_length = decoded_record.length;
			} else {
				/* Buffer provided too small */
				return RPC_ERROR_INVALID_REQUEST_BODY;
			}
		} else {
			/* Default for missing parameter */
			*data_length = 0;
		}
	}

	return rpc_status;
}

static rpc_status_t serialize_import_key_resp(struct rpc_buffer *resp_buf, psa_key_id_t id)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct ts_crypto_import_key_out resp_msg;
	size_t fixed_len = sizeof(struct ts_crypto_import_key_out);

	resp_msg.id = id;

	if (fixed_len <= resp_buf->size) {
		memcpy(resp_buf->data, &resp_msg, fixed_len);
		resp_buf->data_length = fixed_len;
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

/* Operation: copy_key */
static rpc_status_t deserialize_copy_key_req(const struct rpc_buffer *req_buf,
					     psa_key_attributes_t *attributes,
					     psa_key_id_t *source_id)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;
	struct ts_crypto_copy_key_in recv_msg;
	size_t expected_fixed_len = sizeof(struct ts_crypto_copy_key_in);

	if (expected_fixed_len <= req_buf->data_length) {
		rpc_status = RPC_SUCCESS;

		memcpy(&recv_msg, req_buf->data, expected_fixed_len);
		packedc_crypto_provider_translate_key_attributes_from_proto(attributes,
									    &recv_msg.attributes);

		*source_id = recv_msg.source_key_id;
	}

	return rpc_status;
}

static rpc_status_t serialize_copy_key_resp(struct rpc_buffer *resp_buf, psa_key_id_t target_id)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct ts_crypto_copy_key_out resp_msg;
	size_t fixed_len = sizeof(struct ts_crypto_copy_key_out);

	resp_msg.target_key_id = target_id;

	if (fixed_len <= resp_buf->size) {
		memcpy(resp_buf->data, &resp_msg, fixed_len);
		resp_buf->data_length = fixed_len;
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

/* Operation: purge_key */
static rpc_status_t deserialize_purge_key_req(const struct rpc_buffer *req_buf, psa_key_id_t *id)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;
	struct ts_crypto_purge_key_in recv_msg;
	size_t expected_fixed_len = sizeof(struct ts_crypto_purge_key_in);

	if (expected_fixed_len <= req_buf->data_length) {
		memcpy(&recv_msg, req_buf->data, expected_fixed_len);
		*id = recv_msg.id;
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

/* Operation: get_key_attributes */
static rpc_status_t deserialize_get_key_attributes_req(const struct rpc_buffer *req_buf,
						       psa_key_id_t *id)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;
	struct ts_crypto_get_key_attributes_in recv_msg;
	size_t expected_fixed_len = sizeof(struct ts_crypto_get_key_attributes_in);

	if (expected_fixed_len <= req_buf->data_length) {
		memcpy(&recv_msg, req_buf->data, expected_fixed_len);
		*id = recv_msg.id;
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

static rpc_status_t serialize_get_key_attributes_resp(struct rpc_buffer *resp_buf,
						      const psa_key_attributes_t *attributes)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct ts_crypto_get_key_attributes_out resp_msg;
	size_t fixed_len = sizeof(struct ts_crypto_get_key_attributes_out);

	packedc_crypto_provider_translate_key_attributes_to_proto(&resp_msg.attributes, attributes);

	if (fixed_len <= resp_buf->size) {
		memcpy(resp_buf->data, &resp_msg, fixed_len);
		resp_buf->data_length = fixed_len;
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

/* Operation: sign_hash */
static rpc_status_t deserialize_asymmetric_sign_req(const struct rpc_buffer *req_buf,
						    psa_key_id_t *id, psa_algorithm_t *alg,
						    uint8_t *hash, size_t *hash_len)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;
	struct ts_crypto_sign_hash_in recv_msg;
	size_t expected_fixed_len = sizeof(struct ts_crypto_sign_hash_in);

	if (expected_fixed_len <= req_buf->data_length) {
		struct tlv_const_iterator req_iter;
		struct tlv_record decoded_record;

		rpc_status = RPC_SUCCESS;

		memcpy(&recv_msg, req_buf->data, expected_fixed_len);

		*id = recv_msg.id;
		*alg = recv_msg.alg;

		tlv_const_iterator_begin(&req_iter, (uint8_t *)req_buf->data + expected_fixed_len,
					 req_buf->data_length - expected_fixed_len);

		if (tlv_find_decode(&req_iter, TS_CRYPTO_SIGN_HASH_IN_TAG_HASH, &decoded_record)) {
			if (decoded_record.length <= *hash_len) {
				memcpy(hash, decoded_record.value, decoded_record.length);
				*hash_len = decoded_record.length;
			} else {
				/* Buffer provided too small */
				return RPC_ERROR_INVALID_REQUEST_BODY;
			}
		} else {
			/* Default to a zero length hash */
			*hash_len = 0;
		}
	}

	return rpc_status;
}

static rpc_status_t serialize_asymmetric_sign_resp(struct rpc_buffer *resp_buf, const uint8_t *sig,
						   size_t sig_len)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct tlv_iterator resp_iter;
	struct tlv_record sig_record;

	sig_record.tag = TS_CRYPTO_SIGN_HASH_OUT_TAG_SIGNATURE;
	sig_record.length = sig_len;
	sig_record.value = sig;

	tlv_iterator_begin(&resp_iter, resp_buf->data, resp_buf->size);

	if (tlv_encode(&resp_iter, &sig_record)) {
		resp_buf->data_length = tlv_required_space(sig_len);
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

/* Operation: verify_hash */
static rpc_status_t deserialize_asymmetric_verify_req(const struct rpc_buffer *req_buf,
						      psa_key_id_t *id, psa_algorithm_t *alg,
						      uint8_t *hash, size_t *hash_len, uint8_t *sig,
						      size_t *sig_len)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;
	struct ts_crypto_verify_hash_in recv_msg;
	size_t expected_fixed_len = sizeof(struct ts_crypto_verify_hash_in);

	if (expected_fixed_len <= req_buf->data_length) {
		struct tlv_const_iterator req_iter;
		struct tlv_record decoded_record;

		rpc_status = RPC_SUCCESS;

		memcpy(&recv_msg, req_buf->data, expected_fixed_len);

		*id = recv_msg.id;
		*alg = recv_msg.alg;

		tlv_const_iterator_begin(&req_iter, (uint8_t *)req_buf->data + expected_fixed_len,
					 req_buf->data_length - expected_fixed_len);

		if (tlv_find_decode(&req_iter, TS_CRYPTO_VERIFY_HASH_IN_TAG_HASH,
				    &decoded_record)) {
			if (decoded_record.length <= *hash_len) {
				memcpy(hash, decoded_record.value, decoded_record.length);
				*hash_len = decoded_record.length;
			} else {
				/* Buffer provided too small */
				return RPC_ERROR_INVALID_REQUEST_BODY;
			}
		} else {
			/* Default to a zero length hash */
			*hash_len = 0;
		}

		if (tlv_find_decode(&req_iter, TS_CRYPTO_VERIFY_HASH_IN_TAG_SIGNATURE,
				    &decoded_record)) {
			if (decoded_record.length <= *sig_len) {
				memcpy(sig, decoded_record.value, decoded_record.length);
				*sig_len = decoded_record.length;
			} else {
				/* Buffer provided too small */
				return RPC_ERROR_INVALID_REQUEST_BODY;
			}
		} else {
			/* Default to a zero length hash */
			*sig_len = 0;
		}
	}

	return rpc_status;
}

/* Operation: asymmetric_decrypt */
static rpc_status_t deserialize_asymmetric_decrypt_req(const struct rpc_buffer *req_buf,
						       psa_key_id_t *id, psa_algorithm_t *alg,
						       uint8_t *ciphertext, size_t *ciphertext_len,
						       uint8_t *salt, size_t *salt_len)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;
	struct ts_crypto_asymmetric_decrypt_in recv_msg;
	size_t expected_fixed_len = sizeof(struct ts_crypto_asymmetric_decrypt_in);

	if (expected_fixed_len <= req_buf->data_length) {
		struct tlv_const_iterator req_iter;
		struct tlv_record decoded_record;

		rpc_status = RPC_SUCCESS;

		memcpy(&recv_msg, req_buf->data, expected_fixed_len);

		*id = recv_msg.id;
		*alg = recv_msg.alg;

		tlv_const_iterator_begin(&req_iter, (uint8_t *)req_buf->data + expected_fixed_len,
					 req_buf->data_length - expected_fixed_len);

		if (tlv_find_decode(&req_iter, TS_CRYPTO_ASYMMETRIC_DECRYPT_IN_TAG_CIPHERTEXT,
				    &decoded_record)) {
			if (decoded_record.length <= *ciphertext_len) {
				memcpy(ciphertext, decoded_record.value, decoded_record.length);
				*ciphertext_len = decoded_record.length;
			} else {
				/* Buffer provided too small */
				return RPC_ERROR_INVALID_REQUEST_BODY;
			}
		} else {
			/* Default to a zero length hash */
			*ciphertext_len = 0;
		}

		if (tlv_find_decode(&req_iter, TS_CRYPTO_ASYMMETRIC_DECRYPT_IN_TAG_SALT,
				    &decoded_record)) {
			if (decoded_record.length <= *salt_len) {
				memcpy(salt, decoded_record.value, decoded_record.length);
				*salt_len = decoded_record.length;
			} else {
				/* Buffer provided too small */
				return RPC_ERROR_INVALID_REQUEST_BODY;
			}
		} else {
			/* Default to a zero length hash */
			*salt_len = 0;
		}
	}

	return rpc_status;
}

static rpc_status_t serialize_asymmetric_decrypt_resp(struct rpc_buffer *resp_buf,
						      const uint8_t *plaintext,
						      size_t plaintext_len)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct tlv_iterator resp_iter;
	struct tlv_record sig_record;

	sig_record.tag = TS_CRYPTO_ASYMMETRIC_DECRYPT_OUT_TAG_PLAINTEXT;
	sig_record.length = plaintext_len;
	sig_record.value = plaintext;

	tlv_iterator_begin(&resp_iter, resp_buf->data, resp_buf->size);

	if (tlv_encode(&resp_iter, &sig_record)) {
		resp_buf->data_length = tlv_required_space(plaintext_len);
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

/* Operation: asymmetric_encrypt */
static rpc_status_t deserialize_asymmetric_encrypt_req(const struct rpc_buffer *req_buf,
						       psa_key_id_t *id, psa_algorithm_t *alg,
						       uint8_t *plaintext, size_t *plaintext_len,
						       uint8_t *salt, size_t *salt_len)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;
	struct ts_crypto_asymmetric_encrypt_in recv_msg;
	size_t expected_fixed_len = sizeof(struct ts_crypto_asymmetric_encrypt_in);

	if (expected_fixed_len <= req_buf->data_length) {
		struct tlv_const_iterator req_iter;
		struct tlv_record decoded_record;

		rpc_status = RPC_SUCCESS;

		memcpy(&recv_msg, req_buf->data, expected_fixed_len);

		*id = recv_msg.id;
		*alg = recv_msg.alg;

		tlv_const_iterator_begin(&req_iter, (uint8_t *)req_buf->data + expected_fixed_len,
					 req_buf->data_length - expected_fixed_len);

		if (tlv_find_decode(&req_iter, TS_CRYPTO_ASYMMETRIC_ENCRYPT_IN_TAG_PLAINTEXT,
				    &decoded_record)) {
			if (decoded_record.length <= *plaintext_len) {
				memcpy(plaintext, decoded_record.value, decoded_record.length);
				*plaintext_len = decoded_record.length;
			} else {
				/* Buffer provided too small */
				return RPC_ERROR_INVALID_REQUEST_BODY;
			}
		} else {
			/* Default to a zero length hash */
			*plaintext_len = 0;
		}

		if (tlv_find_decode(&req_iter, TS_CRYPTO_ASYMMETRIC_ENCRYPT_IN_TAG_SALT,
				    &decoded_record)) {
			if (decoded_record.length <= *salt_len) {
				memcpy(salt, decoded_record.value, decoded_record.length);
				*salt_len = decoded_record.length;
			} else {
				/* Buffer provided too small */
				return RPC_ERROR_INVALID_REQUEST_BODY;
			}
		} else {
			/* Default to a zero length hash */
			*salt_len = 0;
		}
	}

	return rpc_status;
}

static rpc_status_t serialize_asymmetric_encrypt_resp(struct rpc_buffer *resp_buf,
						      const uint8_t *ciphertext,
						      size_t ciphertext_len)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct tlv_iterator resp_iter;
	struct tlv_record sig_record;

	sig_record.tag = TS_CRYPTO_ASYMMETRIC_ENCRYPT_OUT_TAG_CIPHERTEXT;
	sig_record.length = ciphertext_len;
	sig_record.value = ciphertext;

	tlv_iterator_begin(&resp_iter, resp_buf->data, resp_buf->size);

	if (tlv_encode(&resp_iter, &sig_record)) {
		resp_buf->data_length = tlv_required_space(ciphertext_len);
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

/* Operation: generate_random */
static rpc_status_t deserialize_generate_random_req(const struct rpc_buffer *req_buf, size_t *size)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;
	struct ts_crypto_generate_random_in recv_msg;
	size_t expected_fixed_len = sizeof(struct ts_crypto_generate_random_in);

	if (expected_fixed_len <= req_buf->data_length) {
		memcpy(&recv_msg, req_buf->data, expected_fixed_len);
		*size = recv_msg.size;
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

static rpc_status_t serialize_generate_random_resp(struct rpc_buffer *resp_buf,
						   const uint8_t *output, size_t output_len)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct tlv_iterator resp_iter;
	struct tlv_record out_record;

	out_record.tag = TS_CRYPTO_GENERATE_RANDOM_OUT_TAG_RANDOM_BYTES;
	out_record.length = output_len;
	out_record.value = output;

	tlv_iterator_begin(&resp_iter, resp_buf->data, resp_buf->size);

	if (tlv_encode(&resp_iter, &out_record)) {
		resp_buf->data_length = tlv_required_space(output_len);
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

/* Operation: mbedtls_verify_pkcs7_signature */
static rpc_status_t deserialize_verify_pkcs7_signature_req(
	struct rpc_buffer *req_buf, uint8_t *signature_cert, uint64_t *signature_cert_len,
	uint8_t *hash, uint64_t *hash_len, uint8_t *public_key_cert, uint64_t *public_key_cert_len)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;

	if (req_buf->data_length) {
		struct tlv_const_iterator req_iter;
		struct tlv_record decoded_record;

		rpc_status = RPC_SUCCESS;

		tlv_const_iterator_begin(&req_iter, (uint8_t *)req_buf->data, req_buf->data_length);

		if (tlv_find_decode(&req_iter, TS_CRYPTO_VERIFY_PKCS7_SIGNATURE_IN_TAG_SIGNATURE,
				    &decoded_record)) {
			*signature_cert_len = decoded_record.length;

			if (signature_cert)
				memcpy(signature_cert, decoded_record.value, decoded_record.length);
		} else {
			/* Default to a zero length */
			*signature_cert_len = 0;
		}

		if (tlv_find_decode(&req_iter, TS_CRYPTO_VERIFY_PKCS7_SIGNATURE_IN_TAG_HASH,
				    &decoded_record)) {
			*hash_len = decoded_record.length;

			if (hash)
				memcpy(hash, decoded_record.value, decoded_record.length);
		} else {
			/* Default to a zero length */
			*hash_len = 0;
		}

		if (tlv_find_decode(&req_iter,
				    TS_CRYPTO_VERIFY_PKCS7_SIGNATURE_IN_TAG_PUBLIC_KEY_CERT,
				    &decoded_record)) {
			*public_key_cert_len = decoded_record.length;

			if (public_key_cert)
				memcpy(public_key_cert, decoded_record.value,
				       decoded_record.length);
		} else {
			/* Default to a zero length */
			*public_key_cert_len = 0;
		}
	}

	return rpc_status;
}

/* Operation: get_uefi_priv_auth_var_fingerprintentifier */
static rpc_status_t deserialize_get_uefi_priv_auth_var_fingerprint_req(const struct rpc_buffer *req_buf,
							uint8_t *signed_data,
							uint64_t *signed_data_len)
{
	rpc_status_t rpc_status = RPC_ERROR_INVALID_REQUEST_BODY;

	if (req_buf->data_length) {
		struct tlv_const_iterator req_iter;
		struct tlv_record decoded_record;

		rpc_status = RPC_SUCCESS;

		tlv_const_iterator_begin(&req_iter, (uint8_t *)req_buf->data, req_buf->data_length);

		if (tlv_find_decode(&req_iter, TS_CRYPTO_GET_UEFI_PRIV_AUTH_VAR_FINGERPRINT_IN_TAG_SIGNATURE,
				    &decoded_record)) {
			*signed_data_len = decoded_record.length;

			if (signed_data)
				memcpy(signed_data, decoded_record.value, decoded_record.length);
		} else {
			/* Default to a zero length */
			*signed_data_len = 0;
		}
	}

	return rpc_status;
}

static rpc_status_t serialize_get_uefi_priv_auth_var_fingerprint_resp(struct rpc_buffer *resp_buf,
							const uint8_t *output)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct tlv_iterator resp_iter;
	struct tlv_record out_record;

	out_record.tag = TS_CRYPTO_GET_UEFI_PRIV_AUTH_VAR_FINGERPRINT_OUT_TAG_IDENTIFIER;
	out_record.length = PSA_HASH_MAX_SIZE;
	out_record.value = output;

	tlv_iterator_begin(&resp_iter, resp_buf->data, resp_buf->size);

	if (tlv_encode(&resp_iter, &out_record)) {
		resp_buf->data_length = tlv_required_space(PSA_HASH_MAX_SIZE);
		rpc_status = RPC_SUCCESS;
	}

	return rpc_status;
}

/* Singleton method to provide access to the serializer instance */
const struct crypto_provider_serializer *packedc_crypto_provider_serializer_instance(void)
{
	static const struct crypto_provider_serializer instance = {
		max_deserialised_parameter_size,
		deserialize_generate_key_req,
		serialize_generate_key_resp,
		deserialize_destroy_key_req,
		deserialize_export_key_req,
		serialize_export_key_resp,
		deserialize_export_public_key_req,
		serialize_export_public_key_resp,
		deserialize_import_key_req,
		serialize_import_key_resp,
		deserialize_copy_key_req,
		serialize_copy_key_resp,
		deserialize_purge_key_req,
		deserialize_get_key_attributes_req,
		serialize_get_key_attributes_resp,
		deserialize_asymmetric_sign_req,
		serialize_asymmetric_sign_resp,
		deserialize_asymmetric_verify_req,
		deserialize_asymmetric_decrypt_req,
		serialize_asymmetric_decrypt_resp,
		deserialize_asymmetric_encrypt_req,
		serialize_asymmetric_encrypt_resp,
		deserialize_generate_random_req,
		serialize_generate_random_resp,
		deserialize_verify_pkcs7_signature_req,
		deserialize_get_uefi_priv_auth_var_fingerprint_req,
		serialize_get_uefi_priv_auth_var_fingerprint_resp
	};

	return &instance;
}
