/*
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PACKEDC_CRYPTO_CALLER_VERIFY_HASH_H
#define PACKEDC_CRYPTO_CALLER_VERIFY_HASH_H

#include <string.h>
#include <stdlib.h>
#include <psa/crypto.h>
#include <service/common/client/service_client.h>
#include <protocols/rpc/common/packed-c/status.h>
#include <protocols/service/crypto/packed-c/opcodes.h>
#include <protocols/service/crypto/packed-c/verify_hash.h>
#include <common/tlv/tlv.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline psa_status_t crypto_caller_asym_verify_common(struct service_client *context,
	uint32_t opcode,
	psa_key_id_t id,
	psa_algorithm_t alg,
	const uint8_t *hash, size_t hash_length,
	const uint8_t *signature, size_t signature_length)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	struct ts_crypto_verify_hash_in req_msg;
	const size_t req_fixed_len = sizeof(struct ts_crypto_verify_hash_in);
	size_t req_len = req_fixed_len +
		tlv_required_space(hash_length) + tlv_required_space(signature_length);

	req_msg.id = id;
	req_msg.alg = alg;

	struct tlv_record hash_record;
	hash_record.tag = TS_CRYPTO_VERIFY_HASH_IN_TAG_HASH;
	hash_record.length = hash_length;
	hash_record.value = hash;

	struct tlv_record sig_record;
	sig_record.tag = TS_CRYPTO_VERIFY_HASH_IN_TAG_SIGNATURE;
	sig_record.length = signature_length;
	sig_record.value = signature;

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
		tlv_encode(&req_iter, &hash_record);
		tlv_encode(&req_iter, &sig_record);

		context->rpc_status =
			rpc_caller_session_invoke(call_handle, opcode, &resp_buf, &resp_len,
						  &service_status);

		if (context->rpc_status == RPC_SUCCESS)
			psa_status = service_status;

		rpc_caller_session_end(call_handle);
	}

	return psa_status;
}

static inline psa_status_t crypto_caller_verify_hash(struct service_client *context,
	psa_key_id_t id,
	psa_algorithm_t alg,
	const uint8_t *hash, size_t hash_length,
	const uint8_t *signature, size_t signature_length)
{
	return crypto_caller_asym_verify_common(context,
		TS_CRYPTO_OPCODE_VERIFY_HASH,
		id, alg,
		hash, hash_length,
		signature, signature_length);
}

static inline psa_status_t crypto_caller_verify_message(struct service_client *context,
	psa_key_id_t id,
	psa_algorithm_t alg,
	const uint8_t *input, size_t input_length,
	const uint8_t *signature, size_t signature_length)
{
	return crypto_caller_asym_verify_common(context,
		TS_CRYPTO_OPCODE_VERIFY_MESSAGE,
		id, alg,
		input, input_length,
		signature, signature_length);
}

#ifdef __cplusplus
}
#endif

#endif /* PACKEDC_CRYPTO_CALLER_VERIFY_HASH_H */
