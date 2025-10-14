/*
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef STUB_CRYPTO_CALLER_AEAD_H
#define STUB_CRYPTO_CALLER_AEAD_H

#include <psa/crypto.h>
#include <service/common/client/service_client.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline psa_status_t crypto_caller_aead_encrypt(struct service_client *context,
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
	(void)context;
	(void)key;
	(void)alg;
	(void)nonce;
	(void)nonce_length;
	(void)additional_data;
	(void)additional_data_length;
	(void)plaintext;
	(void)plaintext_length;
	(void)aeadtext;
	(void)aeadtext_size;
	(void)aeadtext_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_aead_decrypt(struct service_client *context,
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
	(void)context;
	(void)key;
	(void)alg;
	(void)nonce;
	(void)nonce_length;
	(void)additional_data;
	(void)additional_data_length;
	(void)aeadtext;
	(void)aeadtext_length;
	(void)plaintext;
	(void)plaintext_size;
	(void)plaintext_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_aead_encrypt_setup(struct service_client *context,
	uint32_t *op_handle,
	psa_key_id_t key,
	psa_algorithm_t alg)
{
	(void)context;
	(void)op_handle;
	(void)key;
	(void)alg;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_aead_decrypt_setup(struct service_client *context,
	uint32_t *op_handle,
	psa_key_id_t key,
	psa_algorithm_t alg)
{
	(void)context;
	(void)op_handle;
	(void)key;
	(void)alg;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_aead_generate_nonce(struct service_client *context,
	uint32_t op_handle,
	uint8_t *nonce,
	size_t nonce_size,
	size_t *nonce_length)
{
	(void)context;
	(void)op_handle;
	(void)nonce;
	(void)nonce_size;
	(void)nonce_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_aead_set_nonce(struct service_client *context,
	uint32_t op_handle,
	const uint8_t *nonce,
	size_t nonce_length)
{
	(void)context;
	(void)op_handle;
	(void)nonce;
	(void)nonce_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_aead_set_lengths(struct service_client *context,
	uint32_t op_handle,
	size_t ad_length,
	size_t plaintext_length)
{
	(void)context;
	(void)op_handle;
	(void)ad_length;
	(void)plaintext_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_aead_update_ad(struct service_client *context,
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

static inline psa_status_t crypto_caller_aead_update(struct service_client *context,
	uint32_t op_handle,
	const uint8_t *input,
	size_t input_length,
	uint8_t *output,
	size_t output_size,
	size_t *output_length)
{
	(void)context;
	(void)op_handle;
	(void)input;
	(void)input_length;
	(void)output;
	(void)output_size;
	(void)output_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_aead_finish(struct service_client *context,
	uint32_t *op_handle,
	uint8_t *aeadtext,
	size_t aeadtext_size,
	size_t *aeadtext_length,
	uint8_t *tag,
	size_t tag_size,
	size_t *tag_length)
{
	(void)context;
	(void)op_handle;
	(void)aeadtext;
	(void)aeadtext_size;
	(void)aeadtext_length;
	(void)tag;
	(void)tag_size;
	(void)tag_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_aead_verify(struct service_client *context,
	uint32_t *op_handle,
	uint8_t *plaintext,
	size_t plaintext_size,
	size_t *plaintext_length,
	const uint8_t *tag,
	size_t tag_length)
{
	(void)context;
	(void)op_handle;
	(void)plaintext;
	(void)plaintext_size;
	(void)plaintext_length;
	(void)tag;
	(void)tag_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_aead_abort(struct service_client *context,
	uint32_t *op_handle)
{
	(void)context;
	(void)op_handle;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline size_t crypto_caller_aead_max_update_ad_size(const struct service_client *context)
{
	return 0;
}

static inline size_t crypto_caller_aead_max_update_size(const struct service_client *context)
{
	return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* STUB_CRYPTO_CALLER_AEAD_H */
