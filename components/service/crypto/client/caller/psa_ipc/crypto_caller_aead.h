/*
 * Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PSA_IPC_CRYPTO_CALLER_AEAD_H
#define PSA_IPC_CRYPTO_CALLER_AEAD_H

#include <string.h>
#include <stdlib.h>
#include <psa/crypto.h>
#include <psa/client.h>
#include <psa/sid.h>
#include <service/common/client/service_client.h>
#include <service/crypto/backend/psa_ipc/crypto_ipc_backend.h>
#include <protocols/rpc/common/packed-c/status.h>
#include <protocols/service/crypto/packed-c/opcodes.h>
#include <protocols/service/crypto/packed-c/key_attributes.h>
#include <protocols/service/crypto/packed-c/import_key.h>
#include "crypto_caller_key_attributes.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline psa_status_t crypto_caller_aead_encrypt(
					      struct service_client *context,
					      psa_key_id_t key,
					      psa_algorithm_t alg,
					      const uint8_t *nonce,
					      size_t nonce_length,
					      const uint8_t *additional_data,
					      size_t additional_data_length,
					      const uint8_t *plaintext,
					      size_t plaintext_length,
					      uint8_t *aeadtext,
					      size_t aeadtext_size,
					      size_t *aeadtext_length)
{
	struct service_client *ipc = context;
	struct rpc_caller_interface *caller = ipc->session->caller;
	psa_status_t status;
	size_t in_len;
	int i;
	struct psa_ipc_crypto_pack_iovec iov = {
		.function_id = TFM_CRYPTO_AEAD_ENCRYPT_SID,
		.key_id = key,
		.alg = alg,
		.aead_in = { .nonce = {0}, .nonce_length = nonce_length },
	};

	if (!additional_data && additional_data_length)
		return PSA_ERROR_INVALID_ARGUMENT;

	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&iov), .len = iov_size },
		{ .base = psa_ptr_const_to_u32(plaintext),
			.len = plaintext_length },
		{ .base = psa_ptr_const_to_u32(additional_data),
			.len = additional_data_length},
	};
	struct psa_outvec out_vec[] = {
		{ .base = psa_ptr_to_u32(aeadtext), .len = aeadtext_size },
	};

	if (nonce_length > TFM_CRYPTO_MAX_NONCE_LENGTH)
		return PSA_ERROR_INVALID_ARGUMENT;

	if (nonce) {
		for (i = 0; i < nonce_length; i++)
			iov.aead_in.nonce[i] = nonce[i];
	}

	in_len = IOVEC_LEN(in_vec);

	if (!additional_data)
		in_len--;

	status = psa_call(caller, TFM_CRYPTO_HANDLE, PSA_IPC_CALL, in_vec,
			  in_len, out_vec, IOVEC_LEN(out_vec));

	*aeadtext_length = out_vec[0].len;

	return status;
}

static inline psa_status_t crypto_caller_aead_decrypt(
					      struct service_client *context,
					      psa_key_id_t key,
					      psa_algorithm_t alg,
					      const uint8_t *nonce,
					      size_t nonce_length,
					      const uint8_t *additional_data,
					      size_t additional_data_length,
					      const uint8_t *aeadtext,
					      size_t aeadtext_length,
					      uint8_t *plaintext,
					      size_t plaintext_size,
					      size_t *plaintext_length)
{
	struct service_client *ipc = context;
	struct rpc_caller_interface *caller = ipc->session->caller;
	psa_status_t status;
	size_t in_len;
	int i;
	struct psa_ipc_crypto_pack_iovec iov = {
		.function_id = TFM_CRYPTO_AEAD_DECRYPT_SID,
		.key_id = key,
		.alg = alg,
		.aead_in = { .nonce = {0}, .nonce_length = nonce_length },
	};

	if (!additional_data && additional_data_length)
		return PSA_ERROR_INVALID_ARGUMENT;

	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&iov), .len = iov_size },
		{ .base = psa_ptr_const_to_u32(aeadtext),
			.len = aeadtext_length },
		{ .base = psa_ptr_const_to_u32(additional_data),
			.len = additional_data_length},
	};
	struct psa_outvec out_vec[] = {
		{ .base = psa_ptr_to_u32(plaintext), .len = plaintext_size },
	};

	if (nonce_length > TFM_CRYPTO_MAX_NONCE_LENGTH)
		return PSA_ERROR_INVALID_ARGUMENT;

	if (nonce) {
		for (i = 0; i < nonce_length; i++)
			iov.aead_in.nonce[i] = nonce[i];
	}

	in_len = IOVEC_LEN(in_vec);

	if (!additional_data)
		in_len--;

	status = psa_call(caller, TFM_CRYPTO_HANDLE, PSA_IPC_CALL, in_vec,
			  in_len, out_vec, IOVEC_LEN(out_vec));

	*plaintext_length = out_vec[0].len;

	return status;
}

static inline psa_status_t crypto_caller_aead_encrypt_setup(
					    struct service_client *context,
					    uint32_t *op_handle,
					    psa_key_id_t key,
					    psa_algorithm_t alg)
{
	struct service_client *ipc = context;
	struct rpc_caller_interface *caller = ipc->session->caller;
	psa_status_t status;
	struct psa_ipc_crypto_pack_iovec iov = {
		.function_id = TFM_CRYPTO_AEAD_ENCRYPT_SETUP_SID,
		.key_id = key,
		.alg = alg,
		.op_handle = (*op_handle),
	};

	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&iov),
			.len = sizeof(struct psa_ipc_crypto_pack_iovec) },
	};
	struct psa_outvec out_vec[] = {
		{ .base = psa_ptr_to_u32(op_handle), .len = sizeof(uint32_t) },
	};

	status = psa_call(caller, TFM_CRYPTO_HANDLE, PSA_IPC_CALL, in_vec,
			  IOVEC_LEN(in_vec), out_vec, IOVEC_LEN(out_vec));

	return status;
}

static inline psa_status_t crypto_caller_aead_decrypt_setup(
					    struct service_client *context,
					    uint32_t *op_handle,
					    psa_key_id_t key,
					    psa_algorithm_t alg)
{
	struct service_client *ipc = context;
	struct rpc_caller_interface *caller = ipc->session->caller;
	psa_status_t status;
	struct psa_ipc_crypto_pack_iovec iov = {
		.function_id = TFM_CRYPTO_AEAD_DECRYPT_SETUP_SID,
		.key_id = key,
		.alg = alg,
		.op_handle = (*op_handle),
	};

	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&iov),
			.len = sizeof(struct psa_ipc_crypto_pack_iovec) }
	};
	struct psa_outvec out_vec[] = {
		{ .base = psa_ptr_to_u32(op_handle), .len = sizeof(uint32_t) }
	};

	status = psa_call(caller, TFM_CRYPTO_HANDLE, PSA_IPC_CALL, in_vec,
			  IOVEC_LEN(in_vec), out_vec, IOVEC_LEN(out_vec));

	return status;
}

static inline psa_status_t crypto_caller_aead_generate_nonce(
					     struct service_client *context,
					     uint32_t op_handle,
					     uint8_t *nonce,
					     size_t nonce_size,
					     size_t *nonce_length)
{
	struct service_client *ipc = context;
	struct rpc_caller_interface *caller = ipc->session->caller;
	psa_status_t status;
	struct psa_ipc_crypto_pack_iovec iov = {
		.function_id = TFM_CRYPTO_AEAD_GENERATE_NONCE_SID,
		.op_handle = op_handle,
	};

	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&iov),
			.len = sizeof(struct psa_ipc_crypto_pack_iovec) },
	};
	struct psa_outvec out_vec[] = {
		{ .base = psa_ptr_to_u32(nonce), .len = nonce_size },
	};

	status = psa_call(caller, TFM_CRYPTO_HANDLE, PSA_IPC_CALL, in_vec,
			  IOVEC_LEN(in_vec), out_vec, IOVEC_LEN(out_vec));

	*nonce_length = out_vec[0].len;

	return status;
}

static inline psa_status_t crypto_caller_aead_set_nonce(
						struct service_client *context,
						uint32_t op_handle,
						const uint8_t *nonce,
						size_t nonce_length)
{
	struct service_client *ipc = context;
	struct rpc_caller_interface *caller = ipc->session->caller;
	psa_status_t status;
	struct psa_ipc_crypto_pack_iovec iov = {
		.function_id = TFM_CRYPTO_AEAD_SET_NONCE_SID,
		.op_handle = op_handle,
	};

	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&iov),
			.len = sizeof(struct psa_ipc_crypto_pack_iovec) },
		{ .base = psa_ptr_const_to_u32(nonce), .len = nonce_length },
	};
	struct psa_outvec out_vec[] = {
		{ .base = psa_ptr_to_u32(&op_handle), .len = sizeof(uint32_t) },
	};

	status = psa_call(caller, TFM_CRYPTO_HANDLE, PSA_IPC_CALL, in_vec,
			  IOVEC_LEN(in_vec), out_vec, IOVEC_LEN(out_vec));

	return status;
}

static inline psa_status_t crypto_caller_aead_set_lengths(
					  struct service_client *context,
					  uint32_t op_handle,
					  size_t ad_length,
					  size_t plaintext_length)
{
	struct service_client *ipc = context;
	struct rpc_caller_interface *caller = ipc->session->caller;
	psa_status_t status;
	struct psa_ipc_crypto_pack_iovec iov = {
		.function_id = TFM_CRYPTO_AEAD_SET_LENGTHS_SID,
		.ad_length = ad_length,
		.plaintext_length = plaintext_length,
		.op_handle = op_handle,
	};

	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&iov),
			.len = sizeof(struct psa_ipc_crypto_pack_iovec) },
	};
	struct psa_outvec out_vec[] = {
		{ .base = psa_ptr_to_u32(&op_handle), .len = sizeof(uint32_t) },
	};

	status = psa_call(caller, TFM_CRYPTO_HANDLE, PSA_IPC_CALL, in_vec,
			  IOVEC_LEN(in_vec), out_vec, IOVEC_LEN(out_vec));

	return status;
}

static inline psa_status_t crypto_caller_aead_update_ad(
						struct service_client *context,
						uint32_t op_handle,
						const uint8_t *input,
						size_t input_length)
{
	struct service_client *ipc = context;
	struct rpc_caller_interface *caller = ipc->session->caller;
	psa_status_t status;
	struct psa_ipc_crypto_pack_iovec iov = {
		.function_id = TFM_CRYPTO_AEAD_UPDATE_AD_SID,
		.op_handle = op_handle,
	};

	/* Sanitize the optional input */
	if ((input == NULL) && (input_length != 0)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&iov),
			.len = sizeof(struct psa_ipc_crypto_pack_iovec) },
		{ .base = psa_ptr_const_to_u32(input), .len = input_length },
	};
	struct psa_outvec out_vec[] = {
		{ .base = psa_ptr_to_u32(&op_handle), .len = sizeof(uint32_t) },
	};

	size_t in_len = IOVEC_LEN(in_vec);

	if (input == NULL)
		in_len--;

	status = psa_call(caller, TFM_CRYPTO_HANDLE, PSA_IPC_CALL, in_vec,
			  in_len, out_vec, IOVEC_LEN(out_vec));

	return status;
}

