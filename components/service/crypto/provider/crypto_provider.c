/*
 * Copyright (c) 2020-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <protocols/common/efi/efi_status.h>
#include <protocols/rpc/common/packed-c/status.h>
#include <protocols/service/crypto/packed-c/opcodes.h>
#include <service/crypto/backend/crypto_backend.h>
#include <service/crypto/provider/crypto_provider.h>
#include <compiler.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "crypto_partition.h"
#include "crypto_uuid.h"

#if defined(MBEDTLS_PKCS7_C) && defined(MBEDTLS_X509_CRT_PARSE_C)
#include "common/mbedtls/mbedtls_utils.h"
#endif

/* Service request handlers */
static rpc_status_t generate_key_handler(void *context, struct rpc_request *req);
static rpc_status_t destroy_key_handler(void *context, struct rpc_request *req);
static rpc_status_t export_key_handler(void *context, struct rpc_request *req);
static rpc_status_t export_public_key_handler(void *context, struct rpc_request *req);
static rpc_status_t import_key_handler(void *context, struct rpc_request *req);
static rpc_status_t asymmetric_sign_handler(void *context, struct rpc_request *req);
static rpc_status_t asymmetric_verify_handler(void *context, struct rpc_request *req);
static rpc_status_t asymmetric_decrypt_handler(void *context, struct rpc_request *req);
static rpc_status_t asymmetric_encrypt_handler(void *context, struct rpc_request *req);
static rpc_status_t generate_random_handler(void *context, struct rpc_request *req);
static rpc_status_t copy_key_handler(void *context, struct rpc_request *req);
static rpc_status_t purge_key_handler(void *context, struct rpc_request *req);
static rpc_status_t get_key_attributes_handler(void *context, struct rpc_request *req);
static rpc_status_t verify_pkcs7_signature_handler(void *context, struct rpc_request *req);
static rpc_status_t get_uefi_priv_auth_var_fingerprint_handler(void *context, struct rpc_request *req);

/* Handler mapping table for service */
static const struct service_handler handler_table[] = {
	{ TS_CRYPTO_OPCODE_GENERATE_KEY,            generate_key_handler },
	{ TS_CRYPTO_OPCODE_DESTROY_KEY,             destroy_key_handler },
	{ TS_CRYPTO_OPCODE_EXPORT_KEY,              export_key_handler },
	{ TS_CRYPTO_OPCODE_EXPORT_PUBLIC_KEY,       export_public_key_handler },
	{ TS_CRYPTO_OPCODE_IMPORT_KEY,              import_key_handler },
	{ TS_CRYPTO_OPCODE_SIGN_HASH,               asymmetric_sign_handler },
	{ TS_CRYPTO_OPCODE_VERIFY_HASH,             asymmetric_verify_handler },
	{ TS_CRYPTO_OPCODE_ASYMMETRIC_DECRYPT,      asymmetric_decrypt_handler },
	{ TS_CRYPTO_OPCODE_ASYMMETRIC_ENCRYPT,      asymmetric_encrypt_handler },
	{ TS_CRYPTO_OPCODE_GENERATE_RANDOM,         generate_random_handler },
	{ TS_CRYPTO_OPCODE_COPY_KEY,                copy_key_handler },
	{ TS_CRYPTO_OPCODE_PURGE_KEY,               purge_key_handler },
	{ TS_CRYPTO_OPCODE_GET_KEY_ATTRIBUTES,      get_key_attributes_handler },
	{ TS_CRYPTO_OPCODE_SIGN_MESSAGE,            asymmetric_sign_handler },
	{ TS_CRYPTO_OPCODE_VERIFY_MESSAGE,          asymmetric_verify_handler },
	{ TS_CRYPTO_OPCODE_VERIFY_PKCS7_SIGNATURE,  verify_pkcs7_signature_handler },
	{ TS_CRYPTO_OPCODE_GET_UEFI_PRIV_AUTH_VAR_FINGERPRINT, get_uefi_priv_auth_var_fingerprint_handler },
};

