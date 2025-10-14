/*
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <psa/crypto.h>
#include "psa_crypto_client.h"
#include "crypto_caller_selector.h"


psa_status_t psa_mac_sign_setup(psa_mac_operation_t *operation,
	psa_key_id_t key,
	psa_algorithm_t alg)
{
	if (psa_crypto_client_instance.init_status != PSA_SUCCESS)
		return psa_crypto_client_instance.init_status;

	if (operation->handle)
		return PSA_ERROR_BAD_STATE;

	return crypto_caller_mac_sign_setup(&psa_crypto_client_instance.base,
		&operation->handle,
		key, alg);
}

psa_status_t psa_mac_verify_setup(psa_mac_operation_t *operation,
	psa_key_id_t key,
	psa_algorithm_t alg)
{
	if (psa_crypto_client_instance.init_status != PSA_SUCCESS)
		return psa_crypto_client_instance.init_status;

	if (operation->handle)
		return PSA_ERROR_BAD_STATE;

	return crypto_caller_mac_verify_setup(&psa_crypto_client_instance.base,
		&operation->handle,
		key, alg);
}

psa_status_t psa_mac_update(psa_mac_operation_t *operation,
	const uint8_t *input,
	size_t input_length)
{
	return crypto_caller_mac_update(&psa_crypto_client_instance.base,
		operation->handle,
		input, input_length);
}

psa_status_t psa_mac_sign_finish(psa_mac_operation_t *operation,
	uint8_t *mac,
	size_t mac_size,
	size_t *mac_length)
{
	return crypto_caller_mac_sign_finish(&psa_crypto_client_instance.base,
		&operation->handle,
		mac, mac_size, mac_length);
}

psa_status_t psa_mac_verify_finish(psa_mac_operation_t *operation,
	const uint8_t *mac,
	size_t mac_length)
{
	return crypto_caller_mac_verify_finish(&psa_crypto_client_instance.base,
		&operation->handle,
		mac, mac_length);
}

psa_status_t psa_mac_abort(psa_mac_operation_t *operation)
{
	return crypto_caller_mac_abort(&psa_crypto_client_instance.base,
		&operation->handle);
}

static psa_status_t multi_mac_update(psa_mac_operation_t *operation,
	const uint8_t *input,
	size_t input_length)
{
	psa_status_t psa_status = PSA_SUCCESS;
	size_t max_update_size = crypto_caller_mac_max_update_size(&psa_crypto_client_instance.base);
	size_t bytes_processed = 0;

	if (!max_update_size) {

		/* Don't know the max update size so assume that the entire
		 * input can be handled in a single update.  If this isn't
		 * true, the first mac update operation will fail safely.
		 */
		max_update_size = input_length;
	}

	while (bytes_processed < input_length) {

		size_t bytes_remaining = input_length - bytes_processed;
		size_t update_len = (bytes_remaining < max_update_size) ?
			bytes_remaining :
			max_update_size;

		psa_status = psa_mac_update(operation, &input[bytes_processed], update_len);

		if (psa_status != PSA_SUCCESS) {

			psa_mac_abort(operation);
			break;
		}

		bytes_processed += update_len;
	}

	return psa_status;
}

psa_status_t psa_mac_verify(psa_key_id_t key,
	psa_algorithm_t alg,
	const uint8_t *input,
	size_t input_length,
	const uint8_t *mac,
	size_t mac_length)
{
	psa_mac_operation_t operation = psa_mac_operation_init();
	psa_status_t psa_status = psa_mac_verify_setup(&operation, key, alg);

	if (psa_status == PSA_SUCCESS) {

		psa_status = multi_mac_update(&operation, input, input_length);
	}

	if (psa_status == PSA_SUCCESS) {

		psa_status = psa_mac_verify_finish(&operation, mac, mac_length);

		if (psa_status != PSA_SUCCESS) {

			psa_mac_abort(&operation);
		}
	}

	return psa_status;
}

psa_status_t psa_mac_compute(psa_key_id_t key,
	psa_algorithm_t alg,
	const uint8_t *input,
	size_t input_length,
	uint8_t *mac,
	size_t mac_size,
	size_t *mac_length)
{
	psa_mac_operation_t operation = psa_mac_operation_init();
	psa_status_t psa_status = psa_mac_sign_setup(&operation, key, alg);

	if (psa_status == PSA_SUCCESS) {

		psa_status = multi_mac_update(&operation, input, input_length);
	}

	if (psa_status == PSA_SUCCESS) {

		psa_status = psa_mac_sign_finish(&operation, mac, mac_size, mac_length);

		if (psa_status != PSA_SUCCESS) {

			psa_mac_abort(&operation);
		}
	}

	return psa_status;
}
