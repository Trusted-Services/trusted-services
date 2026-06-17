/*
 * Copyright (c) 2021-2026, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdint.h>
#include <stdlib.h>
#include <protocols/service/crypto/packed-c/opcodes.h>
#include <service/crypto/provider/extension/cipher/cipher_provider.h>
#include <protocols/rpc/common/packed-c/status.h>
#include <service/crypto/backend/crypto_backend.h>
#include <service/crypto/provider/crypto_partition.h>

/* Service request handlers */
static rpc_status_t cipher_setup_handler(void *context, struct rpc_request *req);
static rpc_status_t cipher_generate_iv_handler(void *context, struct rpc_request *req);
static rpc_status_t cipher_set_iv_handler(void *context, struct rpc_request *req);
static rpc_status_t cipher_update_handler(void *context, struct rpc_request *req);
static rpc_status_t cipher_finish_handler(void *context, struct rpc_request *req);
static rpc_status_t cipher_abort_handler(void *context, struct rpc_request *req);
static void cipher_abort_context(struct crypto_context *crypto_context);

/* Handler mapping table for service */
static const struct service_handler handler_table[] = {
	{TS_CRYPTO_OPCODE_CIPHER_ENCRYPT_SETUP,   	cipher_setup_handler},
	{TS_CRYPTO_OPCODE_CIPHER_DECRYPT_SETUP,   	cipher_setup_handler},
	{TS_CRYPTO_OPCODE_CIPHER_GENERATE_IV,   	cipher_generate_iv_handler},
	{TS_CRYPTO_OPCODE_CIPHER_SET_IV,   			cipher_set_iv_handler},
	{TS_CRYPTO_OPCODE_CIPHER_UPDATE,          	cipher_update_handler},
	{TS_CRYPTO_OPCODE_CIPHER_FINISH,          	cipher_finish_handler},
	{TS_CRYPTO_OPCODE_CIPHER_ABORT,          	cipher_abort_handler}
};

void cipher_provider_init(struct cipher_provider *context)
{
	const struct rpc_uuid nil_uuid = { 0 };

	crypto_context_pool_init(&context->context_pool);

	for (size_t encoding = 0; encoding < TS_RPC_ENCODING_LIMIT; ++encoding)
		context->serializers[encoding] = NULL;

	service_provider_init(&context->base_provider, context, &nil_uuid,
		handler_table, sizeof(handler_table)/sizeof(struct service_handler));
}

void cipher_provider_deinit(struct cipher_provider *context)
{
	crypto_context_pool_deinit(&context->context_pool);
}

void cipher_provider_register_serializer(struct cipher_provider *context,
	unsigned int encoding, const struct cipher_provider_serializer *serializer)
{
	if (encoding < TS_RPC_ENCODING_LIMIT)
		context->serializers[encoding] = serializer;
}

static const struct cipher_provider_serializer* get_serializer(void *context,
	const struct rpc_request *req)
{
	struct cipher_provider *this_instance = (struct cipher_provider*)context;
	unsigned int encoding = 0; /* No other encodings supported */

	return this_instance->serializers[encoding];
}

static void cipher_abort_context(struct crypto_context *crypto_context)
{
	(void)psa_cipher_abort(&crypto_context->op.cipher);
}

