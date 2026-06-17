/*
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdint.h>
#include <stdlib.h>
#include <protocols/service/crypto/packed-c/opcodes.h>
#include <service/crypto/provider/extension/key_derivation/key_derivation_provider.h>
#include <protocols/rpc/common/packed-c/status.h>
#include <service/crypto/backend/crypto_backend.h>
#include <service/crypto/provider/crypto_partition.h>

/* Service request handlers */
static rpc_status_t key_derivation_setup_handler(void *context, struct rpc_request *req);
static rpc_status_t key_derivation_get_capacity_handler(void *context, struct rpc_request *req);
static rpc_status_t key_derivation_set_capacity_handler(void *context, struct rpc_request *req);
static rpc_status_t key_derivation_input_bytes_handler(void *context, struct rpc_request *req);
static rpc_status_t key_derivation_input_key_handler(void *context, struct rpc_request *req);
static rpc_status_t key_derivation_output_bytes_handler(void *context, struct rpc_request *req);
static rpc_status_t key_derivation_output_key_handler(void *context, struct rpc_request *req);
static rpc_status_t key_derivation_abort_handler(void *context, struct rpc_request *req);
static rpc_status_t key_derivation_key_agreement_handler(void *context, struct rpc_request *req);
static rpc_status_t key_derivation_raw_key_agreement_handler(void *context,
							     struct rpc_request *req);
static void key_derivation_abort_context(struct crypto_context *crypto_context);

/* Handler mapping table for service */
static const struct service_handler handler_table[] = {
	{TS_CRYPTO_OPCODE_KEY_DERIVATION_SETUP,				key_derivation_setup_handler},
	{TS_CRYPTO_OPCODE_KEY_DERIVATION_GET_CAPACITY,		key_derivation_get_capacity_handler},
	{TS_CRYPTO_OPCODE_KEY_DERIVATION_SET_CAPACITY,		key_derivation_set_capacity_handler},
	{TS_CRYPTO_OPCODE_KEY_DERIVATION_INPUT_BYTES,		key_derivation_input_bytes_handler},
	{TS_CRYPTO_OPCODE_KEY_DERIVATION_INPUT_KEY,			key_derivation_input_key_handler},
	{TS_CRYPTO_OPCODE_KEY_DERIVATION_OUTPUT_BYTES,		key_derivation_output_bytes_handler},
	{TS_CRYPTO_OPCODE_KEY_DERIVATION_OUTPUT_KEY,		key_derivation_output_key_handler},
	{TS_CRYPTO_OPCODE_KEY_DERIVATION_ABORT,				key_derivation_abort_handler},
	{TS_CRYPTO_OPCODE_KEY_DERIVATION_KEY_AGREEMENT,		key_derivation_key_agreement_handler},
	{TS_CRYPTO_OPCODE_KEY_DERIVATION_RAW_KEY_AGREEMENT,	key_derivation_raw_key_agreement_handler}
};

void key_derivation_provider_init(struct key_derivation_provider *context)
{
	const struct rpc_uuid nil_uuid = { 0 };

	crypto_context_pool_init(&context->context_pool);

	for (size_t encoding = 0; encoding < TS_RPC_ENCODING_LIMIT; ++encoding)
		context->serializers[encoding] = NULL;

	service_provider_init(&context->base_provider, context, &nil_uuid,
		handler_table, sizeof(handler_table)/sizeof(struct service_handler));
}

void key_derivation_provider_deinit(struct key_derivation_provider *context)
{
	crypto_context_pool_deinit(&context->context_pool);
}

void key_derivation_provider_register_serializer(struct key_derivation_provider *context,
	unsigned int encoding, const struct key_derivation_provider_serializer *serializer)
{
	if (encoding < TS_RPC_ENCODING_LIMIT)
		context->serializers[encoding] = serializer;
}

static const struct key_derivation_provider_serializer* get_serializer(void *context,
	const struct rpc_request *req)
{
	struct key_derivation_provider *this_instance = (struct key_derivation_provider*)context;
	unsigned int encoding = 0; /* No other encodings supported */

	return this_instance->serializers[encoding];
}