struct rpc_service_interface *
crypto_provider_init(struct crypto_provider *context, unsigned int encoding,
		     const struct crypto_provider_serializer *serializer)
{
	const struct rpc_uuid crypto_service_uuid[2] = {
		{ .uuid = TS_PSA_CRYPTO_SERVICE_UUID },
		{ .uuid = TS_PSA_CRYPTO_PROTOBUF_SERVICE_UUID },
	};

	if (encoding >= TS_RPC_ENCODING_LIMIT)
		return NULL;

	context->serializer = serializer;

	service_provider_init(&context->base_provider, context, &crypto_service_uuid[encoding],
			      handler_table,
			      sizeof(handler_table) / sizeof(struct service_handler));

	return service_provider_get_rpc_interface(&context->base_provider);
}

void crypto_provider_deinit(struct crypto_provider *context)
{
	(void)context;
}

void crypto_provider_register_serializer(struct crypto_provider *context,
					 const struct crypto_provider_serializer *serializer)
{
	context->serializer = serializer;
}

void crypto_provider_extend(struct crypto_provider *context, struct service_provider *sub_provider)
{
	service_provider_extend(&context->base_provider, sub_provider);
}

static const struct crypto_provider_serializer *get_crypto_serializer(void *context,
								      const struct rpc_request *req)
{
	struct crypto_provider *this_instance = (struct crypto_provider *)context;

	return this_instance->serializer;
}

static rpc_status_t generate_key_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct crypto_provider_serializer *serializer = get_crypto_serializer(context, req);

	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

	if (serializer)
		rpc_status = serializer->deserialize_generate_key_req(req_buf, &attributes);

	if (rpc_status == RPC_SUCCESS) {
		psa_status_t psa_status;
		namespaced_key_id_t ns_key_id = NAMESPACED_KEY_ID_INIT;

		crypto_partition_bind_to_owner(&attributes, req->source_id);

		psa_status = psa_generate_key(&attributes, &ns_key_id);

		if (psa_status == PSA_SUCCESS) {
			psa_key_id_t key_id = namespaced_key_id_get_key_id(ns_key_id);
			struct rpc_buffer *resp_buf = &req->response;
			rpc_status = serializer->serialize_generate_key_resp(resp_buf, key_id);
		}

		req->service_status = psa_status;
	}

	psa_reset_key_attributes(&attributes);

	return rpc_status;
}

static rpc_status_t destroy_key_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct crypto_provider_serializer *serializer = get_crypto_serializer(context, req);

	psa_key_id_t key_id = PSA_KEY_ID_NULL;

	if (serializer)
		rpc_status = serializer->deserialize_destroy_key_req(req_buf, &key_id);

	if (rpc_status == RPC_SUCCESS) {
		psa_status_t psa_status;
		namespaced_key_id_t ns_key_id =
			crypto_partition_get_namespaced_key_id(req->source_id, key_id);

		psa_status = psa_destroy_key(ns_key_id);
		req->service_status = psa_status;
	}

	return rpc_status;
}

static rpc_status_t export_key_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct crypto_provider_serializer *serializer = get_crypto_serializer(context, req);

	psa_key_id_t key_id = PSA_KEY_ID_NULL;

	if (serializer)
		rpc_status = serializer->deserialize_export_key_req(req_buf, &key_id);

	if (rpc_status == RPC_SUCCESS) {
		namespaced_key_id_t ns_key_id =
			crypto_partition_get_namespaced_key_id(req->source_id, key_id);
		size_t max_export_size = PSA_EXPORT_KEY_PAIR_MAX_SIZE;
		uint8_t *key_buffer = malloc(max_export_size);

		if (key_buffer) {
			size_t export_size;
			psa_status_t psa_status =
				psa_export_key(ns_key_id, key_buffer, max_export_size, &export_size);

			if (psa_status == PSA_SUCCESS) {
				struct rpc_buffer *resp_buf = &req->response;

				rpc_status = serializer->serialize_export_key_resp(
					resp_buf, key_buffer, export_size);
			}

			free(key_buffer);
			req->service_status = psa_status;
		} else {
			/* Failed to allocate key buffer */
			rpc_status = RPC_ERROR_RESOURCE_FAILURE;
		}
	}

	return rpc_status;
}

