/*
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PACKEDC_CRYPTO_CALLER_HASH_H
#define PACKEDC_CRYPTO_CALLER_HASH_H

#include <string.h>
#include <stdlib.h>
#include <psa/crypto.h>
#include <service/common/client/service_client.h>
#include <protocols/rpc/common/packed-c/status.h>
#include <protocols/service/crypto/packed-c/opcodes.h>
#include <protocols/service/crypto/packed-c/hash.h>
#include <common/tlv/tlv.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline psa_status_t crypto_caller_hash_setup(struct service_client *context,
	uint32_t *op_handle,
	psa_algorithm_t alg)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	struct ts_crypto_hash_setup_in req_msg;
	size_t req_len = sizeof(struct ts_crypto_hash_setup_in);

	req_msg.alg = alg;

	rpc_call_handle call_handle;
	uint8_t *req_buf;

	call_handle = rpc_caller_session_begin(context->session, &req_buf, req_len, 0);

	if (call_handle) {

		uint8_t *resp_buf;
		size_t resp_len;
		service_status_t service_status;

		memcpy(req_buf, &req_msg, req_len);

		context->rpc_status =
			rpc_caller_session_invoke(call_handle, TS_CRYPTO_OPCODE_HASH_SETUP,
						  &resp_buf, &resp_len, &service_status);

		if (context->rpc_status == RPC_SUCCESS) {

			psa_status = service_status;

			if (psa_status == PSA_SUCCESS) {

				if (resp_len >= sizeof(struct ts_crypto_hash_setup_out)) {

					struct ts_crypto_hash_setup_out resp_msg;
					memcpy(&resp_msg, resp_buf, sizeof(struct ts_crypto_hash_setup_out));
					*op_handle = resp_msg.op_handle;
				}
				else {
					/* Failed to decode response message */
					psa_status = PSA_ERROR_GENERIC_ERROR;
				}
			}
		}

		rpc_caller_session_end(call_handle);
	}

	return psa_status;
}

static inline psa_status_t crypto_caller_hash_update(struct service_client *context,
	uint32_t op_handle,
	const uint8_t *input,
	size_t input_length)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	struct ts_crypto_hash_update_in req_msg;
	const size_t req_fixed_len = sizeof(struct ts_crypto_hash_update_in);
	size_t req_len = req_fixed_len;

	req_msg.op_handle = op_handle;

	/* Mandatory input data parameter */
	struct tlv_record data_record;
	data_record.tag = TS_CRYPTO_HASH_UPDATE_IN_TAG_DATA;
	data_record.length = input_length;
	data_record.value = input;
	req_len += tlv_required_space(data_record.length);

	rpc_call_handle call_handle;
	uint8_t *req_buf;

	call_handle = rpc_caller_session_begin(context->session, &req_buf, req_len, 0);

	if (call_handle) {

		uint8_t *resp_buf;
		size_t resp_len;
		service_status_t service_status;
		struct tlv_iterator req_iter;

		memcpy(req_buf, &req_msg, req_fixed_len);

		tlv_iterator_begin(&req_iter, &req_buf[req_fixed_len], req_len - req_fixed_len);
		tlv_encode(&req_iter, &data_record);

		context->rpc_status =
			rpc_caller_session_invoke(call_handle, TS_CRYPTO_OPCODE_HASH_UPDATE,
						  &resp_buf, &resp_len, &service_status);

		if (context->rpc_status == RPC_SUCCESS)
			psa_status = service_status;

		rpc_caller_session_end(call_handle);
	}

	return psa_status;
}

static inline psa_status_t crypto_caller_hash_finish(struct service_client *context,
	uint32_t op_handle,
	uint8_t *hash,
	size_t hash_size,
	size_t *hash_length)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	struct ts_crypto_hash_finish_in req_msg;
	const size_t req_fixed_len = sizeof(struct ts_crypto_hash_finish_in);
	size_t req_len = req_fixed_len;

	*hash_length = 0;
	req_msg.op_handle = op_handle;

	rpc_call_handle call_handle;
	uint8_t *req_buf;

	call_handle = rpc_caller_session_begin(context->session, &req_buf, req_len,
					       tlv_required_space(hash_size));

	if (call_handle) {

		uint8_t *resp_buf;
		size_t resp_len;
		service_status_t service_status;

		memcpy(req_buf, &req_msg, req_fixed_len);

		context->rpc_status =
			rpc_caller_session_invoke(call_handle, TS_CRYPTO_OPCODE_HASH_FINISH,
						  &resp_buf, &resp_len, &service_status);

		if (context->rpc_status == RPC_SUCCESS) {

			psa_status = service_status;

			if (psa_status == PSA_SUCCESS) {

				struct tlv_const_iterator resp_iter;
				struct tlv_record decoded_record;
				tlv_const_iterator_begin(&resp_iter, resp_buf, resp_len);

				if (tlv_find_decode(&resp_iter,
					TS_CRYPTO_HASH_FINISH_OUT_TAG_HASH, &decoded_record)) {

					if (decoded_record.length <= hash_size) {

						memcpy(hash, decoded_record.value, decoded_record.length);
						*hash_length = decoded_record.length;
					}
					else {
						/* Provided buffer is too small */
						psa_status = PSA_ERROR_BUFFER_TOO_SMALL;
					}
				}
				else {
					/* Mandatory response parameter missing */
					psa_status = PSA_ERROR_GENERIC_ERROR;
				}
			}
		}

		rpc_caller_session_end(call_handle);
	}

	return psa_status;
}

