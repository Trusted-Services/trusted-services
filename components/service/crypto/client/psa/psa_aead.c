/*
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <psa/crypto.h>
#include "psa_crypto_client.h"
#include "crypto_caller_selector.h"
#include <string.h>

psa_status_t psa_aead_encrypt_setup(psa_aead_operation_t *operation,
	psa_key_id_t key,
	psa_algorithm_t alg)
{
	if (psa_crypto_client_instance.init_status != PSA_SUCCESS)
		return psa_crypto_client_instance.init_status;

	return crypto_caller_aead_encrypt_setup(&psa_crypto_client_instance.base,
		&operation->handle, key, alg);
}

psa_status_t psa_aead_decrypt_setup(psa_aead_operation_t *operation,
	psa_key_id_t key,
	psa_algorithm_t alg)
{
	if (psa_crypto_client_instance.init_status != PSA_SUCCESS)
		return psa_crypto_client_instance.init_status;

	return crypto_caller_aead_decrypt_setup(&psa_crypto_client_instance.base,
		&operation->handle, key, alg);
}

psa_status_t psa_aead_generate_nonce(psa_aead_operation_t *operation,
	uint8_t *nonce,
	size_t nonce_size,
	size_t *nonce_length)
{
	return crypto_caller_aead_generate_nonce(&psa_crypto_client_instance.base,
		operation->handle,
		nonce, nonce_size, nonce_length);
}

psa_status_t psa_aead_set_nonce(psa_aead_operation_t *operation,
	const uint8_t *nonce,
	size_t nonce_length)
{
	return crypto_caller_aead_set_nonce(&psa_crypto_client_instance.base,
		operation->handle,
		nonce, nonce_length);
}

psa_status_t psa_aead_set_lengths(psa_aead_operation_t *operation,
	size_t ad_length,
	size_t plaintext_length)
{
	return crypto_caller_aead_set_lengths(&psa_crypto_client_instance.base,
		operation->handle,
		ad_length, plaintext_length);
}

psa_status_t psa_aead_update_ad(psa_aead_operation_t *operation,
	const uint8_t *input,
	size_t input_length)
{
	return crypto_caller_aead_update_ad(&psa_crypto_client_instance.base,
		operation->handle,
		input, input_length);
}

psa_status_t psa_aead_update(psa_aead_operation_t *operation,
	const uint8_t *input,
	size_t input_length,
	uint8_t *output,
	size_t output_size,
	size_t *output_length)
{
	psa_status_t status = crypto_caller_aead_update(&psa_crypto_client_instance.base,
		operation->handle,
		input, input_length,
		output, output_size, output_length);

	/*
	 * If too small a buffer has been provided for the output, the operation
	 * state will have been updated but the result can't be put anywhere. This
	 * is an unrecoveral condition so abort the operation.
	 */
	if (status == PSA_ERROR_BUFFER_TOO_SMALL) {

		psa_aead_abort(operation);
	}

	return status;
}

psa_status_t psa_aead_finish(psa_aead_operation_t *operation,
	uint8_t *aeadtext,
	size_t aeadtext_size,
	size_t *aeadtext_length,
	uint8_t *tag,
	size_t tag_size,
	size_t *tag_length)
{
	return crypto_caller_aead_finish(&psa_crypto_client_instance.base,
		operation->handle,
		aeadtext, aeadtext_size, aeadtext_length,
		tag, tag_size, tag_length);
}

psa_status_t psa_aead_verify(psa_aead_operation_t *operation,
	uint8_t *plaintext,
	size_t plaintext_size,
	size_t *plaintext_length,
	const uint8_t *tag,
	size_t tag_length)
{
	return crypto_caller_aead_verify(&psa_crypto_client_instance.base,
		operation->handle,
		plaintext, plaintext_size, plaintext_length,
		tag, tag_length);
}

psa_status_t psa_aead_abort(psa_aead_operation_t *operation)
{
	return crypto_caller_aead_abort(&psa_crypto_client_instance.base,
		operation->handle);
}

static psa_status_t multi_aead_update_ad(psa_aead_operation_t *operation,
	const uint8_t *input,
	size_t input_length)
{
	psa_status_t psa_status = PSA_SUCCESS;
	size_t max_update_size =
		crypto_caller_aead_max_update_ad_size(&psa_crypto_client_instance.base);
	size_t bytes_input = 0;

	if (!max_update_size) {

		/* Don't know the max update size so assume that the entire
		 * input and output can be handled in a single update.  If
		 * this isn't true, the first aead update operation will fail
		 * safely.
		 */
		max_update_size = input_length;
	}

	while (bytes_input < input_length) {

		size_t bytes_remaining = input_length - bytes_input;
		size_t update_len = (bytes_remaining < max_update_size) ?
			bytes_remaining :
			max_update_size;

		psa_status = psa_aead_update_ad(operation,
			&input[bytes_input], update_len);

		if (psa_status != PSA_SUCCESS) break;

		bytes_input += update_len;
	}

	return psa_status;
}

