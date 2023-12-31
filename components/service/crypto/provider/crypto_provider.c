/*
 * Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <protocols/rpc/common/packed-c/status.h>
#include <protocols/service/crypto/packed-c/opcodes.h>
#include <psa/crypto.h>
#include <service/crypto/provider/crypto_provider.h>
#include <stdint.h>
#include <stdlib.h>

#include "crypto_uuid.h"

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

/* Handler mapping table for service */
static const struct service_handler handler_table[] = {
	{ TS_CRYPTO_OPCODE_GENERATE_KEY, generate_key_handler },
	{ TS_CRYPTO_OPCODE_DESTROY_KEY, destroy_key_handler },
	{ TS_CRYPTO_OPCODE_EXPORT_KEY, export_key_handler },
	{ TS_CRYPTO_OPCODE_EXPORT_PUBLIC_KEY, export_public_key_handler },
	{ TS_CRYPTO_OPCODE_IMPORT_KEY, import_key_handler },
	{ TS_CRYPTO_OPCODE_SIGN_HASH, asymmetric_sign_handler },
	{ TS_CRYPTO_OPCODE_VERIFY_HASH, asymmetric_verify_handler },
	{ TS_CRYPTO_OPCODE_ASYMMETRIC_DECRYPT, asymmetric_decrypt_handler },
	{ TS_CRYPTO_OPCODE_ASYMMETRIC_ENCRYPT, asymmetric_encrypt_handler },
	{ TS_CRYPTO_OPCODE_GENERATE_RANDOM, generate_random_handler },
	{ TS_CRYPTO_OPCODE_COPY_KEY, copy_key_handler },
	{ TS_CRYPTO_OPCODE_PURGE_KEY, purge_key_handler },
	{ TS_CRYPTO_OPCODE_GET_KEY_ATTRIBUTES, get_key_attributes_handler },
	{ TS_CRYPTO_OPCODE_SIGN_MESSAGE, asymmetric_sign_handler },
	{ TS_CRYPTO_OPCODE_VERIFY_MESSAGE, asymmetric_verify_handler },
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
		psa_key_id_t id;

		psa_status = psa_generate_key(&attributes, &id);

		if (psa_status == PSA_SUCCESS) {
			struct rpc_buffer *resp_buf = &req->response;
			rpc_status = serializer->serialize_generate_key_resp(resp_buf, id);
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

	psa_key_id_t id;

	if (serializer)
		rpc_status = serializer->deserialize_destroy_key_req(req_buf, &id);

	if (rpc_status == RPC_SUCCESS) {
		psa_status_t psa_status;

		psa_status = psa_destroy_key(id);
		req->service_status = psa_status;
	}

	return rpc_status;
}

static rpc_status_t export_key_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct crypto_provider_serializer *serializer = get_crypto_serializer(context, req);

	psa_key_id_t id;

	if (serializer)
		rpc_status = serializer->deserialize_export_key_req(req_buf, &id);

	if (rpc_status == RPC_SUCCESS) {
		size_t max_export_size = PSA_EXPORT_KEY_PAIR_MAX_SIZE;
		uint8_t *key_buffer = malloc(max_export_size);

		if (key_buffer) {
			size_t export_size;
			psa_status_t psa_status =
				psa_export_key(id, key_buffer, max_export_size, &export_size);

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

	psa_key_id_t id;

	if (serializer)
		rpc_status = serializer->deserialize_export_public_key_req(req_buf, &id);

	if (rpc_status == RPC_SUCCESS) {
		size_t max_export_size = PSA_EXPORT_PUBLIC_KEY_MAX_SIZE;
		uint8_t *key_buffer = malloc(max_export_size);

		if (key_buffer) {
			size_t export_size;
			psa_status_t psa_status = psa_export_public_key(
				id, key_buffer, max_export_size, &export_size);

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
				psa_key_id_t id;

				psa_status =
					psa_import_key(&attributes, key_buffer, key_data_len, &id);

				if (psa_status == PSA_SUCCESS) {
					struct rpc_buffer *resp_buf = &req->response;

					rpc_status =
						serializer->serialize_import_key_resp(resp_buf, id);
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

	psa_key_id_t id;
	psa_algorithm_t alg;
	size_t hash_len = PSA_HASH_MAX_SIZE;
	uint8_t hash_buffer[PSA_HASH_MAX_SIZE];

	if (serializer)
		rpc_status = serializer->deserialize_asymmetric_sign_req(req_buf, &id, &alg,
									 hash_buffer, &hash_len);

	if (rpc_status == RPC_SUCCESS) {
		psa_status_t psa_status;
		size_t sig_len;
		uint8_t sig_buffer[PSA_SIGNATURE_MAX_SIZE];

		psa_status = (req->opcode == TS_CRYPTO_OPCODE_SIGN_HASH) ?
				     psa_sign_hash(id, alg, hash_buffer, hash_len, sig_buffer,
						   sizeof(sig_buffer), &sig_len) :
				     psa_sign_message(id, alg, hash_buffer, hash_len, sig_buffer,
						      sizeof(sig_buffer), &sig_len);

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

	psa_key_id_t id;
	psa_algorithm_t alg;
	size_t hash_len = PSA_HASH_MAX_SIZE;
	uint8_t hash_buffer[PSA_HASH_MAX_SIZE];
	size_t sig_len = PSA_SIGNATURE_MAX_SIZE;
	uint8_t sig_buffer[PSA_SIGNATURE_MAX_SIZE];

	if (serializer)
		rpc_status = serializer->deserialize_asymmetric_verify_req(
			req_buf, &id, &alg, hash_buffer, &hash_len, sig_buffer, &sig_len);

	if (rpc_status == RPC_SUCCESS) {
		psa_status_t psa_status;

		psa_status = (req->opcode == TS_CRYPTO_OPCODE_VERIFY_HASH) ?
				     psa_verify_hash(id, alg, hash_buffer, hash_len, sig_buffer,
						     sig_len) :
				     psa_verify_message(id, alg, hash_buffer, hash_len, sig_buffer,
							sig_len);

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

		psa_key_id_t id;
		psa_algorithm_t alg;
		size_t ciphertext_len = max_param_size;
		uint8_t *ciphertext_buffer = malloc(ciphertext_len);
		size_t salt_len = max_param_size;
		uint8_t *salt_buffer = malloc(salt_len);

		if (ciphertext_buffer && salt_buffer) {
			rpc_status = serializer->deserialize_asymmetric_decrypt_req(
				req_buf, &id, &alg, ciphertext_buffer, &ciphertext_len, salt_buffer,
				&salt_len);

			if (rpc_status == RPC_SUCCESS) {
				psa_status_t psa_status;
				psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

				psa_status = psa_get_key_attributes(id, &attributes);

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
							id, alg, ciphertext_buffer, ciphertext_len,
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

		psa_key_id_t id;
		psa_algorithm_t alg;
		size_t plaintext_len = max_param_size;
		uint8_t *plaintext_buffer = malloc(plaintext_len);
		size_t salt_len = max_param_size;
		uint8_t *salt_buffer = malloc(salt_len);

		if (plaintext_buffer && salt_buffer) {
			rpc_status = serializer->deserialize_asymmetric_encrypt_req(
				req_buf, &id, &alg, plaintext_buffer, &plaintext_len, salt_buffer,
				&salt_len);

			if (rpc_status == RPC_SUCCESS) {
				psa_status_t psa_status;
				psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

				psa_status = psa_get_key_attributes(id, &attributes);

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
							id, alg, plaintext_buffer, plaintext_len,
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
	psa_key_id_t source_key_id;

	if (serializer)
		rpc_status =
			serializer->deserialize_copy_key_req(req_buf, &attributes, &source_key_id);

	if (rpc_status == RPC_SUCCESS) {
		psa_key_id_t target_key_id;

		psa_status_t psa_status = psa_copy_key(source_key_id, &attributes, &target_key_id);

		if (psa_status == PSA_SUCCESS) {
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

	psa_key_id_t id;

	if (serializer)
		rpc_status = serializer->deserialize_purge_key_req(req_buf, &id);

	if (rpc_status == RPC_SUCCESS) {
		psa_status_t psa_status = psa_purge_key(id);
		req->service_status = psa_status;
	}

	return rpc_status;
}

static rpc_status_t get_key_attributes_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct crypto_provider_serializer *serializer = get_crypto_serializer(context, req);

	psa_key_id_t id;

	if (serializer)
		rpc_status = serializer->deserialize_get_key_attributes_req(req_buf, &id);

	if (rpc_status == RPC_SUCCESS) {
		psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

		psa_status_t psa_status = psa_get_key_attributes(id, &attributes);

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
