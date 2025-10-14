/*
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef STUB_CRYPTO_CALLER_KEY_DERIVATION_H
#define STUB_CRYPTO_CALLER_KEY_DERIVATION_H

#include <psa/crypto.h>
#include <service/common/client/service_client.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline psa_status_t crypto_caller_key_derivation_setup(struct service_client *context,
	uint32_t *op_handle,
	psa_algorithm_t alg)
{
	(void)context;
	(void)op_handle;
	(void)alg;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_key_derivation_get_capacity(struct service_client *context,
	const uint32_t op_handle,
	size_t *capacity)
{
	(void)context;
	(void)op_handle;
	(void)capacity;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_key_derivation_set_capacity(struct service_client *context,
	uint32_t op_handle,
	size_t capacity)
{
	(void)context;
	(void)op_handle;
	(void)capacity;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_key_derivation_input_bytes(struct service_client *context,
	uint32_t op_handle,
	psa_key_derivation_step_t step,
	const uint8_t *data,
	size_t data_length)
{
	(void)context;
	(void)op_handle;
	(void)step;
	(void)data;
	(void)data_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_key_derivation_input_key(struct service_client *context,
	uint32_t op_handle,
	psa_key_derivation_step_t step,
	psa_key_id_t key)
{
	(void)context;
	(void)op_handle;
	(void)step;
	(void)key;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_key_derivation_output_bytes(struct service_client *context,
	uint32_t op_handle,
	uint8_t *output,
	size_t output_length)
{
	(void)context;
	(void)op_handle;
	(void)output;
	(void)output_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_key_derivation_output_key(struct service_client *context,
	const psa_key_attributes_t *attributes,
	uint32_t op_handle,
	psa_key_id_t *key)
{
	(void)context;
	(void)attributes;
	(void)op_handle;
	(void)key;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_key_derivation_abort(struct service_client *context,
	uint32_t *op_handle)
{
	(void)context;
	(void)op_handle;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_key_derivation_key_agreement(struct service_client *context,
	uint32_t op_handle,
	psa_key_derivation_step_t step,
	psa_key_id_t private_key,
	const uint8_t *peer_key,
	size_t peer_key_length)
{
	(void)context;
	(void)op_handle;
	(void)step;
	(void)private_key;
	(void)peer_key;
	(void)peer_key_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_raw_key_agreement(struct service_client *context,
	psa_algorithm_t alg,
	psa_key_id_t private_key,
	const uint8_t *peer_key,
	size_t peer_key_length,
	uint8_t *output,
	size_t output_size,
	size_t *output_length)
{
	(void)context;
	(void)alg;
	(void)private_key;
	(void)peer_key;
	(void)peer_key_length;
	(void)output;
	(void)output_size;
	(void)output_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

#ifdef __cplusplus
}
#endif

#endif /* STUB_CRYPTO_CALLER_KEY_DERIVATION_H */