static rpc_status_t export_public_key_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct crypto_provider_serializer *serializer = get_crypto_serializer(context, req);

	psa_key_id_t key_id = PSA_KEY_ID_NULL;

	if (serializer)
		rpc_status = serializer->deserialize_export_public_key_req(req_buf, &key_id);

	if (rpc_status == RPC_SUCCESS) {
		namespaced_key_id_t ns_key_id =
			crypto_partition_get_namespaced_key_id(req->source_id, key_id);
		size_t max_export_size = PSA_EXPORT_PUBLIC_KEY_MAX_SIZE;
		uint8_t *key_buffer = malloc(max_export_size);

		if (key_buffer) {
			size_t export_size;
			psa_status_t psa_status = psa_export_public_key(
				ns_key_id, key_buffer, max_export_size, &export_size);

			if (psa_status == PSA_SUCCESS) {
				struct rpc_buffer *resp_buf = &req->response;

				rpc_status = serializer->serialize_export_public_key_resp(
					resp_buf, key_buffer, export_size);
			}

			free(key_buffer);
			req->service_status = psa_status;
		} else {
			/* Failed to allocate key buffer */
			rpc_status = RPC_ERROR_RESOURCE_FAILURE;
		}
	}

	return rpc_status;
}

static rpc_status_t import_key_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct crypto_provider_serializer *serializer = get_crypto_serializer(context, req);

	if (serializer) {
		size_t key_data_len = serializer->max_deserialised_parameter_size(req_buf);
		uint8_t *key_buffer = malloc(key_data_len);

		if (key_buffer) {
			psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
			rpc_status = serializer->deserialize_import_key_req(
				req_buf, &attributes, key_buffer, &key_data_len);

			if (rpc_status == RPC_SUCCESS) {
				psa_status_t psa_status;
				namespaced_key_id_t ns_key_id = NAMESPACED_KEY_ID_INIT;

				crypto_partition_bind_to_owner(&attributes, req->source_id);

				psa_status =
					psa_import_key(&attributes, key_buffer, key_data_len, &ns_key_id);

				if (psa_status == PSA_SUCCESS) {
					psa_key_id_t key_id =
						namespaced_key_id_get_key_id(ns_key_id);
					struct rpc_buffer *resp_buf = &req->response;

					rpc_status =
						serializer->serialize_import_key_resp(resp_buf, key_id);
				}

				req->service_status = psa_status;
			}

			psa_reset_key_attributes(&attributes);
			free(key_buffer);
		} else {
			rpc_status = RPC_ERROR_RESOURCE_FAILURE;
		}
	}

	return rpc_status;
}

static rpc_status_t asymmetric_sign_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct crypto_provider_serializer *serializer = get_crypto_serializer(context, req);

	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_algorithm_t alg;
	size_t hash_len = PSA_HASH_MAX_SIZE;
	uint8_t hash_buffer[PSA_HASH_MAX_SIZE];

	if (serializer)
		rpc_status = serializer->deserialize_asymmetric_sign_req(req_buf, &key_id, &alg,
									 hash_buffer, &hash_len);

	if (rpc_status == RPC_SUCCESS) {
		namespaced_key_id_t ns_key_id =
			crypto_partition_get_namespaced_key_id(req->source_id, key_id);
		psa_status_t psa_status;
		size_t sig_len;
		uint8_t sig_buffer[PSA_SIGNATURE_MAX_SIZE];

		psa_status = (req->opcode == TS_CRYPTO_OPCODE_SIGN_HASH) ?
			psa_sign_hash(ns_key_id, alg, hash_buffer, hash_len, sig_buffer, sizeof(sig_buffer), &sig_len) :
			psa_sign_message(ns_key_id, alg, hash_buffer, hash_len, sig_buffer, sizeof(sig_buffer), &sig_len);

		if (psa_status == PSA_SUCCESS) {
			struct rpc_buffer *resp_buf = &req->response;

			rpc_status = serializer->serialize_asymmetric_sign_resp(
				resp_buf, sig_buffer, sig_len);
		}

		req->service_status = psa_status;
	}

	return rpc_status;
}

static rpc_status_t asymmetric_verify_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct crypto_provider_serializer *serializer = get_crypto_serializer(context, req);

	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_algorithm_t alg;
	size_t hash_len = PSA_HASH_MAX_SIZE;
	uint8_t hash_buffer[PSA_HASH_MAX_SIZE];
	size_t sig_len = PSA_SIGNATURE_MAX_SIZE;
	uint8_t sig_buffer[PSA_SIGNATURE_MAX_SIZE];

	if (serializer)
		rpc_status = serializer->deserialize_asymmetric_verify_req(
			req_buf, &key_id, &alg, hash_buffer, &hash_len, sig_buffer, &sig_len);

	if (rpc_status == RPC_SUCCESS) {
		psa_status_t psa_status;
		namespaced_key_id_t ns_key_id =
			crypto_partition_get_namespaced_key_id(req->source_id, key_id);

		psa_status = (req->opcode == TS_CRYPTO_OPCODE_VERIFY_HASH) ?
			psa_verify_hash(ns_key_id, alg, hash_buffer, hash_len, sig_buffer, sig_len) :
			psa_verify_message(ns_key_id, alg, hash_buffer, hash_len, sig_buffer, sig_len);

		req->service_status = psa_status;
	}

	return rpc_status;
}