static void key_derivation_abort_context(struct crypto_context *crypto_context)
{
	(void)psa_key_derivation_abort(&crypto_context->op.key_derivation);
}

static rpc_status_t key_derivation_setup_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct key_derivation_provider_serializer *serializer = get_serializer(context, req);
	struct key_derivation_provider *this_instance = (struct key_derivation_provider*)context;

	psa_algorithm_t alg = PSA_ALG_NONE;

	if (serializer)
		rpc_status = serializer->deserialize_key_derivation_setup_req(req_buf, &alg);

	if (rpc_status == RPC_SUCCESS) {

		uint32_t op_handle = 0;

		struct crypto_context *crypto_context =
			crypto_context_pool_alloc(&this_instance->context_pool,
				CRYPTO_CONTEXT_OP_ID_KEY_DERIVATION, req->source_id,
				key_derivation_abort_context, &op_handle);

		if (crypto_context) {

			psa_status_t psa_status;

			crypto_context->op.key_derivation = psa_key_derivation_operation_init();
			psa_status = psa_key_derivation_setup(&crypto_context->op.key_derivation, alg);

			if (psa_status == PSA_SUCCESS) {

				struct rpc_buffer *resp_buf = &req->response;
				rpc_status = serializer->serialize_key_derivation_setup_resp(resp_buf, op_handle);
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

static rpc_status_t key_derivation_get_capacity_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct key_derivation_provider_serializer *serializer = get_serializer(context, req);
	struct key_derivation_provider *this_instance = (struct key_derivation_provider*)context;

	uint32_t op_handle = 0;

	if (serializer)
		rpc_status = serializer->deserialize_key_derivation_get_capacity_req(req_buf, &op_handle);

	if (rpc_status == RPC_SUCCESS) {

		psa_status_t psa_status = PSA_ERROR_BAD_STATE;

		struct crypto_context *crypto_context =
			crypto_context_pool_find(&this_instance->context_pool,
				CRYPTO_CONTEXT_OP_ID_KEY_DERIVATION, req->source_id,
				op_handle);

		if (crypto_context) {

			size_t capacity;

			psa_status = psa_key_derivation_get_capacity(&crypto_context->op.key_derivation,
				&capacity);

			if (psa_status == PSA_SUCCESS) {

				struct rpc_buffer *resp_buf = &req->response;
				rpc_status = serializer->serialize_key_derivation_get_capacity_resp(resp_buf,
					capacity);
			}
		}

		req->service_status = psa_status;
	}

	return rpc_status;
}

static rpc_status_t key_derivation_set_capacity_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct key_derivation_provider_serializer *serializer = get_serializer(context, req);
	struct key_derivation_provider *this_instance = (struct key_derivation_provider*)context;

	uint32_t op_handle = 0;
	size_t capacity = 0;

	if (serializer)
		rpc_status = serializer->deserialize_key_derivation_set_capacity_req(req_buf,
			&op_handle, &capacity);

	if (rpc_status == RPC_SUCCESS) {

		psa_status_t psa_status = PSA_ERROR_BAD_STATE;

		struct crypto_context *crypto_context =
			crypto_context_pool_find(&this_instance->context_pool,
				CRYPTO_CONTEXT_OP_ID_KEY_DERIVATION, req->source_id,
				op_handle);

		if (crypto_context) {

			psa_status = psa_key_derivation_set_capacity(&crypto_context->op.key_derivation,
				capacity);
		}

		req->service_status = psa_status;
	}

	return rpc_status;
}

static rpc_status_t key_derivation_input_bytes_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct key_derivation_provider_serializer *serializer = get_serializer(context, req);
	struct key_derivation_provider *this_instance = (struct key_derivation_provider*)context;

	uint32_t op_handle = 0;
	psa_key_derivation_step_t step = (psa_key_derivation_step_t)0;
	const uint8_t *data = NULL;
	size_t data_len = 0;

	if (serializer)
		rpc_status = serializer->deserialize_key_derivation_input_bytes_req(req_buf,
			&op_handle, &step, &data, &data_len);

	if (rpc_status == RPC_SUCCESS) {

		psa_status_t psa_status = PSA_ERROR_BAD_STATE;

		struct crypto_context *crypto_context =
			crypto_context_pool_find(&this_instance->context_pool,
				CRYPTO_CONTEXT_OP_ID_KEY_DERIVATION, req->source_id,
				op_handle);

		if (crypto_context) {

			psa_status = psa_key_derivation_input_bytes(&crypto_context->op.key_derivation,
				step, data, data_len);
		}

		req->service_status = psa_status;
	}

	return rpc_status;
}