static psa_status_t multi_aead_update(psa_aead_operation_t *operation,
	const uint8_t *input,
	size_t input_length,
	uint8_t *output,
	size_t output_size,
	size_t *output_length)
{
	psa_status_t psa_status = PSA_SUCCESS;
	size_t max_update_size =
		crypto_caller_aead_max_update_size(&psa_crypto_client_instance.base);
	size_t bytes_input = 0;
	size_t bytes_output = 0;

	*output_length = 0;

	if (!max_update_size) {

		/* Don't know the max update size so assume that the entire
		 * input and output can be handled in a single update.  If
		 * this isn't true, the first aead update operation will fail
		 * safely.
		 */
		max_update_size = input_length;
	}

	while ((bytes_input < input_length) && (bytes_output < output_size)) {

		size_t update_output_len = 0;
		size_t bytes_remaining = input_length - bytes_input;
		size_t update_len = (bytes_remaining < max_update_size) ?
			bytes_remaining :
			max_update_size;

		psa_status = psa_aead_update(operation,
			&input[bytes_input], update_len,
			&output[bytes_output], output_size - bytes_output, &update_output_len);

		if (psa_status != PSA_SUCCESS) break;

		bytes_input += update_len;
		bytes_output += update_output_len;
	}

	if (psa_status == PSA_SUCCESS) {

		*output_length = bytes_output;
	}

	return psa_status;
}

psa_status_t psa_aead_encrypt(psa_key_id_t key,
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
	psa_aead_operation_t operation = psa_aead_operation_init();
	size_t bytes_output = 0;
	*aeadtext_length = 0;

	psa_status_t psa_status = psa_aead_encrypt_setup(&operation, key, alg);
	if (psa_status != PSA_SUCCESS) return psa_status;

	if ((psa_status = psa_aead_set_lengths(&operation, additional_data_length, plaintext_length),
			psa_status == PSA_SUCCESS) &&
		(psa_status = psa_aead_set_nonce(&operation, nonce, nonce_length),
			psa_status == PSA_SUCCESS) &&
		(psa_status = multi_aead_update_ad(&operation, additional_data, additional_data_length),
			psa_status == PSA_SUCCESS) &&
		(psa_status = multi_aead_update(&operation, plaintext, plaintext_length,
			aeadtext, aeadtext_size, &bytes_output),
			psa_status == PSA_SUCCESS))
	{
		size_t remaining_aead_len = 0;
		size_t tag_len = 0;
		uint8_t tag[PSA_AEAD_TAG_MAX_SIZE];

		psa_status = psa_aead_finish(&operation,
			&aeadtext[bytes_output], aeadtext_size - bytes_output, &remaining_aead_len,
			tag, PSA_AEAD_TAG_MAX_SIZE, &tag_len);

		if (aeadtext_size < bytes_output + remaining_aead_len + tag_len)
			psa_status = PSA_ERROR_BUFFER_TOO_SMALL;

		if (psa_status == PSA_SUCCESS) {
			bytes_output += remaining_aead_len;
			memcpy(&aeadtext[bytes_output], tag, tag_len);
			*aeadtext_length = bytes_output + tag_len;
		}
		else {

			psa_aead_abort(&operation);
		}
	}
	else {

		psa_aead_abort(&operation);
	}

	return psa_status;
}

psa_status_t psa_aead_decrypt(psa_key_id_t key,
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
	psa_aead_operation_t operation = psa_aead_operation_init();
	size_t bytes_output = 0;
	*plaintext_length = 0;

	psa_status_t psa_status = psa_aead_decrypt_setup(&operation, key, alg);
	if (psa_status != PSA_SUCCESS) return psa_status;

	size_t tag_len = PSA_ALG_AEAD_GET_TAG_LENGTH(alg);
	size_t ciphertext_len = (aeadtext_length > tag_len) ? aeadtext_length - tag_len : 0;

	if ((psa_status = psa_aead_set_lengths(&operation, additional_data_length, ciphertext_len),
			psa_status == PSA_SUCCESS) &&
		(psa_status = psa_aead_set_nonce(&operation, nonce, nonce_length),
			psa_status == PSA_SUCCESS) &&
		(psa_status = multi_aead_update_ad(&operation, additional_data, additional_data_length),
			psa_status == PSA_SUCCESS) &&
		(psa_status = multi_aead_update(&operation, aeadtext, ciphertext_len,
			plaintext, plaintext_size, &bytes_output),
			psa_status == PSA_SUCCESS))
	{
		size_t remaining_plaintext_len = 0;

		psa_status = psa_aead_verify(&operation,
			&plaintext[bytes_output], plaintext_size - bytes_output,
			&remaining_plaintext_len, &aeadtext[ciphertext_len], tag_len);

		if (psa_status == PSA_SUCCESS) {

			*plaintext_length = bytes_output + remaining_plaintext_len;
		}
		else {

			psa_aead_abort(&operation);
		}
	}
	else {

		psa_aead_abort(&operation);
	}

	return psa_status;
}
