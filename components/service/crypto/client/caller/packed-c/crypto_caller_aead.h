/*
 * Copyright (c) 2021-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PACKEDC_CRYPTO_CALLER_AEAD_H
#define PACKEDC_CRYPTO_CALLER_AEAD_H

#include <string.h>
#include <stdlib.h>
#include <psa/crypto.h>
#include "rpc_caller_session.h"
#include <service/common/client/service_client.h>
#include <protocols/rpc/common/packed-c/status.h>
#include <protocols/service/crypto/packed-c/opcodes.h>
#include <protocols/service/crypto/packed-c/aead.h>
#include <common/tlv/tlv.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline psa_status_t common_aead_setup(struct service_client *context,
	uint32_t *op_handle,
	psa_key_id_t key,
	psa_algorithm_t alg,
	uint32_t opcode)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	struct ts_crypto_aead_setup_in req_msg;
	size_t req_len = sizeof(struct ts_crypto_aead_setup_in);

	req_msg.key_id = key;
	req_msg.alg = alg;

	rpc_call_handle call_handle;
	uint8_t *req_buf;

	call_handle = rpc_caller_session_begin(context->session, &req_buf, req_len,
					       sizeof(struct ts_crypto_aead_setup_out));

	if (call_handle) {

		uint8_t *resp_buf;
		size_t resp_len;
		service_status_t service_status;

		memcpy(req_buf, &req_msg, req_len);

		context->rpc_status =
			rpc_caller_session_invoke(call_handle, opcode,
				&resp_buf, &resp_len, &service_status);

		if (context->rpc_status == RPC_SUCCESS) {

			psa_status = service_status;

			if (psa_status == PSA_SUCCESS) {

				if (resp_len >= sizeof(struct ts_crypto_aead_setup_out)) {

					struct ts_crypto_aead_setup_out resp_msg;
					memcpy(&resp_msg, resp_buf, sizeof(struct ts_crypto_aead_setup_out));
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

static inline psa_status_t crypto_caller_aead_encrypt_setup(struct service_client *context,
	uint32_t *op_handle,
	psa_key_id_t key,
	psa_algorithm_t alg)
{
	return common_aead_setup(context,
		op_handle, key, alg, TS_CRYPTO_OPCODE_AEAD_ENCRYPT_SETUP);
}

static inline psa_status_t crypto_caller_aead_decrypt_setup(struct service_client *context,
	uint32_t *op_handle,
	psa_key_id_t key,
	psa_algorithm_t alg)
{
	return common_aead_setup(context,
		op_handle, key, alg, TS_CRYPTO_OPCODE_AEAD_DECRYPT_SETUP);
}

static inline psa_status_t crypto_caller_aead_generate_nonce(struct service_client *context,
	uint32_t op_handle,
	uint8_t *nonce,
	size_t nonce_size,
	size_t *nonce_length)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	struct ts_crypto_aead_generate_nonce_in req_msg;
	size_t req_fixed_len = sizeof(struct ts_crypto_aead_generate_nonce_in);
	size_t req_len = req_fixed_len;

	*nonce_length = 0;
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
			rpc_caller_session_invoke(call_handle, TS_CRYPTO_OPCODE_AEAD_GENERATE_NONCE,
						  &resp_buf, &resp_len, &service_status);

		if (context->rpc_status == RPC_SUCCESS) {

			psa_status = service_status;

			if (psa_status == PSA_SUCCESS) {

				struct tlv_const_iterator resp_iter;
				struct tlv_record decoded_record;
				tlv_const_iterator_begin(&resp_iter, resp_buf, resp_len);

				if (tlv_find_decode(&resp_iter,
					TS_CRYPTO_AEAD_GENERATE_NONCE_OUT_TAG_NONCE, &decoded_record)) {

					if (decoded_record.length <= nonce_size) {

						memcpy(nonce, decoded_record.value, decoded_record.length);
						*nonce_length = decoded_record.length;
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

static inline psa_status_t crypto_caller_aead_set_nonce(struct service_client *context,
	uint32_t op_handle,
	const uint8_t *nonce,
	size_t nonce_length)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	struct ts_crypto_aead_set_nonce_in req_msg;
	size_t req_fixed_len = sizeof(struct ts_crypto_aead_set_nonce_in);
	size_t req_len = req_fixed_len;

	req_msg.op_handle = op_handle;

	/* Mandatory input data parameter */
	struct tlv_record data_record;
	data_record.tag = TS_CRYPTO_AEAD_SET_NONCE_IN_TAG_NONCE;
	data_record.length = nonce_length;
	data_record.value = nonce;
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
			rpc_caller_session_invoke(call_handle, TS_CRYPTO_OPCODE_AEAD_SET_NONCE,
						  &resp_buf, &resp_len, &service_status);

		if (context->rpc_status == RPC_SUCCESS)
			psa_status = service_status;

		rpc_caller_session_end(call_handle);
	}

	return psa_status;
}