static inline psa_status_t crypto_caller_aead_update(
					     struct service_client *context,
					     uint32_t op_handle,
					     const uint8_t *input,
					     size_t input_length,
					     uint8_t *output,
					     size_t output_size,
					     size_t *output_length)
{
	struct service_client *ipc = context;
	struct rpc_caller_interface *caller = ipc->session->caller;
	psa_status_t status;
	struct psa_ipc_crypto_pack_iovec iov = {
		.function_id = TFM_CRYPTO_AEAD_UPDATE_SID,
		.op_handle = op_handle,
	};

	/* Sanitize the optional input */
	if ((input == NULL) && (input_length != 0)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&iov),
			.len = sizeof(struct psa_ipc_crypto_pack_iovec) },
		{ .base = psa_ptr_const_to_u32(input), .len = input_length },
	};
	struct psa_outvec out_vec[] = {
		{ .base = psa_ptr_const_to_u32(output), .len = output_size },
	};

	size_t in_len = IOVEC_LEN(in_vec);

	if (input == NULL)
		in_len--;

	status = psa_call(caller, TFM_CRYPTO_HANDLE, PSA_IPC_CALL, in_vec,
			  in_len, out_vec, IOVEC_LEN(out_vec));

	*output_length = out_vec[0].len;

	return status;
}

