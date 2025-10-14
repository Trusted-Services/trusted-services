/*
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <psa/crypto.h>
#include "psa_crypto_client.h"
#include "crypto_caller_selector.h"

psa_status_t psa_cipher_encrypt_setup(psa_cipher_operation_t *operation,
	psa_key_id_t key,
	psa_algorithm_t alg)
{
	if (psa_crypto_client_instance.init_status != PSA_SUCCESS)
		return psa_crypto_client_instance.init_status;

	if (operation->handle)
		return PSA_ERROR_BAD_STATE;

	return crypto_caller_cipher_encrypt_setup(&psa_crypto_client_instance.base,
		&operation->handle,
		key, alg);
}

psa_status_t psa_cipher_decrypt_setup(psa_cipher_operation_t *operation,
	psa_key_id_t key,
	psa_algorithm_t alg)
{
	if (psa_crypto_client_instance.init_status != PSA_SUCCESS)
		return psa_crypto_client_instance.init_status;

	if (operation->handle)
		return PSA_ERROR_BAD_STATE;

	return crypto_caller_cipher_decrypt_setup(&psa_crypto_client_instance.base,
		&operation->handle,
		key, alg);
}

psa_status_t psa_cipher_generate_iv(psa_cipher_operation_t *operation,
	uint8_t *iv,
	size_t iv_size,
	size_t *iv_length)
{
	return crypto_caller_cipher_generate_iv(&psa_crypto_client_instance.base,
		operation->handle,
		iv, iv_size, iv_length);
}

psa_status_t psa_cipher_set_iv(psa_cipher_operation_t *operation,
	const uint8_t *iv,
	size_t iv_length)
{
	return crypto_caller_cipher_set_iv(&psa_crypto_client_instance.base,
		operation->handle,
		iv, iv_length);
}

psa_status_t psa_cipher_update(psa_cipher_operation_t *operation,
	const uint8_t *input,
	size_t input_length,
	uint8_t *output,
	size_t output_size,
	size_t *output_length)
{
	return crypto_caller_cipher_update(&psa_crypto_client_instance.base,
		operation->handle,
		input, input_length,
		output, output_size, output_length);
}

psa_status_t psa_cipher_finish(psa_cipher_operation_t *operation,
	uint8_t *output,
	size_t output_size,
	size_t *output_length)
{
	return crypto_caller_cipher_finish(&psa_crypto_client_instance.base,
		&operation->handle,
		output, output_size, output_length);
}

psa_status_t psa_cipher_abort(psa_cipher_operation_t *operation)
{
	return crypto_caller_cipher_abort(&psa_crypto_client_instance.base,
		&operation->handle);
}

static psa_status_t multi_cipher_update(psa_cipher_operation_t *operation,
	const uint8_t *input,
	size_t input_length,
	uint8_t *output,
	size_t output_size,
	size_t *output_length)
{
	psa_status_t psa_status = PSA_SUCCESS;
	size_t max_update_size =
		crypto_caller_cipher_max_update_size(&psa_crypto_client_instance.base);
	size_t bytes_input = 0;
	size_t bytes_output = 0;

	*output_length = 0;

	if (!max_update_size) {

		/* Don't know the max update size so assume that the entire
		 * input and output can be handled in a single update.  If
		 * this isn't true, the first cipher update operation will fail
		 * safely.
		 */
		max_update_size = input_length;
	}

	while (bytes_input < input_length) {

		if (bytes_output >= output_size) {

			psa_status = PSA_ERROR_BUFFER_TOO_SMALL;
			break;
		}

		size_t update_output_len = 0;
		size_t bytes_remaining = input_length - bytes_input;
		size_t update_len = (bytes_remaining < max_update_size) ?
			bytes_remaining :
			max_update_size;

		psa_status = psa_cipher_update(operation,
			&input[bytes_input], update_len,
			&output[bytes_output], output_size - bytes_output, &update_output_len);

		if (psa_status != PSA_SUCCESS)
			break;

		bytes_input += update_len;
		bytes_output += update_output_len;
	}

	if (psa_status == PSA_SUCCESS) {

		if (bytes_output < output_size) {

			size_t finish_output_len = 0;

			psa_status = psa_cipher_finish(operation,
				&output[bytes_output], output_size - bytes_output, &finish_output_len);

			if (psa_status == PSA_SUCCESS) {

				*output_length = bytes_output + finish_output_len;
			}
			else {

				psa_cipher_abort(operation);
			}
		}
		else {

			psa_status = PSA_ERROR_BUFFER_TOO_SMALL;
		}
	}

	return psa_status;
}

psa_status_t psa_cipher_encrypt(psa_key_id_t key,
	psa_algorithm_t alg,
	const uint8_t *input,
	size_t input_length,
	uint8_t *output,
	size_t output_size,
	size_t *output_length)
{
	psa_cipher_operation_t operation = psa_cipher_operation_init();
	psa_status_t psa_status = psa_cipher_encrypt_setup(&operation, key, alg);

	if (psa_status == PSA_SUCCESS) {

		size_t ciphertext_len = 0;
		size_t iv_len = 0;

		psa_status = psa_cipher_generate_iv(&operation, output, output_size, &iv_len);

		if (psa_status == PSA_SUCCESS) {

			if (iv_len <= output_size) {

				psa_status = multi_cipher_update(&operation,
					input, input_length,
					&output[iv_len], output_size - iv_len, &ciphertext_len);

				*output_length = iv_len + ciphertext_len;
			}
			else {

				psa_status = PSA_ERROR_BUFFER_TOO_SMALL;
			}
		}

		if (psa_status != PSA_SUCCESS) {

			psa_cipher_abort(&operation);
		}
	}

	return psa_status;
}

psa_status_t psa_cipher_decrypt(psa_key_id_t key,
	psa_algorithm_t alg,
	const uint8_t *input,
	size_t input_length,
	uint8_t *output,
	size_t output_size,
	size_t *output_length)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t psa_status = psa_get_key_attributes(key, &attributes);

	if (psa_status == PSA_SUCCESS) {

		psa_cipher_operation_t operation = psa_cipher_operation_init();
		psa_status = psa_cipher_decrypt_setup(&operation, key, alg);

		if (psa_status == PSA_SUCCESS) {

			size_t iv_len = PSA_CIPHER_IV_LENGTH(psa_get_key_type(&attributes), alg);

			if (input_length >= iv_len) {

				psa_status = psa_cipher_set_iv(&operation, input, iv_len);

				if (psa_status == PSA_SUCCESS) {

					psa_status = multi_cipher_update(&operation,
						&input[iv_len], input_length - iv_len,
						output, output_size, output_length);
				}
			}
			else {

				psa_status = PSA_ERROR_INVALID_ARGUMENT;
			}

			if (psa_status != PSA_SUCCESS) {

				psa_cipher_abort(&operation);
			}
		}

		psa_reset_key_attributes(&attributes);
	}

	return psa_status;
}