static inline psa_status_t crypto_caller_aead_set_lengths(struct service_client *context,
	uint32_t op_handle,
	size_t ad_length,
	size_t plaintext_length)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	struct ts_crypto_aead_set_lengths_in req_msg;
	size_t req_fixed_len = sizeof(struct ts_crypto_aead_set_lengths_in);
	size_t req_len = req_fixed_len;

	req_msg.op_handle = op_handle;
	req_msg.ad_length = ad_length;
	req_msg.plaintext_length = plaintext_length;

	rpc_call_handle call_handle;
	uint8_t *req_buf;

	call_handle = rpc_caller_session_begin(context->session, &req_buf, req_len, 0);

	if (call_handle) {

		uint8_t *resp_buf;
		size_t resp_len;
		service_status_t service_status;

		memcpy(req_buf, &req_msg, req_fixed_len);

		context->rpc_status =
			rpc_caller_session_invoke(call_handle, TS_CRYPTO_OPCODE_AEAD_SET_LENGTHS,
						  &resp_buf, &resp_len, &service_status);

		if (context->rpc_status == RPC_SUCCESS)
			psa_status = service_status;

		rpc_caller_session_end(call_handle);
	}

	return psa_status;
}

static inline psa_status_t crypto_caller_aead_update_ad(struct service_client *context,
	uint32_t op_handle,
	const uint8_t *input,
	size_t input_length)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	struct ts_crypto_aead_update_ad_in req_msg;
	size_t req_fixed_len = sizeof(struct ts_crypto_aead_update_ad_in);
	size_t req_len = req_fixed_len;

	req_msg.op_handle = op_handle;

	/* Mandatory input data parameter */
	struct tlv_record data_record;
	data_record.tag = TS_CRYPTO_AEAD_UPDATE_AD_IN_TAG_DATA;
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
			rpc_caller_session_invoke(call_handle, TS_CRYPTO_OPCODE_AEAD_UPDATE_AD,
						  &resp_buf, &resp_len, &service_status);

		if (context->rpc_status == RPC_SUCCESS)
			psa_status = service_status;

		rpc_caller_session_end(call_handle);
	}

	return psa_status;
}

