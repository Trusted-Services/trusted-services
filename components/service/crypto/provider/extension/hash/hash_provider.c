/*
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdint.h>
#include <stdlib.h>
#include <protocols/service/crypto/packed-c/opcodes.h>
#include <service/crypto/provider/extension/hash/hash_provider.h>
#include <protocols/rpc/common/packed-c/status.h>
#include <service/crypto/backend/crypto_backend.h>

/* Service request handlers */
static rpc_status_t hash_setup_handler(void *context, struct rpc_request *req);
static rpc_status_t hash_update_handler(void *context, struct rpc_request *req);
static rpc_status_t hash_finish_handler(void *context, struct rpc_request *req);
static rpc_status_t hash_abort_handler(void *context, struct rpc_request *req);
static rpc_status_t hash_verify_handler(void *context, struct rpc_request *req);
static rpc_status_t hash_clone_handler(void *context, struct rpc_request *req);
static void hash_abort_context(struct crypto_context *crypto_context);

/* Handler mapping table for service */
static const struct service_handler handler_table[] = {
	{TS_CRYPTO_OPCODE_HASH_SETUP,           hash_setup_handler},
	{TS_CRYPTO_OPCODE_HASH_UPDATE,          hash_update_handler},
	{TS_CRYPTO_OPCODE_HASH_FINISH,          hash_finish_handler},
	{TS_CRYPTO_OPCODE_HASH_ABORT,          	hash_abort_handler},
	{TS_CRYPTO_OPCODE_HASH_VERIFY,          hash_verify_handler},
	{TS_CRYPTO_OPCODE_HASH_CLONE,          	hash_clone_handler}
};

void hash_provider_init(struct hash_provider *context)
{
	const struct rpc_uuid nil_uuid = { 0 };

	crypto_context_pool_init(&context->context_pool);

	for (size_t encoding = 0; encoding < TS_RPC_ENCODING_LIMIT; ++encoding)
		context->serializers[encoding] = NULL;

	service_provider_init(&context->base_provider, context, &nil_uuid,
		handler_table, sizeof(handler_table)/sizeof(struct service_handler));
}

void hash_provider_deinit(struct hash_provider *context)
{
	crypto_context_pool_deinit(&context->context_pool);
}

void hash_provider_register_serializer(struct hash_provider *context,
	unsigned int encoding, const struct hash_provider_serializer *serializer)
{
	if (encoding < TS_RPC_ENCODING_LIMIT)
		context->serializers[encoding] = serializer;
}

static const struct hash_provider_serializer* get_serializer(void *context,
	const struct rpc_request *req)
{
	struct hash_provider *this_instance = (struct hash_provider*)context;
	unsigned int encoding = 0; /* No other encodings supported */

	return this_instance->serializers[encoding];
}

static void hash_abort_context(struct crypto_context *crypto_context)
{
	(void)psa_hash_abort(&crypto_context->op.hash);
}

