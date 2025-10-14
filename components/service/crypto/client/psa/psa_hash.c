/*
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <psa/crypto.h>
#include "psa_crypto_client.h"
#include "crypto_caller_selector.h"

psa_status_t psa_hash_setup(psa_hash_operation_t *operation,
	psa_algorithm_t alg)
{
	if (psa_crypto_client_instance.init_status != PSA_SUCCESS)
		return psa_crypto_client_instance.init_status;

	if (operation->handle)
		return PSA_ERROR_BAD_STATE;

	return crypto_caller_hash_setup(&psa_crypto_client_instance.base,
		&operation->handle, alg);
}

psa_status_t psa_hash_update(psa_hash_operation_t *operation,
	const uint8_t *input,
	size_t input_length)
{
	return crypto_caller_hash_update(&psa_crypto_client_instance.base,
		operation->handle,
		input, input_length);
}

psa_status_t psa_hash_finish(psa_hash_operation_t *operation,
	uint8_t *hash,
	size_t hash_size,
	size_t *hash_length)
{
	return crypto_caller_hash_finish(&psa_crypto_client_instance.base,
		&operation->handle,
		hash, hash_size, hash_length);
}

psa_status_t psa_hash_abort(psa_hash_operation_t *operation)
{
	return crypto_caller_hash_abort(&psa_crypto_client_instance.base,
		&operation->handle);
}

psa_status_t psa_hash_verify(psa_hash_operation_t *operation,
	const uint8_t *hash,
	size_t hash_length)
{
	return crypto_caller_hash_verify(&psa_crypto_client_instance.base,
		&operation->handle,
		hash, hash_length);
}

psa_status_t psa_hash_clone(const psa_hash_operation_t *source_operation,
	psa_hash_operation_t *target_operation)
{
	if (target_operation->handle)
		return PSA_ERROR_BAD_STATE;

	return crypto_caller_hash_clone(&psa_crypto_client_instance.base,
		source_operation->handle,
		&target_operation->handle);
}

psa_status_t psa_hash_suspend(psa_hash_operation_t *operation,
	uint8_t *hash_state,
	size_t hash_state_size,
	size_t *hash_state_length)
{
	(void)operation;
	(void)hash_state;
	(void)hash_state_size;
	(void)hash_state_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t psa_hash_resume(psa_hash_operation_t *operation,
	const uint8_t *hash_state,
	size_t hash_state_length)
{
	(void)operation;
	(void)hash_state;
	(void)hash_state_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

static psa_status_t multi_hash_update(psa_hash_operation_t *operation,
	psa_algorithm_t alg,
	const uint8_t *input,
	size_t input_length)
{
	*operation = psa_hash_operation_init();
	psa_status_t psa_status = psa_hash_setup(operation, alg);
	size_t max_update_size = crypto_caller_hash_max_update_size(&psa_crypto_client_instance.base);

	if (!max_update_size) {

		/* Don't know the max update size so assume that the entire
		 * input can be handled in a single update.  If this isn't
		 * true, the first hash update operation will fail safely.
		 */
		max_update_size = input_length;
	}

	if (psa_status == PSA_SUCCESS) {

		size_t bytes_processed = 0;

		while (bytes_processed < input_length) {

			size_t bytes_remaining = input_length - bytes_processed;
			size_t update_len = (bytes_remaining < max_update_size) ?
				bytes_remaining :
				max_update_size;

			psa_status = psa_hash_update(operation, &input[bytes_processed], update_len);

			if (psa_status != PSA_SUCCESS) {

				psa_hash_abort(operation);
				break;
			}

			bytes_processed += update_len;
		}
	}

	return psa_status;
}

psa_status_t psa_hash_compare(psa_algorithm_t alg,
	const uint8_t *input,
	size_t input_length,
	const uint8_t *hash,
	size_t hash_length)
{
	psa_hash_operation_t operation;
	psa_status_t psa_status = multi_hash_update(&operation, alg, input, input_length);

	if (psa_status == PSA_SUCCESS) {

		psa_status = psa_hash_verify(&operation, hash, hash_length);

		if (psa_status != PSA_SUCCESS) {

			psa_hash_abort(&operation);
		}
	}

	return psa_status;
}

psa_status_t psa_hash_compute(psa_algorithm_t alg,
	const uint8_t *input,
	size_t input_length,
	uint8_t *hash,
	size_t hash_size,
	size_t *hash_length)
{
	psa_hash_operation_t operation;
	psa_status_t psa_status = multi_hash_update(&operation, alg, input, input_length);

	if (psa_status == PSA_SUCCESS) {

		psa_status = psa_hash_finish(&operation, hash, hash_size, hash_length);

		if (psa_status != PSA_SUCCESS) {

			psa_hash_abort(&operation);
		}
	}

	return psa_status;
}