static inline psa_status_t crypto_caller_aead_update(struct service_client *context,
	uint32_t op_handle,
	const uint8_t *input,
	size_t input_length,
	uint8_t *output,
	size_t output_size,
	size_t *output_length)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	struct ts_crypto_aead_update_in req_msg;
	size_t req_fixed_len = sizeof(struct ts_crypto_aead_update_in);
	size_t req_len = req_fixed_len;

	*output_length = 0;
	req_msg.op_handle = op_handle;

	/* Mandatory input data parameter */
	struct tlv_record data_record;
	data_record.tag = TS_CRYPTO_AEAD_UPDATE_IN_TAG_DATA;
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
			rpc_caller_session_invoke(call_handle, TS_CRYPTO_OPCODE_AEAD_UPDATE,
						  &resp_buf, &resp_len, &service_status);

		if (context->rpc_status == RPC_SUCCESS) {

			psa_status = service_status;

			if (psa_status == PSA_SUCCESS) {

				struct tlv_const_iterator resp_iter;
				struct tlv_record decoded_record;
				tlv_const_iterator_begin(&resp_iter, resp_buf, resp_len);

				if (tlv_find_decode(&resp_iter,
					TS_CRYPTO_AEAD_UPDATE_OUT_TAG_DATA, &decoded_record)) {

					if (decoded_record.length <= output_size) {

						memcpy(output, decoded_record.value, decoded_record.length);
						*output_length = decoded_record.length;
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

static inline psa_status_t crypto_caller_aead_finish(struct service_client *context,
	uint32_t op_handle,
	uint8_t *aeadtext,
	size_t aeadtext_size,
	size_t *aeadtext_length,
	uint8_t *tag,
	size_t tag_size,
	size_t *tag_length)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	struct ts_crypto_aead_finish_in req_msg;
	size_t req_fixed_len = sizeof(struct ts_crypto_aead_finish_in);
	size_t req_len = req_fixed_len;

	*aeadtext_length = 0;
	*tag_length = 0;
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
			rpc_caller_session_invoke(call_handle, TS_CRYPTO_OPCODE_AEAD_FINISH,
						  &resp_buf, &resp_len, &service_status);

		if (context->rpc_status == RPC_SUCCESS) {

			psa_status = service_status;

			if (psa_status == PSA_SUCCESS) {

				struct tlv_const_iterator resp_iter;
				struct tlv_record decoded_record;
				tlv_const_iterator_begin(&resp_iter, resp_buf, resp_len);

				if (tlv_find_decode(&resp_iter,
					TS_CRYPTO_AEAD_FINISH_OUT_TAG_CIPHERTEXT, &decoded_record)) {

					if (decoded_record.length <= aeadtext_size) {

						if (decoded_record.length && aeadtext)
							memcpy(aeadtext, decoded_record.value,
							       decoded_record.length);

						*aeadtext_length = decoded_record.length;
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

				if ((psa_status == PSA_SUCCESS) && tlv_find_decode(&resp_iter,
					TS_CRYPTO_AEAD_FINISH_OUT_TAG_TAG, &decoded_record)) {

					if (decoded_record.length <= tag_size) {

						memcpy(tag, decoded_record.value, decoded_record.length);
						*tag_length = decoded_record.length;
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

static inline psa_status_t crypto_caller_aead_verify(struct service_client *context,
	uint32_t op_handle,
	uint8_t *plaintext,
	size_t plaintext_size,
	size_t *plaintext_length,
	const uint8_t *tag,
	size_t tag_length)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	struct ts_crypto_aead_verify_in req_msg;
	size_t req_fixed_len = sizeof(struct ts_crypto_aead_verify_in);
	size_t req_len = req_fixed_len;

	*plaintext_length = 0;
	req_msg.op_handle = op_handle;

	/* Mandatory input data parameter */
	struct tlv_record data_record;
	data_record.tag = TS_CRYPTO_AEAD_VERIFY_IN_TAG_TAG;
	data_record.length = tag_length;
	data_record.value = tag;
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
			rpc_caller_session_invoke(call_handle, TS_CRYPTO_OPCODE_AEAD_VERIFY,
						  &resp_buf, &resp_len, &service_status);

		if (context->rpc_status == RPC_SUCCESS) {

			psa_status = service_status;

			if (psa_status == PSA_SUCCESS) {

				struct tlv_const_iterator resp_iter;
				struct tlv_record decoded_record;
				tlv_const_iterator_begin(&resp_iter, resp_buf, resp_len);

				if (tlv_find_decode(&resp_iter,
					TS_CRYPTO_AEAD_VERIFY_OUT_TAG_PLAINTEXT, &decoded_record)) {

					if (decoded_record.length <= plaintext_size) {

						if (decoded_record.length && plaintext)
							memcpy(plaintext, decoded_record.value,
							       decoded_record.length);

						*plaintext_length = decoded_record.length;
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

static inline psa_status_t crypto_caller_aead_abort(struct service_client *context,
	uint32_t op_handle)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	struct ts_crypto_aead_abort_in req_msg;
	size_t req_fixed_len = sizeof(struct ts_crypto_aead_abort_in);
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
			rpc_caller_session_invoke(call_handle, TS_CRYPTO_OPCODE_AEAD_ABORT,
						  &resp_buf, &resp_len, &service_status);

		if (context->rpc_status == RPC_SUCCESS)
			psa_status = service_status;

		rpc_caller_session_end(call_handle);
	}

	return psa_status;
}

/**
 * The maximum data length that may be carried in an update operation will be
 * constrained by the maximum call payload capacity imposed by the end-to-end
 * RPC call path. These functions return the maximum update size when serialization
 * overheads are considered. This allows large paylaods to be processed in
 * maximum size chunks.
 */
static inline size_t crypto_caller_aead_max_update_ad_size(const struct service_client *context)
{
	/* Returns the maximum number of bytes of additional data that may be
	 * carried as a parameter of the aead_update_ad operation
	 * using the packed-c encoding.
	 */
	size_t payload_space = context->service_info.max_payload;
	size_t overhead = sizeof(struct ts_crypto_aead_update_ad_in) + TLV_HDR_LEN;

	return (payload_space > overhead) ? payload_space - overhead : 0;
}

static inline size_t crypto_caller_aead_max_update_size(const struct service_client *context)
{
	/* Returns the maximum number of bytes that may be
	 * carried as a parameter of the aead_update operation
	 * using the packed-c encoding.
	 */
	size_t payload_space = context->service_info.max_payload;
	size_t overhead = sizeof(struct ts_crypto_aead_update_in) + TLV_HDR_LEN;

	/* Allow for output to be a whole number of blocks */
	overhead += PSA_BLOCK_CIPHER_BLOCK_MAX_SIZE;

	return (payload_space > overhead) ? payload_space - overhead : 0;
}

#ifdef __cplusplus
}
#endif

#endif /* PACKEDC_CRYPTO_CALLER_AEAD_H */