static rpc_status_t asymmetric_decrypt_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct crypto_provider_serializer *serializer = get_crypto_serializer(context, req);

	if (serializer) {
		size_t max_param_size = serializer->max_deserialised_parameter_size(req_buf);

		psa_key_id_t key_id = PSA_KEY_ID_NULL;
		psa_algorithm_t alg;
		size_t ciphertext_len = max_param_size;
		uint8_t *ciphertext_buffer = malloc(ciphertext_len);
		size_t salt_len = max_param_size;
		uint8_t *salt_buffer = malloc(salt_len);

		if (ciphertext_buffer && salt_buffer) {
			rpc_status = serializer->deserialize_asymmetric_decrypt_req(
				req_buf, &key_id, &alg, ciphertext_buffer, &ciphertext_len, salt_buffer,
				&salt_len);

			if (rpc_status == RPC_SUCCESS) {
				psa_status_t psa_status;
				psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
				namespaced_key_id_t ns_key_id =
					crypto_partition_get_namespaced_key_id(req->source_id, key_id);

				psa_status = psa_get_key_attributes(ns_key_id, &attributes);

				if (psa_status == PSA_SUCCESS) {
					size_t max_decrypt_size =
						PSA_ASYMMETRIC_DECRYPT_OUTPUT_SIZE(
							psa_get_key_type(&attributes),
							psa_get_key_bits(&attributes), alg);

					size_t plaintext_len;
					uint8_t *plaintext_buffer = malloc(max_decrypt_size);

					if (plaintext_buffer) {
						/* Salt is an optional parameter */
						uint8_t *salt = (salt_len) ? salt_buffer : NULL;

						psa_status = psa_asymmetric_decrypt(
							ns_key_id, alg, ciphertext_buffer, ciphertext_len,
							salt, salt_len, plaintext_buffer,
							max_decrypt_size, &plaintext_len);

						if (psa_status == PSA_SUCCESS) {
							struct rpc_buffer *resp_buf =
								&req->response;
							rpc_status = serializer->serialize_asymmetric_decrypt_resp(
								resp_buf, plaintext_buffer, plaintext_len);
						}

						free(plaintext_buffer);
					} else {
						/* Failed to allocate ouptput buffer */
						rpc_status = RPC_ERROR_RESOURCE_FAILURE;
					}
				}

				req->service_status = psa_status;
				psa_reset_key_attributes(&attributes);
			}
		} else {
			/* Failed to allocate buffers */
			rpc_status = RPC_ERROR_RESOURCE_FAILURE;
		}

		free(ciphertext_buffer);
		free(salt_buffer);
	}

	return rpc_status;
}

