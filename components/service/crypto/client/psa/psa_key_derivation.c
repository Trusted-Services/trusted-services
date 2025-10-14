/*
 * Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <psa/crypto.h>
#include "psa_crypto_client.h"
#include "crypto_caller_selector.h"

psa_status_t psa_key_derivation_setup(
	psa_key_derivation_operation_t *operation,
	psa_algorithm_t alg)
{
	if (psa_crypto_client_instance.init_status != PSA_SUCCESS)
		return psa_crypto_client_instance.init_status;

	return crypto_caller_key_derivation_setup(&psa_crypto_client_instance.base,
		&operation->handle, alg);
}

psa_status_t psa_key_derivation_get_capacity(
	const psa_key_derivation_operation_t *operation,
	size_t *capacity)
{
	return crypto_caller_key_derivation_get_capacity(&psa_crypto_client_instance.base,
		operation->handle, capacity);
}

psa_status_t psa_key_derivation_set_capacity(
	psa_key_derivation_operation_t *operation,
	size_t capacity)
{
	return crypto_caller_key_derivation_set_capacity(&psa_crypto_client_instance.base,
		operation->handle, capacity);
}

psa_status_t psa_key_derivation_input_bytes(
	psa_key_derivation_operation_t *operation,
	psa_key_derivation_step_t step,
	const uint8_t *data,
	size_t data_length)
{
	return crypto_caller_key_derivation_input_bytes(&psa_crypto_client_instance.base,
		operation->handle, step,
		data, data_length);
}

psa_status_t psa_key_derivation_input_key(
	psa_key_derivation_operation_t *operation,
	psa_key_derivation_step_t step,
	psa_key_id_t key)
{
	return crypto_caller_key_derivation_input_key(&psa_crypto_client_instance.base,
		operation->handle, step, key);
}

psa_status_t psa_key_derivation_output_bytes(
	psa_key_derivation_operation_t *operation,
	uint8_t *output,
	size_t output_length)
{
	return crypto_caller_key_derivation_output_bytes(&psa_crypto_client_instance.base,
		operation->handle,
		output, output_length);
}

psa_status_t psa_key_derivation_output_key(
	const psa_key_attributes_t *attributes,
	psa_key_derivation_operation_t *operation,
	psa_key_id_t *key)
{
	return crypto_caller_key_derivation_output_key(&psa_crypto_client_instance.base,
		attributes, operation->handle,
		key);
}

psa_status_t psa_key_derivation_abort(
	psa_key_derivation_operation_t *operation)
{
	return crypto_caller_key_derivation_abort(&psa_crypto_client_instance.base,
		&operation->handle);
}

psa_status_t psa_key_derivation_key_agreement(
	psa_key_derivation_operation_t *operation,
	psa_key_derivation_step_t step,
	psa_key_id_t private_key,
	const uint8_t *peer_key,
	size_t peer_key_length)
{
	return crypto_caller_key_derivation_key_agreement(&psa_crypto_client_instance.base,
		operation->handle, step,
		private_key, peer_key, peer_key_length);
}

psa_status_t psa_raw_key_agreement(psa_algorithm_t alg,
	psa_key_id_t private_key,
	const uint8_t *peer_key,
	size_t peer_key_length,
	uint8_t *output,
	size_t output_size,
	size_t *output_length)
{
	return crypto_caller_raw_key_agreement(&psa_crypto_client_instance.base,
		alg,
		private_key, peer_key, peer_key_length,
		output, output_size, output_length);
}