static rpc_status_t cipher_setup_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct cipher_provider_serializer *serializer = get_serializer(context, req);
	struct cipher_provider *this_instance = (struct cipher_provider*)context;

	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_algorithm_t alg = PSA_ALG_NONE;

	if (serializer)
		rpc_status = serializer->deserialize_cipher_setup_req(req_buf, &key_id, &alg);

	if (rpc_status == RPC_SUCCESS) {

		uint32_t op_handle = 0;

		struct crypto_context *crypto_context =
			crypto_context_pool_alloc(&this_instance->context_pool,
				CRYPTO_CONTEXT_OP_ID_CIPHER, req->source_id,
				cipher_abort_context, &op_handle);

		if (crypto_context) {

			psa_status_t psa_status;
			namespaced_key_id_t ns_key_id =
				crypto_partition_get_namespaced_key_id(req->source_id, key_id);

			crypto_context->op.cipher = psa_cipher_operation_init();

			psa_status = (req->opcode == TS_CRYPTO_OPCODE_CIPHER_ENCRYPT_SETUP) ?
				psa_cipher_encrypt_setup(&crypto_context->op.cipher, ns_key_id, alg) :
				psa_cipher_decrypt_setup(&crypto_context->op.cipher, ns_key_id, alg);

			if (psa_status == PSA_SUCCESS) {

				struct rpc_buffer *resp_buf = &req->response;
				rpc_status = serializer->serialize_cipher_setup_resp(resp_buf, op_handle);
			}

			if ((psa_status != PSA_SUCCESS) || (rpc_status != RPC_SUCCESS))
				crypto_context_pool_free(&this_instance->context_pool, crypto_context);

			req->service_status = psa_status;
		}
		else {
			/* Failed to allocate crypto context for transaction */
			rpc_status = RPC_ERROR_RESOURCE_FAILURE;
		}
	}

	return rpc_status;
}

static rpc_status_t cipher_generate_iv_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct cipher_provider_serializer *serializer = get_serializer(context, req);
	struct cipher_provider *this_instance = (struct cipher_provider*)context;

	uint32_t op_handle = 0;

	if (serializer)
		rpc_status = serializer->deserialize_cipher_generate_iv_req(req_buf, &op_handle);

	if (rpc_status == RPC_SUCCESS) {

		psa_status_t psa_status = PSA_ERROR_BAD_STATE;

		struct crypto_context *crypto_context =
			crypto_context_pool_find(&this_instance->context_pool,
				CRYPTO_CONTEXT_OP_ID_CIPHER, req->source_id,
				op_handle);

		if (crypto_context) {

			size_t iv_len;
			uint8_t iv[PSA_CIPHER_IV_MAX_SIZE];

			psa_status = psa_cipher_generate_iv(&crypto_context->op.cipher, iv, sizeof(iv), &iv_len);

			if (psa_status == PSA_SUCCESS) {

				struct rpc_buffer *resp_buf = &req->response;
				rpc_status = serializer->serialize_cipher_generate_iv_resp(resp_buf, iv, iv_len);
			} else {
				(void)psa_cipher_abort(&crypto_context->op.cipher);
				crypto_context_pool_free(&this_instance->context_pool,
					crypto_context);
			}
		}

		req->service_status = psa_status;
	}

	return rpc_status;
}

static rpc_status_t cipher_set_iv_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct cipher_provider_serializer *serializer = get_serializer(context, req);
	struct cipher_provider *this_instance = (struct cipher_provider*)context;

	uint32_t op_handle = 0;
	const uint8_t *iv = NULL;
	size_t iv_len = 0;

	if (serializer)
		rpc_status = serializer->deserialize_cipher_set_iv_req(req_buf, &op_handle,
			&iv, &iv_len);

	if (rpc_status == RPC_SUCCESS) {

		psa_status_t psa_status = PSA_ERROR_BAD_STATE;

		struct crypto_context *crypto_context =
			crypto_context_pool_find(&this_instance->context_pool,
				CRYPTO_CONTEXT_OP_ID_CIPHER, req->source_id,
				op_handle);

		if (crypto_context) {

			psa_status = psa_cipher_set_iv(&crypto_context->op.cipher, iv, iv_len);

			if (psa_status != PSA_SUCCESS) {
				(void)psa_cipher_abort(&crypto_context->op.cipher);
				crypto_context_pool_free(&this_instance->context_pool,
					crypto_context);
			}
		}

		req->service_status = psa_status;
	}

	return rpc_status;
}