static rpc_status_t asymmetric_encrypt_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct crypto_provider_serializer *serializer = get_crypto_serializer(context, req);

	if (serializer) {
		size_t max_param_size = serializer->max_deserialised_parameter_size(req_buf);

		psa_key_id_t key_id = PSA_KEY_ID_NULL;
		psa_algorithm_t alg;
		size_t plaintext_len = max_param_size;
		uint8_t *plaintext_buffer = malloc(plaintext_len);
		size_t salt_len = max_param_size;
		uint8_t *salt_buffer = malloc(salt_len);

		if (plaintext_buffer && salt_buffer) {
			rpc_status = serializer->deserialize_asymmetric_encrypt_req(
				req_buf, &key_id, &alg, plaintext_buffer, &plaintext_len, salt_buffer,
				&salt_len);

			if (rpc_status == RPC_SUCCESS) {
				psa_status_t psa_status;
				psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
				namespaced_key_id_t ns_key_id =
					crypto_partition_get_namespaced_key_id(req->source_id, key_id);

				psa_status = psa_get_key_attributes(ns_key_id, &attributes);

				if (psa_status == PSA_SUCCESS) {
					size_t max_encrypt_size =
						PSA_ASYMMETRIC_ENCRYPT_OUTPUT_SIZE(
							psa_get_key_type(&attributes),
							psa_get_key_bits(&attributes), alg);

					size_t ciphertext_len;
					uint8_t *ciphertext_buffer = malloc(max_encrypt_size);

					if (ciphertext_buffer) {
						/* Salt is an optional parameter */
						uint8_t *salt = (salt_len) ? salt_buffer : NULL;

						psa_status = psa_asymmetric_encrypt(
							ns_key_id, alg, plaintext_buffer, plaintext_len,
							salt, salt_len, ciphertext_buffer,
							max_encrypt_size, &ciphertext_len);

						if (psa_status == PSA_SUCCESS) {
							struct rpc_buffer *resp_buf =
								&req->response;

							rpc_status = serializer->serialize_asymmetric_encrypt_resp(
								resp_buf, ciphertext_buffer, ciphertext_len);
						}

						free(ciphertext_buffer);
					} else {
						/* Failed to allocate ouptput buffer */
						rpc_status = RPC_ERROR_RESOURCE_FAILURE;
					}
				}

				req->service_status = psa_status;
				psa_reset_key_attributes(&attributes);
			}
		} else {
			/* Failed to allocate buffers */
			rpc_status = RPC_ERROR_RESOURCE_FAILURE;
		}

		free(plaintext_buffer);
		free(salt_buffer);
	}

	return rpc_status;
}

static rpc_status_t generate_random_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct crypto_provider_serializer *serializer = get_crypto_serializer(context, req);

	size_t output_size;

	if (serializer)
		rpc_status = serializer->deserialize_generate_random_req(req_buf, &output_size);

	if (rpc_status == RPC_SUCCESS) {
		psa_status_t psa_status;
		uint8_t *output_buffer = malloc(output_size);

		if (output_buffer) {
			psa_status = psa_generate_random(output_buffer, output_size);

			if (psa_status == PSA_SUCCESS) {
				struct rpc_buffer *resp_buf = &req->response;

				rpc_status = serializer->serialize_generate_random_resp(
					resp_buf, output_buffer, output_size);
			}

			req->service_status = psa_status;
			free(output_buffer);
		} else {
			/* Failed to allocate output buffer */
			rpc_status = RPC_ERROR_RESOURCE_FAILURE;
		}
	}

	return rpc_status;
}

static rpc_status_t copy_key_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct crypto_provider_serializer *serializer = get_crypto_serializer(context, req);

	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t source_key_id = PSA_KEY_ID_NULL;

	if (serializer)
		rpc_status =
			serializer->deserialize_copy_key_req(req_buf, &attributes, &source_key_id);

	if (rpc_status == RPC_SUCCESS) {
		namespaced_key_id_t target_ns_key_id = NAMESPACED_KEY_ID_INIT;
		namespaced_key_id_t source_ns_key_id =
			crypto_partition_get_namespaced_key_id(req->source_id, source_key_id);

		crypto_partition_bind_to_owner(&attributes, req->source_id);

		psa_status_t psa_status = psa_copy_key(source_ns_key_id, &attributes, &target_ns_key_id);

		if (psa_status == PSA_SUCCESS) {
			psa_key_id_t target_key_id = namespaced_key_id_get_key_id(target_ns_key_id);
			struct rpc_buffer *resp_buf = &req->response;
			rpc_status = serializer->serialize_copy_key_resp(resp_buf, target_key_id);
		}

		req->service_status = psa_status;
	}

	psa_reset_key_attributes(&attributes);

	return rpc_status;
}

static rpc_status_t purge_key_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct crypto_provider_serializer *serializer = get_crypto_serializer(context, req);

	psa_key_id_t key_id = PSA_KEY_ID_NULL;

	if (serializer)
		rpc_status = serializer->deserialize_purge_key_req(req_buf, &key_id);

	if (rpc_status == RPC_SUCCESS) {
		namespaced_key_id_t ns_key_id =
			crypto_partition_get_namespaced_key_id(req->source_id, key_id);
		psa_status_t psa_status = psa_purge_key(ns_key_id);
		req->service_status = psa_status;
	}

	return rpc_status;
}

