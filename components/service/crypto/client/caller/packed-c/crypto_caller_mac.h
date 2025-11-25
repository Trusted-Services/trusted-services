/*
 * Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PACKEDC_CRYPTO_CALLER_MAC_H
#define PACKEDC_CRYPTO_CALLER_MAC_H

#include <string.h>
#include <stdlib.h>
#include <psa/crypto.h>
#include <service/common/client/service_client.h>
#include <protocols/rpc/common/packed-c/status.h>
#include <protocols/service/crypto/packed-c/opcodes.h>
#include <protocols/service/crypto/packed-c/mac.h>
#include <common/tlv/tlv.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline psa_status_t common_mac_setup(struct service_client *context,
	uint32_t *op_handle,
	psa_key_id_t key,
	psa_algorithm_t alg,
	uint32_t opcode)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	struct ts_crypto_mac_setup_in req_msg;
	size_t req_len = sizeof(struct ts_crypto_mac_setup_in);

	req_msg.key_id = key;
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
			rpc_caller_session_invoke(call_handle, opcode, &resp_buf, &resp_len,
						  &service_status);

		if (context->rpc_status == RPC_SUCCESS) {

			psa_status = service_status;

			if (psa_status == PSA_SUCCESS) {

				if (resp_len >= sizeof(struct ts_crypto_mac_setup_out)) {

					struct ts_crypto_mac_setup_out resp_msg;
					memcpy(&resp_msg, resp_buf, sizeof(struct ts_crypto_mac_setup_out));
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

static inline psa_status_t crypto_caller_mac_sign_setup(struct service_client *context,
	uint32_t *op_handle,
	psa_key_id_t key,
	psa_algorithm_t alg)
{
	return common_mac_setup(context, op_handle, key, alg, TS_CRYPTO_OPCODE_MAC_SIGN_SETUP);
}

static inline psa_status_t crypto_caller_mac_verify_setup(struct service_client *context,
	uint32_t *op_handle,
	psa_key_id_t key,
	psa_algorithm_t alg)
{
	return common_mac_setup(context, op_handle, key, alg, TS_CRYPTO_OPCODE_MAC_VERIFY_SETUP);
}

static inline psa_status_t crypto_caller_mac_update(struct service_client *context,
	uint32_t op_handle,
	const uint8_t *input,
	size_t input_length)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	struct ts_crypto_mac_update_in req_msg;
	const size_t req_fixed_len = sizeof(struct ts_crypto_mac_update_in);
	size_t req_len = req_fixed_len;

	req_msg.op_handle = op_handle;

	/* Mandatory input data parameter */
	struct tlv_record data_record;
	data_record.tag = TS_CRYPTO_MAC_UPDATE_IN_TAG_DATA;
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
			rpc_caller_session_invoke(call_handle,  TS_CRYPTO_OPCODE_MAC_UPDATE,
						  &resp_buf, &resp_len, &service_status);

		if (context->rpc_status == RPC_SUCCESS)
			psa_status = service_status;

		rpc_caller_session_end(call_handle);
	}

	return psa_status;
}

static inline psa_status_t crypto_caller_mac_sign_finish(struct service_client *context,
	uint32_t op_handle,
	uint8_t *mac,
	size_t mac_size,
	size_t *mac_length)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	struct ts_crypto_mac_sign_finish_in req_msg;
	const size_t req_fixed_len = sizeof(struct ts_crypto_mac_sign_finish_in);
	size_t req_len = req_fixed_len;

	*mac_length = 0;
	req_msg.op_handle = op_handle;

	rpc_call_handle call_handle;
	uint8_t *req_buf;

	call_handle = rpc_caller_session_begin(context->session, &req_buf, req_len,
					       tlv_required_space(mac_size));

	if (call_handle) {

		uint8_t *resp_buf;
		size_t resp_len;
		service_status_t service_status;

		memcpy(req_buf, &req_msg, req_fixed_len);

		context->rpc_status =
			rpc_caller_session_invoke(call_handle, TS_CRYPTO_OPCODE_MAC_SIGN_FINISH,
						  &resp_buf, &resp_len, &service_status);

		if (context->rpc_status == RPC_SUCCESS) {

			psa_status = service_status;

			if (psa_status == PSA_SUCCESS) {

				struct tlv_const_iterator resp_iter;
				struct tlv_record decoded_record;
				tlv_const_iterator_begin(&resp_iter, resp_buf, resp_len);

				if (tlv_find_decode(&resp_iter,
					TS_CRYPTO_MAC_SIGN_FINISH_OUT_TAG_MAC, &decoded_record)) {

					if (decoded_record.length <= mac_size) {

						memcpy(mac, decoded_record.value, decoded_record.length);
						*mac_length = decoded_record.length;
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

static inline psa_status_t crypto_caller_mac_verify_finish(struct service_client *context,
	uint32_t op_handle,
	const uint8_t *mac,
	size_t mac_length)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	struct ts_crypto_mac_verify_finish_in req_msg;
	const size_t req_fixed_len = sizeof(struct ts_crypto_mac_verify_finish_in);
	size_t req_len = req_fixed_len;

	req_msg.op_handle = op_handle;

	/* Mandatory input data parameter */
	struct tlv_record data_record;
	data_record.tag = TS_CRYPTO_MAC_VERIFY_FINISH_IN_TAG_MAC;
	data_record.length = mac_length;
	data_record.value = mac;
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
			rpc_caller_session_invoke(call_handle, TS_CRYPTO_OPCODE_MAC_VERIFY_FINISH,
						  &resp_buf, &resp_len, &service_status);

		if (context->rpc_status == RPC_SUCCESS)
			psa_status = service_status;

		rpc_caller_session_end(call_handle);
	}

	return psa_status;
}

static inline psa_status_t crypto_caller_mac_abort(struct service_client *context,
	uint32_t op_handle)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	struct ts_crypto_mac_abort_in req_msg;
	const size_t req_fixed_len = sizeof(struct ts_crypto_mac_abort_in);
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
			rpc_caller_session_invoke(call_handle, TS_CRYPTO_OPCODE_MAC_ABORT,
						  &resp_buf, &resp_len, &service_status);

		if (context->rpc_status == RPC_SUCCESS)
			psa_status = service_status;

		rpc_caller_session_end(call_handle);
	}

	return psa_status;
}

static inline size_t crypto_caller_mac_max_update_size(const struct service_client *context)
{
	/* Returns the maximum number of bytes that may be
	 * carried as a parameter of the mac_update operation
	 * using the packed-c encoding.
	 */
	size_t payload_space = context->service_info.max_payload;
	size_t overhead = sizeof(struct ts_crypto_mac_update_in) + TLV_HDR_LEN;

	return (payload_space > overhead) ? payload_space - overhead : 0;
}

#ifdef __cplusplus
}
#endif

#endif /* PACKEDC_CRYPTO_CALLER_MAC_H */