static inline psa_status_t crypto_caller_aead_finish(
					     struct service_client *context,
					     uint32_t *op_handle,
					     uint8_t *aeadtext,
					     size_t aeadtext_size,
					     size_t *aeadtext_length,
					     uint8_t *tag,
					     size_t tag_size,
					     size_t *tag_length)
{
	struct service_client *ipc = context;
	struct rpc_caller_interface *caller = ipc->session->caller;
	psa_status_t status;
	struct psa_ipc_crypto_pack_iovec iov = {
		.function_id = TFM_CRYPTO_AEAD_FINISH_SID,
		.op_handle = *op_handle,
	};

	/* Sanitize the optional output */
	if ((aeadtext == NULL) && (aeadtext_size != 0)) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&iov),
			.len = sizeof(struct psa_ipc_crypto_pack_iovec) },
	};
	struct psa_outvec out_vec[] = {
		{ .base = psa_ptr_to_u32(op_handle), .len = sizeof(uint32_t) },
		{ .base = psa_ptr_const_to_u32(tag), .len = tag_size },
		{ .base = psa_ptr_const_to_u32(aeadtext), .len = aeadtext_size }
	};

	size_t out_len = IOVEC_LEN(out_vec);

	if (aeadtext == NULL || aeadtext_size == 0)
		out_len--;

	if ((out_len == 3) && (aeadtext_length == NULL))
		return PSA_ERROR_INVALID_ARGUMENT;

	status = psa_call(caller, TFM_CRYPTO_HANDLE, PSA_IPC_CALL, in_vec,
			  IOVEC_LEN(in_vec), out_vec, out_len);

	*tag_length = out_vec[1].len;

	if (out_len == 3)
		*aeadtext_length = out_vec[2].len;
	else
		*aeadtext_length = 0;

	return status;
}