static rpc_status_t get_key_attributes_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct crypto_provider_serializer *serializer = get_crypto_serializer(context, req);

	psa_key_id_t key_id = PSA_KEY_ID_NULL;

	if (serializer)
		rpc_status = serializer->deserialize_get_key_attributes_req(req_buf, &key_id);

	if (rpc_status == RPC_SUCCESS) {
		psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
		namespaced_key_id_t ns_key_id =
			crypto_partition_get_namespaced_key_id(req->source_id, key_id);

		psa_status_t psa_status = psa_get_key_attributes(ns_key_id, &attributes);

		if (psa_status == PSA_SUCCESS) {
			struct rpc_buffer *resp_buf = &req->response;

			rpc_status = serializer->serialize_get_key_attributes_resp(resp_buf,
										   &attributes);
		}

		psa_reset_key_attributes(&attributes);
		req->service_status = psa_status;
	}

	return rpc_status;
}

#if defined(MBEDTLS_PKCS7_C) && defined(MBEDTLS_X509_CRT_PARSE_C)
static rpc_status_t verify_pkcs7_signature_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct crypto_provider_serializer *serializer = get_crypto_serializer(context, req);

	int mbedtls_status = MBEDTLS_ERR_PKCS7_VERIFY_FAIL;

	uint8_t *signature_cert = NULL;
	uint64_t signature_cert_len = 0;
	uint8_t *hash = NULL;
	uint64_t hash_len = 0;
	uint8_t *public_key_cert = NULL;
	uint64_t public_key_cert_len = 0;

	if (serializer) {
		/* First collect the lengths of the fields */
		rpc_status = serializer->deserialize_verify_pkcs7_signature_req(
			req_buf, NULL, &signature_cert_len, NULL, &hash_len, NULL,
			&public_key_cert_len);

		if (rpc_status == RPC_SUCCESS) {
			/* Allocate the needed space and get the data */
			signature_cert = (uint8_t *)malloc(signature_cert_len);
			hash = (uint8_t *)malloc(hash_len);
			public_key_cert = (uint8_t *)malloc(public_key_cert_len);

			if (signature_cert && hash && public_key_cert) {
				rpc_status = serializer->deserialize_verify_pkcs7_signature_req(
					req_buf, signature_cert, &signature_cert_len, hash,
					&hash_len, public_key_cert, &public_key_cert_len);
			} else {
				rpc_status = RPC_ERROR_RESOURCE_FAILURE;
			}
		}
	}

	if (rpc_status == RPC_SUCCESS) {
		/* Parse the PKCS#7 DER encoded signature block */
		mbedtls_pkcs7 pkcs7_structure;

		mbedtls_pkcs7_init(&pkcs7_structure);

		mbedtls_status = mbedtls_pkcs7_parse_der(&pkcs7_structure, signature_cert,
								signature_cert_len);

		if (mbedtls_status == MBEDTLS_PKCS7_SIGNED_DATA) {

			/*
			 * If a separate public key is provided, verify the signature with it,
			 * else use the key from the pkcs7 signature structure, because it is
			 * a self-signed certificate.
			 */
			if(public_key_cert_len) {
				/* Parse the public key certificate */
				mbedtls_x509_crt signer_certificate;

				mbedtls_x509_crt_init(&signer_certificate);

				mbedtls_status = mbedtls_x509_crt_parse_der(&signer_certificate, public_key_cert,
									public_key_cert_len);

				if (mbedtls_status == 0) {
					/* Verify hash against signed hash */
					mbedtls_status = mbedtls_pkcs7_signed_hash_verify(
						&pkcs7_structure, &signer_certificate, hash, hash_len);
				}

				mbedtls_x509_crt_free(&signer_certificate);
			} else {
				mbedtls_status = mbedtls_pkcs7_signed_hash_verify(
					&pkcs7_structure, &pkcs7_structure.private_signed_data.private_certs, hash, hash_len);
			}
		}

		mbedtls_pkcs7_free(&pkcs7_structure);
	}

	free(signature_cert);
	free(hash);
	free(public_key_cert);

	/* Provide the result of the verification */
	req->service_status = mbedtls_status;

	return rpc_status;
}

