/*
 * Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PSA_IPC_CRYPTO_CALLER_KEY_DERIVATION_H
#define PSA_IPC_CRYPTO_CALLER_KEY_DERIVATION_H

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

static inline psa_status_t crypto_caller_key_derivation_setup(
					      struct service_client *context,
					      uint32_t *op_handle,
					      psa_algorithm_t alg)
{
	struct service_client *ipc = context;
	struct rpc_caller_interface *caller = ipc->session->caller;
	psa_status_t status;
	struct psa_ipc_crypto_pack_iovec iov = {
		.function_id = TFM_CRYPTO_KEY_DERIVATION_SETUP_SID,
		.alg = alg,
		.op_handle = *op_handle,
	};
	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&iov), .len = iov_size },
	};
	struct psa_outvec out_vec[] = {
		{ .base = psa_ptr_to_u32(op_handle), .len = sizeof(uint32_t) }
	};

	status = psa_call(caller, TFM_CRYPTO_HANDLE, PSA_IPC_CALL, in_vec,
			  IOVEC_LEN(in_vec), out_vec, IOVEC_LEN(out_vec));

	return status;
}

static inline psa_status_t crypto_caller_key_derivation_get_capacity(
					     struct service_client *context,
					     const uint32_t op_handle,
					     size_t *capacity)
{
	struct service_client *ipc = context;
	struct rpc_caller_interface *caller = ipc->session->caller;
	psa_status_t status;
	struct psa_ipc_crypto_pack_iovec iov = {
		.function_id = TFM_CRYPTO_KEY_DERIVATION_GET_CAPACITY_SID,
		.op_handle = op_handle,
	};
	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&iov), .len = iov_size },
	};
	struct psa_outvec out_vec[] = {
		{ .base = psa_ptr_to_u32(capacity), .len = sizeof(uint32_t) }
	};

	status = psa_call(caller, TFM_CRYPTO_HANDLE, PSA_IPC_CALL, in_vec,
			  IOVEC_LEN(in_vec), out_vec, IOVEC_LEN(out_vec));

	return status;
}

static inline psa_status_t crypto_caller_key_derivation_set_capacity(
					     struct service_client *context,
					     uint32_t op_handle,
					     size_t capacity)
{
	struct service_client *ipc = context;
	struct rpc_caller_interface *caller = ipc->session->caller;
	psa_status_t status;
	struct psa_ipc_crypto_pack_iovec iov = {
		.function_id = TFM_CRYPTO_KEY_DERIVATION_SET_CAPACITY_SID,
		.capacity = capacity,
		.op_handle = op_handle,
	};
	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&iov), .len = iov_size },
	};

	status = psa_call(caller, TFM_CRYPTO_HANDLE, PSA_IPC_CALL, in_vec,
			  IOVEC_LEN(in_vec), NULL, 0);

	return status;
}

static inline psa_status_t crypto_caller_key_derivation_input_bytes(
					    struct service_client *context,
					    uint32_t op_handle,
					    psa_key_derivation_step_t step,
					    const uint8_t *data,
					    size_t data_length)
{
	struct service_client *ipc = context;
	struct rpc_caller_interface *caller = ipc->session->caller;
	psa_status_t status;
	struct psa_ipc_crypto_pack_iovec iov = {
		.function_id = TFM_CRYPTO_KEY_DERIVATION_INPUT_BYTES_SID,
		.step = step,
		.op_handle = op_handle,
	};
	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&iov), .len = iov_size },
		{ .base = psa_ptr_const_to_u32(data), .len = data_length },
	};

	status = psa_call(caller, TFM_CRYPTO_HANDLE, PSA_IPC_CALL, in_vec,
			  IOVEC_LEN(in_vec), NULL, 0);

	return status;
}

static inline psa_status_t crypto_caller_key_derivation_input_key(
					  struct service_client *context,
					  uint32_t op_handle,
					  psa_key_derivation_step_t step,
					  psa_key_id_t key)
{
	struct service_client *ipc = context;
	struct rpc_caller_interface *caller = ipc->session->caller;
	psa_status_t status;
	struct psa_ipc_crypto_pack_iovec iov = {
		.function_id = TFM_CRYPTO_KEY_DERIVATION_INPUT_KEY_SID,
		.key_id = key,
		.step = step,
		.op_handle = op_handle,
	};
	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&iov), .len = iov_size },
	};

	status = psa_call(caller, TFM_CRYPTO_HANDLE, PSA_IPC_CALL, in_vec,
			  IOVEC_LEN(in_vec), NULL, 0);

	return status;
}

static inline psa_status_t crypto_caller_key_derivation_output_bytes(
					     struct service_client *context,
					     uint32_t op_handle,
					     uint8_t *output,
					     size_t output_length)
{
	struct service_client *ipc = context;
	struct rpc_caller_interface *caller = ipc->session->caller;
	psa_status_t status;
	struct psa_ipc_crypto_pack_iovec iov = {
		.function_id = TFM_CRYPTO_KEY_DERIVATION_OUTPUT_BYTES_SID,
		.op_handle = op_handle,
	};
	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&iov), .len = iov_size },
	};
	struct psa_outvec out_vec[] = {
		{ .base = psa_ptr_to_u32(output), .len = output_length },
	};

	status = psa_call(caller, TFM_CRYPTO_HANDLE, PSA_IPC_CALL, in_vec,
			  IOVEC_LEN(in_vec), out_vec, IOVEC_LEN(out_vec));

	return status;
}

static inline psa_status_t crypto_caller_key_derivation_output_key(
				   struct service_client *context,
				   const psa_key_attributes_t *attributes,
				   uint32_t op_handle,
				   psa_key_id_t *key)
{
	struct service_client *ipc = context;
	struct rpc_caller_interface *caller = ipc->session->caller;
	psa_status_t status;
	struct psa_ipc_crypto_pack_iovec iov = {
		.function_id = TFM_CRYPTO_KEY_DERIVATION_OUTPUT_KEY_SID,
		.op_handle = op_handle,
	};
	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&iov), .len = iov_size },
		{ .base = psa_ptr_const_to_u32(attributes),
			.len = sizeof(psa_key_attributes_t) },
	};
	struct psa_outvec out_vec[] = {
		{ .base = psa_ptr_to_u32(key), .len = sizeof(psa_key_id_t)},
	};

	status = psa_call(caller, TFM_CRYPTO_HANDLE, PSA_IPC_CALL, in_vec,
			  IOVEC_LEN(in_vec), out_vec, IOVEC_LEN(out_vec));

	return status;
}

static inline psa_status_t crypto_caller_key_derivation_abort(
					      struct service_client *context,
					      uint32_t *op_handle)
{
	struct service_client *ipc = context;
	struct rpc_caller_interface *caller = ipc->session->caller;
	psa_status_t status;
	struct psa_ipc_crypto_pack_iovec iov = {
		.function_id = TFM_CRYPTO_KEY_DERIVATION_ABORT_SID,
		.op_handle = *op_handle,
	};
	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&iov), .len = iov_size },
	};
	struct psa_outvec out_vec[] = {
		{ .base = psa_ptr_to_u32(op_handle), .len = sizeof(uint32_t) },
	};

	status = psa_call(caller, TFM_CRYPTO_HANDLE, PSA_IPC_CALL, in_vec,
			  IOVEC_LEN(in_vec), out_vec, IOVEC_LEN(out_vec));

	return status;
}

static inline psa_status_t crypto_caller_key_derivation_key_agreement(
				      struct service_client *context,
				      uint32_t op_handle,
				      psa_key_derivation_step_t step,
				      psa_key_id_t private_key,
				      const uint8_t *peer_key,
				      size_t peer_key_length)
{
	struct service_client *ipc = context;
	struct rpc_caller_interface *caller = ipc->session->caller;
	psa_status_t status;
	struct psa_ipc_crypto_pack_iovec iov = {
		.function_id = TFM_CRYPTO_KEY_DERIVATION_KEY_AGREEMENT_SID,
		.key_id = private_key,
		.step = step,
		.op_handle = op_handle,
	};
	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&iov), .len = iov_size },
		{ .base = psa_ptr_const_to_u32(peer_key),
			.len = peer_key_length},
	};

	status = psa_call(caller, TFM_CRYPTO_HANDLE, PSA_IPC_CALL, in_vec,
			  IOVEC_LEN(in_vec), NULL, 0);

	return status;
}

static inline psa_status_t crypto_caller_raw_key_agreement(
					   struct service_client *context,
					   psa_algorithm_t alg,
					   psa_key_id_t private_key,
					   const uint8_t *peer_key,
					   size_t peer_key_length,
					   uint8_t *output,
					   size_t output_size,
					   size_t *output_length)
{
	struct service_client *ipc = context;
	struct rpc_caller_interface *caller = ipc->session->caller;
	psa_status_t status;
	struct psa_ipc_crypto_pack_iovec iov = {
		.function_id = TFM_CRYPTO_RAW_KEY_AGREEMENT_SID,
		.alg = alg,
		.key_id = private_key,
	};
	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&iov), .len = iov_size },
		{ .base = psa_ptr_const_to_u32(peer_key),
			.len = peer_key_length},
	};
	struct psa_outvec out_vec[] = {
		{ .base = psa_ptr_to_u32(output), .len = output_size },
	};

	status = psa_call(caller, TFM_CRYPTO_HANDLE, PSA_IPC_CALL, in_vec,
			  IOVEC_LEN(in_vec), out_vec, IOVEC_LEN(out_vec));

	*output_length = out_vec[0].len;

	return status;
}

#ifdef __cplusplus
}
#endif

#endif /* PSA_IPC_CRYPTO_CALLER_KEY_DERIVATION_H */