static inline psa_status_t crypto_caller_aead_verify(
					     struct service_client *context,
					     uint32_t *op_handle,
					     uint8_t *plaintext,
					     size_t plaintext_size,
					     size_t *plaintext_length,
					     const uint8_t *tag,
					     size_t tag_length)
{
	struct service_client *ipc = context;
	struct rpc_caller_interface *caller = ipc->session->caller;
	psa_status_t status;
	struct psa_ipc_crypto_pack_iovec iov = {
		.function_id = TFM_CRYPTO_AEAD_VERIFY_SID,
		.op_handle = *op_handle,
	};

	/* Sanitize the optional output */
	if ((plaintext == NULL) && (plaintext_size != 0))
		return PSA_ERROR_INVALID_ARGUMENT;

	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&iov),
			.len = sizeof(struct psa_ipc_crypto_pack_iovec) },
		{ .base = psa_ptr_const_to_u32(tag), .len = tag_length },
	};
	struct psa_outvec out_vec[] = {
		{ .base = psa_ptr_to_u32(op_handle), .len = sizeof(uint32_t) },
		{ .base = psa_ptr_const_to_u32(plaintext),
			.len = plaintext_size },
	};

	size_t out_len = IOVEC_LEN(out_vec);

	if (plaintext == NULL || plaintext_size == 0)
		out_len--;

	if ((out_len == 2) && (plaintext_length == NULL))
		return PSA_ERROR_INVALID_ARGUMENT;

	status = psa_call(caller, TFM_CRYPTO_HANDLE, PSA_IPC_CALL, in_vec,
			  IOVEC_LEN(in_vec), out_vec, out_len);

	if (out_len == 2)
		*plaintext_length = out_vec[1].len;
	else
		*plaintext_length = 0;

	return status;
}

static inline psa_status_t crypto_caller_aead_abort(
					    struct service_client *context,
					    uint32_t *op_handle)
{
	struct service_client *ipc = context;
	struct rpc_caller_interface *caller = ipc->session->caller;
	psa_status_t status;
	struct psa_ipc_crypto_pack_iovec iov = {
		.function_id = TFM_CRYPTO_AEAD_ABORT_SID,
		.op_handle = *op_handle,
	};

	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&iov),
			.len = sizeof(struct psa_ipc_crypto_pack_iovec) },
	};
	struct psa_outvec out_vec[] = {
		{ .base = psa_ptr_to_u32(op_handle), .len = sizeof(uint32_t) },
	};

	status = psa_call(caller, TFM_CRYPTO_HANDLE, PSA_IPC_CALL, in_vec,
			  IOVEC_LEN(in_vec), out_vec, IOVEC_LEN(out_vec));

	return status;
}

static inline size_t crypto_caller_aead_max_update_size(
					const struct service_client *context)
{
       /* Returns the maximum number of bytes that may be
        * carried as a parameter of the mac_update operation
        *  using the packed-c encoding.
        */
       size_t payload_space = context->service_info.max_payload;
       size_t overhead = iov_size;

       return (payload_space > overhead) ? payload_space - overhead : 0;
}

static inline size_t crypto_caller_aead_max_update_ad_size(
					   const struct service_client *context)
{
	/* Returns the maximum number of bytes that may be
	 * carried as a parameter of the mac_update operation
	 *  using the packed-c encoding.
	 */
	size_t payload_space = context->service_info.max_payload;
	size_t overhead = iov_size;

	return (payload_space > overhead) ? payload_space - overhead : 0;
}

#ifdef __cplusplus
}
#endif

#endif /* PSA_IPC_CRYPTO_CALLER_AEAD_H */