static rpc_status_t get_uefi_priv_auth_var_fingerprint_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct crypto_provider_serializer *serializer = get_crypto_serializer(context, req);

	int mbedtls_status = MBEDTLS_ERR_PKCS7_VERIFY_FAIL;

	uint8_t *signature_cert = NULL;
	uint64_t signature_cert_len = 0;

	if (serializer) {
		/* First collect the lengths of the field */
		rpc_status = serializer->deserialize_get_uefi_priv_auth_var_fingerprint_req(
			req_buf, NULL, &signature_cert_len);

		if (rpc_status == RPC_SUCCESS) {
			/* Allocate the needed space and get the data */
			signature_cert = (uint8_t *)malloc(signature_cert_len);

			if (signature_cert) {
				rpc_status = serializer->deserialize_get_uefi_priv_auth_var_fingerprint_req(
					req_buf, signature_cert, &signature_cert_len);
			} else {
				rpc_status = RPC_ERROR_RESOURCE_FAILURE;
			}
		}
	}

	if (rpc_status == RPC_SUCCESS) {
		/* Parse the PKCS#7 DER encoded signature block */
		mbedtls_pkcs7 pkcs7_structure;

		mbedtls_pkcs7_init(&pkcs7_structure);

		mbedtls_status = mbedtls_pkcs7_parse_der(&pkcs7_structure, signature_cert,
								signature_cert_len);

		if (mbedtls_status == MBEDTLS_PKCS7_SIGNED_DATA) {

			uint8_t output_buffer[PSA_HASH_MAX_SIZE] =  { 0 };
			size_t __maybe_unused output_size = 0;
			const mbedtls_asn1_buf *signerCertCN = NULL;
			const mbedtls_x509_crt *topLevelCert = &pkcs7_structure.private_signed_data.private_certs;
			const mbedtls_x509_buf *toplevelCertTbs = NULL;
			struct rpc_buffer *resp_buf = &req->response;;
			psa_hash_operation_t op = PSA_HASH_OPERATION_INIT;

			/* Find common name field of the signing certificate, which is the first in the chain */
			signerCertCN = findCommonName(&topLevelCert->subject);
			if (!signerCertCN) {
				mbedtls_status = MBEDTLS_ERR_PKCS7_VERIFY_FAIL;
				goto end;
			}

			/* Get the TopLevel certificate which is the last in the chain */
			while(topLevelCert->next)
				topLevelCert = topLevelCert->next;
			toplevelCertTbs = &topLevelCert->tbs;

			/* Hash the data to create the fingerprint */
			op = psa_hash_operation_init();

			if (psa_hash_setup(&op, PSA_ALG_SHA_256) != PSA_SUCCESS) {
				mbedtls_status = MBEDTLS_ERR_PKCS7_VERIFY_FAIL;
				goto end;
			}

			if (psa_hash_update(&op, signerCertCN->p, signerCertCN->len)) {
				psa_hash_abort(&op);
				mbedtls_status = MBEDTLS_ERR_PKCS7_VERIFY_FAIL;
				goto end;
			}

			if (psa_hash_update(&op, toplevelCertTbs->p, toplevelCertTbs->len)) {
				psa_hash_abort(&op);
				mbedtls_status = MBEDTLS_ERR_PKCS7_VERIFY_FAIL;
				goto end;
			}

			if (psa_hash_finish(&op, (uint8_t*)&output_buffer, PSA_HASH_MAX_SIZE, &output_size)) {
				psa_hash_abort(&op);
				mbedtls_status = MBEDTLS_ERR_PKCS7_VERIFY_FAIL;
				goto end;
			}

			/* Clear the remaining part of the buffer for consistency */
			memset(&output_buffer[output_size], 0, PSA_HASH_MAX_SIZE - output_size);

			rpc_status = serializer->serialize_get_uefi_priv_auth_var_fingerprint_resp(
				resp_buf, (uint8_t*)&output_buffer);
		}

end:
		mbedtls_pkcs7_free(&pkcs7_structure);
	}

	free(signature_cert);

	/* Provide the result of the verification */
	req->service_status = (mbedtls_status == MBEDTLS_PKCS7_SIGNED_DATA) ? EFI_SUCCESS : EFI_COMPROMISED_DATA;

	return rpc_status;
}
#else
static rpc_status_t verify_pkcs7_signature_handler(void *context, struct rpc_request *req)
{
	(void)context;
	(void)req;

	return RPC_ERROR_INTERNAL;
}

static rpc_status_t get_uefi_priv_auth_var_fingerprint_handler(void *context, struct rpc_request *req)
{
	(void)context;
	(void)req;

	return RPC_ERROR_INTERNAL;
}
#endif
