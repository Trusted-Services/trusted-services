/*
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef STUB_CRYPTO_CALLER_HASH_H
#define STUB_CRYPTO_CALLER_HASH_H

#include <psa/crypto.h>
#include <service/common/client/service_client.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline psa_status_t crypto_caller_hash_setup(struct service_client *context,
	uint32_t *op_handle,
	psa_algorithm_t alg)
{
	(void)context;
	(void)op_handle;
	(void)alg;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_hash_update(struct service_client *context,
	uint32_t op_handle,
	const uint8_t *input,
	size_t input_length)
{
	(void)context;
	(void)op_handle;
	(void)input;
	(void)input_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_hash_finish(struct service_client *context,
	uint32_t *op_handle,
	uint8_t *hash,
	size_t hash_size,
	size_t *hash_length)
{
	(void)context;
	(void)op_handle;
	(void)hash;
	(void)hash_size;
	(void)hash_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_hash_abort(struct service_client *context,
	uint32_t *op_handle)
{
	(void)context;
	(void)op_handle;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_hash_verify(struct service_client *context,
	uint32_t *op_handle,
	const uint8_t *hash,
	size_t hash_length)
{
	(void)context;
	(void)op_handle;
	(void)hash;
	(void)hash_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_hash_clone(struct service_client *context,
	uint32_t source_op_handle,
	uint32_t *target_op_handle)
{
	(void)context;
	(void)source_op_handle;
	(void)target_op_handle;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline size_t crypto_caller_hash_max_update_size(struct service_client *context)
{
	(void)context;

	return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* STUB_CRYPTO_CALLER_HASH_H */