static rpc_status_t key_derivation_input_key_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct key_derivation_provider_serializer *serializer = get_serializer(context, req);
	struct key_derivation_provider *this_instance = (struct key_derivation_provider*)context;

	uint32_t op_handle = 0;
	psa_key_derivation_step_t step = (psa_key_derivation_step_t)0;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;

	if (serializer)
		rpc_status = serializer->deserialize_key_derivation_input_key_req(req_buf,
			&op_handle, &step, &key_id);

	if (rpc_status == RPC_SUCCESS) {

		psa_status_t psa_status = PSA_ERROR_INVALID_HANDLE;

		if (key_id) {

			psa_status = PSA_ERROR_BAD_STATE;

			struct crypto_context *crypto_context =
				crypto_context_pool_find(&this_instance->context_pool,
					CRYPTO_CONTEXT_OP_ID_KEY_DERIVATION, req->source_id,
					op_handle);

			if (crypto_context) {
				namespaced_key_id_t ns_key_id =
					crypto_partition_get_namespaced_key_id(req->source_id,
									       key_id);

				psa_status = psa_key_derivation_input_key(&crypto_context->op.key_derivation,
					step, ns_key_id);
			}
		}

		req->service_status = psa_status;
	}

	return rpc_status;
}

static rpc_status_t key_derivation_output_bytes_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct key_derivation_provider_serializer *serializer = get_serializer(context, req);
	struct key_derivation_provider *this_instance = (struct key_derivation_provider*)context;

	uint32_t op_handle = 0;
	size_t output_len = 0;

	if (serializer)
		rpc_status = serializer->deserialize_key_derivation_output_bytes_req(req_buf,
			&op_handle, &output_len);

	if (rpc_status == RPC_SUCCESS) {

		psa_status_t psa_status = PSA_ERROR_BAD_STATE;

		struct crypto_context *crypto_context =
			crypto_context_pool_find(&this_instance->context_pool,
				CRYPTO_CONTEXT_OP_ID_KEY_DERIVATION, req->source_id,
				op_handle);

		if (crypto_context) {

			uint8_t *output = malloc(output_len);

			if (output) {

				psa_status = psa_key_derivation_output_bytes(&crypto_context->op.key_derivation,
					output, output_len);

				if (psa_status == PSA_SUCCESS) {

					struct rpc_buffer *resp_buf = &req->response;
					rpc_status = serializer->serialize_key_derivation_output_bytes_resp(resp_buf,
						output, output_len);
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

static rpc_status_t key_derivation_output_key_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct key_derivation_provider_serializer *serializer = get_serializer(context, req);
	struct key_derivation_provider *this_instance = (struct key_derivation_provider*)context;

	uint32_t op_handle = 0;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

	if (serializer)
		rpc_status = serializer->deserialize_key_derivation_output_key_req(req_buf,
			&op_handle, &attributes);

	if (rpc_status == RPC_SUCCESS) {

		psa_status_t psa_status = PSA_ERROR_BAD_STATE;

		struct crypto_context *crypto_context =
			crypto_context_pool_find(&this_instance->context_pool,
				CRYPTO_CONTEXT_OP_ID_KEY_DERIVATION, req->source_id,
				op_handle);

		if (crypto_context) {

			namespaced_key_id_t ns_key_id = NAMESPACED_KEY_ID_INIT;

			crypto_partition_bind_to_owner(&attributes, req->source_id);

			psa_status = psa_key_derivation_output_key(&attributes,
				&crypto_context->op.key_derivation,
				&ns_key_id);

			if (psa_status == PSA_SUCCESS) {
				psa_key_id_t key_id = namespaced_key_id_get_key_id(ns_key_id);
				struct rpc_buffer *resp_buf = &req->response;
				rpc_status = serializer->serialize_key_derivation_output_key_resp(resp_buf,
					key_id);
			}
		}

		req->service_status = psa_status;
	}

	psa_reset_key_attributes(&attributes);

	return rpc_status;
}

static rpc_status_t key_derivation_abort_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct key_derivation_provider_serializer *serializer = get_serializer(context, req);
	struct key_derivation_provider *this_instance = (struct key_derivation_provider*)context;

	uint32_t op_handle = 0;

	if (serializer)
		rpc_status = serializer->deserialize_key_derivation_abort_req(req_buf, &op_handle);

	if (rpc_status == RPC_SUCCESS) {

		/* Return success if operation is no longer active and
		 * doesn't need aborting.
		 */
		psa_status_t psa_status = PSA_SUCCESS;

		struct crypto_context *crypto_context =
			crypto_context_pool_find(&this_instance->context_pool,
				CRYPTO_CONTEXT_OP_ID_KEY_DERIVATION, req->source_id,
				op_handle);

		if (crypto_context) {

			psa_status = psa_key_derivation_abort(&crypto_context->op.key_derivation);
			crypto_context_pool_free(&this_instance->context_pool, crypto_context);
		}

		req->service_status = psa_status;
	}

	return rpc_status;
}

static rpc_status_t key_derivation_key_agreement_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct key_derivation_provider_serializer *serializer = get_serializer(context, req);
	struct key_derivation_provider *this_instance = (struct key_derivation_provider*)context;

	uint32_t op_handle = 0;
	psa_key_derivation_step_t step = (psa_key_derivation_step_t)0;
	psa_key_id_t private_key_id = PSA_KEY_ID_NULL;
	const uint8_t *peer_key = NULL;
	size_t peer_key_len = 0;

	if (serializer)
		rpc_status = serializer->deserialize_key_derivation_key_agreement_req(req_buf,
			&op_handle, &step, &private_key_id, &peer_key, &peer_key_len);

	if (rpc_status == RPC_SUCCESS) {

		psa_status_t psa_status = PSA_ERROR_BAD_STATE;

		struct crypto_context *crypto_context =
			crypto_context_pool_find(&this_instance->context_pool,
				CRYPTO_CONTEXT_OP_ID_KEY_DERIVATION, req->source_id,
				op_handle);

		if (crypto_context) {
			namespaced_key_id_t private_ns_key_id =
				crypto_partition_get_namespaced_key_id(req->source_id,
								       private_key_id);

			psa_status = psa_key_derivation_key_agreement(&crypto_context->op.key_derivation,
				step, private_ns_key_id, peer_key, peer_key_len);
		}

		req->service_status = psa_status;
	}

	return rpc_status;
}

static rpc_status_t key_derivation_raw_key_agreement_handler(void *context, struct rpc_request *req)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_buffer *req_buf = &req->request;
	const struct key_derivation_provider_serializer *serializer = get_serializer(context, req);

	psa_algorithm_t alg = PSA_ALG_NONE;
	psa_key_id_t private_key_id = PSA_KEY_ID_NULL;
	const uint8_t *peer_key = NULL;
	size_t peer_key_len = 0;

	if (serializer)
		rpc_status = serializer->deserialize_key_derivation_raw_key_agreement_req(req_buf,
			&alg, &private_key_id, &peer_key, &peer_key_len);

	if (rpc_status == RPC_SUCCESS) {

		size_t output_len;
		uint8_t output[PSA_RAW_KEY_AGREEMENT_OUTPUT_MAX_SIZE];

		namespaced_key_id_t private_ns_key_id =
			crypto_partition_get_namespaced_key_id(req->source_id, private_key_id);

		psa_status_t psa_status = psa_raw_key_agreement(
			alg, private_ns_key_id, peer_key, peer_key_len,
			output, sizeof(output), &output_len);

		if (psa_status == PSA_SUCCESS) {

			struct rpc_buffer *resp_buf = &req->response;
			rpc_status = serializer->serialize_key_derivation_raw_key_agreement_resp(resp_buf,
				output, output_len);
		}

		req->service_status = psa_status;
	}

	return rpc_status;
}