static inline psa_status_t crypto_caller_hash_abort(struct service_client *context,
	uint32_t op_handle)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	struct ts_crypto_hash_abort_in req_msg;
	const size_t req_fixed_len = sizeof(struct ts_crypto_hash_abort_in);
	size_t req_len = req_fixed_len;

	req_msg.op_handle = op_handle;

	rpc_call_handle call_handle;
	uint8_t *req_buf;

	call_handle = rpc_caller_session_begin(context->session, &req_buf, req_len, 0);

	if (call_handle) {

		uint8_t *resp_buf;
		size_t resp_len;
		service_status_t service_status;

		memcpy(req_buf, &req_msg, req_fixed_len);

		context->rpc_status =
			rpc_caller_session_invoke(call_handle, TS_CRYPTO_OPCODE_HASH_ABORT,
						  &resp_buf, &resp_len, &service_status);

		if (context->rpc_status == RPC_SUCCESS)
			psa_status = service_status;

		rpc_caller_session_end(call_handle);
	}

	return psa_status;
}

static inline psa_status_t crypto_caller_hash_verify(struct service_client *context,
	uint32_t op_handle,
	const uint8_t *hash,
	size_t hash_length)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	struct ts_crypto_hash_verify_in req_msg;
	const size_t req_fixed_len = sizeof(struct ts_crypto_hash_verify_in);
	size_t req_len = req_fixed_len;

	req_msg.op_handle = op_handle;

	/* Mandatory input data parameter */
	struct tlv_record data_record;
	data_record.tag = TS_CRYPTO_HASH_VERIFY_IN_TAG_HASH;
	data_record.length = hash_length;
	data_record.value = hash;
	req_len += tlv_required_space(data_record.length);

	rpc_call_handle call_handle;
	uint8_t *req_buf;

	call_handle = rpc_caller_session_begin(context->session, &req_buf, req_len, 0);

	if (call_handle) {

		uint8_t *resp_buf;
		size_t resp_len;
		service_status_t service_status;
		struct tlv_iterator req_iter;

		memcpy(req_buf, &req_msg, req_fixed_len);

		tlv_iterator_begin(&req_iter, &req_buf[req_fixed_len], req_len - req_fixed_len);
		tlv_encode(&req_iter, &data_record);

		context->rpc_status =
			rpc_caller_session_invoke(call_handle, TS_CRYPTO_OPCODE_HASH_VERIFY,
						  &resp_buf, &resp_len, &service_status);

		if (context->rpc_status == RPC_SUCCESS)
			psa_status = service_status;

		rpc_caller_session_end(call_handle);
	}

	return psa_status;
}

static inline psa_status_t crypto_caller_hash_clone(struct service_client *context,
	uint32_t source_op_handle,
	uint32_t *target_op_handle)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	struct ts_crypto_hash_clone_in req_msg;
	const size_t req_fixed_len = sizeof(struct ts_crypto_hash_clone_in);
	size_t req_len = req_fixed_len;

	req_msg.source_op_handle = source_op_handle;

	rpc_call_handle call_handle;
	uint8_t *req_buf;

	call_handle = rpc_caller_session_begin(context->session, &req_buf, req_len, 0);

	if (call_handle) {

		uint8_t *resp_buf;
		size_t resp_len;
		service_status_t service_status;

		memcpy(req_buf, &req_msg, req_fixed_len);

		context->rpc_status =
			rpc_caller_session_invoke(call_handle, TS_CRYPTO_OPCODE_HASH_CLONE,
						  &resp_buf, &resp_len, &service_status);

		if (context->rpc_status == RPC_SUCCESS) {

			psa_status = service_status;

			if (psa_status == PSA_SUCCESS) {

				if (resp_len >= sizeof(struct ts_crypto_hash_clone_out)) {

					struct ts_crypto_hash_clone_out resp_msg;
					memcpy(&resp_msg, resp_buf, sizeof(struct ts_crypto_hash_clone_out));
					*target_op_handle = resp_msg.target_op_handle;
				}
				else {
					/* Failed to decode response message */
					psa_status = PSA_ERROR_GENERIC_ERROR;
				}
			}
		}

		rpc_caller_session_end(call_handle);
	}

	return psa_status;
}

static inline size_t crypto_caller_hash_max_update_size(const struct service_client *context)
{
	/* Returns the maximum number of bytes that may be
	 * carried as a parameter of the hash_update operation
	 * using the packed-c encoding.
	 */
	size_t payload_space = context->service_info.max_payload;
	size_t overhead = sizeof(struct ts_crypto_hash_update_in) + TLV_HDR_LEN;

	return (payload_space > overhead) ? payload_space - overhead : 0;
}

#ifdef __cplusplus
}
#endif

#endif /* PACKEDC_CRYPTO_CALLER_HASH_H */