static rpc_status_t hash_setup_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct hash_provider_serializer *serializer = get_serializer(context, req);
	struct hash_provider *this_instance = (struct hash_provider*)context;

	psa_algorithm_t alg = PSA_ALG_NONE;

	if (serializer)
		rpc_status = serializer->deserialize_hash_setup_req(req_buf, &alg);

	if (rpc_status == RPC_SUCCESS) {

		uint32_t op_handle = 0;

		struct crypto_context *crypto_context =
			crypto_context_pool_alloc(&this_instance->context_pool,
				CRYPTO_CONTEXT_OP_ID_HASH, req->source_id,
				hash_abort_context, &op_handle);

		if (crypto_context) {

			psa_status_t psa_status;

			crypto_context->op.hash = psa_hash_operation_init();
			psa_status = psa_hash_setup(&crypto_context->op.hash, alg);

			if (psa_status == PSA_SUCCESS) {

				struct rpc_buffer *resp_buf = &req->response;
				rpc_status = serializer->serialize_hash_setup_resp(resp_buf, op_handle);
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

static rpc_status_t hash_update_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct hash_provider_serializer *serializer = get_serializer(context, req);
	struct hash_provider *this_instance = (struct hash_provider*)context;

	uint32_t op_handle = 0;
	const uint8_t *data = NULL;
	size_t data_len = 0;

	if (serializer)
		rpc_status = serializer->deserialize_hash_update_req(req_buf, &op_handle, &data, &data_len);

	if (rpc_status == RPC_SUCCESS) {

		psa_status_t psa_status = PSA_ERROR_BAD_STATE;

		struct crypto_context *crypto_context =
			crypto_context_pool_find(&this_instance->context_pool,
				CRYPTO_CONTEXT_OP_ID_HASH, req->source_id,
				op_handle);

		if (crypto_context) {

			psa_status = psa_hash_update(&crypto_context->op.hash, data, data_len);
		}

		req->service_status = psa_status;
	}

	return rpc_status;
}

static rpc_status_t hash_finish_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct hash_provider_serializer *serializer = get_serializer(context, req);
	struct hash_provider *this_instance = (struct hash_provider*)context;

	uint32_t op_handle = 0;

	if (serializer)
		rpc_status = serializer->deserialize_hash_finish_req(req_buf, &op_handle);

	if (rpc_status == RPC_SUCCESS) {

		psa_status_t psa_status = PSA_ERROR_BAD_STATE;

		struct crypto_context *crypto_context =
			crypto_context_pool_find(&this_instance->context_pool,
				CRYPTO_CONTEXT_OP_ID_HASH, req->source_id,
				op_handle);

		if (crypto_context) {

			size_t hash_len;
			uint8_t hash[PSA_HASH_MAX_SIZE];

			psa_status = psa_hash_finish(&crypto_context->op.hash, hash, sizeof(hash), &hash_len);

			if (psa_status == PSA_SUCCESS) {

				struct rpc_buffer *resp_buf = &req->response;
				rpc_status = serializer->serialize_hash_finish_resp(resp_buf, hash, hash_len);

				crypto_context_pool_free(&this_instance->context_pool, crypto_context);
			}
		}

		req->service_status = psa_status;
	}

	return rpc_status;
}

static rpc_status_t hash_abort_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct hash_provider_serializer *serializer = get_serializer(context, req);
	struct hash_provider *this_instance = (struct hash_provider*)context;

	uint32_t op_handle = 0;

	if (serializer)
		rpc_status = serializer->deserialize_hash_abort_req(req_buf, &op_handle);

	if (rpc_status == RPC_SUCCESS) {

		/* Return success if operation is no longer active and
		 * doesn't need aborting.
		 */
		psa_status_t psa_status = PSA_SUCCESS;

		struct crypto_context *crypto_context =
			crypto_context_pool_find(&this_instance->context_pool,
				CRYPTO_CONTEXT_OP_ID_HASH, req->source_id,
				op_handle);

		if (crypto_context) {

			psa_status = psa_hash_abort(&crypto_context->op.hash);
			crypto_context_pool_free(&this_instance->context_pool, crypto_context);
		}

		req->service_status = psa_status;
	}

	return rpc_status;
}

static rpc_status_t hash_verify_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct hash_provider_serializer *serializer = get_serializer(context, req);
	struct hash_provider *this_instance = (struct hash_provider*)context;

	uint32_t op_handle = 0;
	const uint8_t *hash = NULL;
	size_t hash_len = 0;

	if (serializer)
		rpc_status = serializer->deserialize_hash_verify_req(req_buf, &op_handle, &hash, &hash_len);

	if (rpc_status == RPC_SUCCESS) {

		psa_status_t psa_status = PSA_ERROR_BAD_STATE;

		struct crypto_context *crypto_context =
			crypto_context_pool_find(&this_instance->context_pool,
				CRYPTO_CONTEXT_OP_ID_HASH, req->source_id,
				op_handle);

		if (crypto_context) {

			psa_status = psa_hash_verify(&crypto_context->op.hash, hash, hash_len);

			if (psa_status == PSA_SUCCESS)
				crypto_context_pool_free(&this_instance->context_pool, crypto_context);
		}

		req->service_status = psa_status;
	}

	return rpc_status;
}

static rpc_status_t hash_clone_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct hash_provider_serializer *serializer = get_serializer(context, req);
	struct hash_provider *this_instance = (struct hash_provider*)context;

	uint32_t source_op_handle = 0;

	if (serializer)
		rpc_status = serializer->deserialize_hash_clone_req(req_buf, &source_op_handle);

	if (rpc_status == RPC_SUCCESS) {

		psa_status_t psa_status = PSA_ERROR_BAD_STATE;

		struct crypto_context *source_crypto_context =
			crypto_context_pool_find(&this_instance->context_pool,
				CRYPTO_CONTEXT_OP_ID_HASH, req->source_id,
				source_op_handle);

		if (source_crypto_context) {

			uint32_t target_op_handle;

			struct crypto_context *target_crypto_context = crypto_context_pool_alloc(
				&this_instance->context_pool,
				CRYPTO_CONTEXT_OP_ID_HASH, req->source_id,
				hash_abort_context, &target_op_handle);

			if (target_crypto_context) {

				target_crypto_context->op.hash = psa_hash_operation_init();

				psa_status = psa_hash_clone(&source_crypto_context->op.hash,
					&target_crypto_context->op.hash);

				if (psa_status == PSA_SUCCESS) {

					struct rpc_buffer *resp_buf = &req->response;
					rpc_status = serializer->serialize_hash_clone_resp(resp_buf, target_op_handle);
				}
			}
		}

		req->service_status = psa_status;
	}

	return rpc_status;
}