static rpc_status_t cipher_update_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct cipher_provider_serializer *serializer = get_serializer(context, req);
	struct cipher_provider *this_instance = (struct cipher_provider*)context;

	uint32_t op_handle = 0;
	const uint8_t *input = NULL;
	size_t input_len = 0;

	if (serializer)
		rpc_status = serializer->deserialize_cipher_update_req(req_buf, &op_handle,
			&input, &input_len);

	if (rpc_status == RPC_SUCCESS) {

		psa_status_t psa_status = PSA_ERROR_BAD_STATE;

		struct crypto_context *crypto_context =
			crypto_context_pool_find(&this_instance->context_pool,
				CRYPTO_CONTEXT_OP_ID_CIPHER, req->source_id,
				op_handle);

		if (crypto_context) {

			size_t output_len = 0;
			size_t output_size = PSA_CIPHER_UPDATE_OUTPUT_MAX_SIZE(input_len);
			uint8_t *output = malloc(output_size);

			if (output) {

				psa_status = psa_cipher_update(&crypto_context->op.cipher,
					input, input_len,
					output, output_size, &output_len);

				if (psa_status == PSA_SUCCESS) {

					struct rpc_buffer *resp_buf = &req->response;
					rpc_status = serializer->serialize_cipher_update_resp(resp_buf,
						output, output_len);
				} else {
					(void)psa_cipher_abort(&crypto_context->op.cipher);
					crypto_context_pool_free(&this_instance->context_pool,
						crypto_context);
				}

				free(output);
			}
			else {

				psa_status = PSA_ERROR_INSUFFICIENT_MEMORY;
			}
		}

		req->service_status = psa_status;
	}

	return rpc_status;
}

static rpc_status_t cipher_finish_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct cipher_provider_serializer *serializer = get_serializer(context, req);
	struct cipher_provider *this_instance = (struct cipher_provider*)context;

	uint32_t op_handle = 0;

	if (serializer)
		rpc_status = serializer->deserialize_cipher_finish_req(req_buf, &op_handle);

	if (rpc_status == RPC_SUCCESS) {

		psa_status_t psa_status = PSA_ERROR_BAD_STATE;

		struct crypto_context *crypto_context =
			crypto_context_pool_find(&this_instance->context_pool,
				CRYPTO_CONTEXT_OP_ID_CIPHER, req->source_id,
				op_handle);

		if (crypto_context) {

			size_t output_len;
			uint8_t output[PSA_CIPHER_FINISH_OUTPUT_MAX_SIZE];

			psa_status = psa_cipher_finish(&crypto_context->op.cipher, output, sizeof(output), &output_len);

			if (psa_status == PSA_SUCCESS) {

				struct rpc_buffer *resp_buf = &req->response;
				rpc_status = serializer->serialize_cipher_finish_resp(resp_buf, output, output_len);
			} else {
				(void)psa_cipher_abort(&crypto_context->op.cipher);
			}
			crypto_context_pool_free(&this_instance->context_pool,
					crypto_context);
		}

		req->service_status = psa_status;
	}

	return rpc_status;
}

static rpc_status_t cipher_abort_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct cipher_provider_serializer *serializer = get_serializer(context, req);
	struct cipher_provider *this_instance = (struct cipher_provider*)context;

	uint32_t op_handle = 0;

	if (serializer)
		rpc_status = serializer->deserialize_cipher_abort_req(req_buf, &op_handle);

	if (rpc_status == RPC_SUCCESS) {

		/* Return success if operation is no longer active and
		 * doesn't need aborting.
		 */
		psa_status_t psa_status = PSA_SUCCESS;

		struct crypto_context *crypto_context =
			crypto_context_pool_find(&this_instance->context_pool,
				CRYPTO_CONTEXT_OP_ID_CIPHER, req->source_id,
				op_handle);

		if (crypto_context) {

			psa_status = psa_cipher_abort(&crypto_context->op.cipher);
			crypto_context_pool_free(&this_instance->context_pool, crypto_context);
		}

		req->service_status = psa_status;
	}

	return rpc_status;
}
