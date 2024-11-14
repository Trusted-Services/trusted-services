/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PACKEDC_CRYPTO_CALLER_GET_UEFI_PRIV_AUTH_VAR_FINGERPRINT_H
#define PACKEDC_CRYPTO_CALLER_GET_UEFI_PRIV_AUTH_VAR_FINGERPRINT_H

#include <common/tlv/tlv.h>
#include <protocols/common/efi/efi_status.h>
#include <protocols/rpc/common/packed-c/status.h>
#include <protocols/service/crypto/packed-c/opcodes.h>
#include <protocols/service/crypto/packed-c/get_uefi_priv_auth_var_fingerprint.h>
#include <service/common/client/service_client.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int crypto_caller_get_uefi_priv_auth_var_fingerprint(struct service_client *context,
						       const uint8_t *signature_cert,
						       uint64_t signature_cert_len,
						       uint8_t *output)
{
	efi_status_t efi_status = EFI_INVALID_PARAMETER;
	size_t req_len = 0;

	if (signature_cert_len > UINT16_MAX)
		return efi_status;

	struct tlv_record signature_record = {
		.tag = TS_CRYPTO_GET_UEFI_PRIV_AUTH_VAR_FINGERPRINT_IN_TAG_SIGNATURE,
		.length = (uint16_t)signature_cert_len,
		.value = signature_cert
	};

	req_len += tlv_required_space(signature_record.length);

	rpc_call_handle call_handle;
	uint8_t *req_buf;

	call_handle = rpc_caller_session_begin(context->session, &req_buf, req_len, 0);

	if (call_handle) {
		uint8_t *resp_buf;
		size_t resp_len;
		service_status_t service_status;
		struct tlv_iterator req_iter;

		tlv_iterator_begin(&req_iter, req_buf, req_len);
		if (!tlv_encode(&req_iter, &signature_record))
			return efi_status;

		context->rpc_status = rpc_caller_session_invoke(
			call_handle, TS_CRYPTO_OPCODE_GET_UEFI_PRIV_AUTH_VAR_FINGERPRINT, &resp_buf, &resp_len,
			&service_status);

		efi_status = (efi_status_t)service_status;

		if (context->rpc_status == RPC_SUCCESS) {

			if (efi_status == EFI_SUCCESS) {

				struct tlv_const_iterator resp_iter;
				struct tlv_record decoded_record;
				tlv_const_iterator_begin(&resp_iter, resp_buf, resp_len);

				if (tlv_find_decode(&resp_iter,
							TS_CRYPTO_GET_UEFI_PRIV_AUTH_VAR_FINGERPRINT_OUT_TAG_IDENTIFIER, &decoded_record)) {

					memcpy(output, decoded_record.value, PSA_HASH_MAX_SIZE);
					efi_status = EFI_SUCCESS;
				} else {
					/* Mandatory response parameter missing */
					efi_status = EFI_PROTOCOL_ERROR;
				}
			}
		}

		rpc_caller_session_end(call_handle);
	}

	return efi_status;
}

#ifdef __cplusplus
}
#endif

#endif /* PACKEDC_CRYPTO_CALLER_GET_UEFI_PRIV_AUTH_VAR_FINGERPRINT_H */
