/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef MOCK_CRYPTO_H
#define MOCK_CRYPTO_H

#include <psa/crypto.h>
#ifdef __cplusplus
extern "C" {
#endif

void expect_psa_crypto_init(psa_status_t result);
void expect_psa_reset_key_attributes(psa_key_attributes_t *attributes);
void expect_psa_destroy_key(psa_key_id_t key, psa_status_t result);
void expect_psa_import_key(const psa_key_attributes_t *attributes, const uint8_t *data,
			   size_t data_length, psa_key_id_t *key, psa_status_t result);
void expect_psa_cipher_encrypt_setup(psa_cipher_operation_t *operation, psa_key_id_t key,
				     psa_algorithm_t alg, psa_status_t result);
void expect_psa_cipher_decrypt_setup(psa_cipher_operation_t *operation, psa_key_id_t key,
				     psa_algorithm_t alg, psa_status_t result);
void expect_psa_cipher_set_iv(psa_cipher_operation_t *operation, const uint8_t *iv,
			      size_t iv_length, psa_status_t result);
void expect_psa_cipher_update(psa_cipher_operation_t *operation, const uint8_t *input,
			      size_t input_length, uint8_t *output, size_t output_size,
			      size_t *output_length, psa_status_t result);
void expect_psa_cipher_finish(psa_cipher_operation_t *operation, uint8_t *output,
			      size_t output_size, size_t *output_length, psa_status_t result);
void expect_psa_cipher_abort(psa_cipher_operation_t *operation, psa_status_t result);
void expect_psa_key_derivation_setup(psa_key_derivation_operation_t *operation, psa_algorithm_t alg,
				     psa_status_t result);
void expect_psa_key_derivation_input_bytes(psa_key_derivation_operation_t *operation,
					   psa_key_derivation_step_t step, const uint8_t *data,
					   size_t data_length, psa_status_t result);
void expect_psa_key_derivation_input_key(psa_key_derivation_operation_t *operation,
					 psa_key_derivation_step_t step, psa_key_id_t key,
					 psa_status_t result);
void expect_psa_key_derivation_output_key(const psa_key_attributes_t *attributes,
					  psa_key_derivation_operation_t *operation,
					  psa_key_id_t *key, psa_status_t result);
void expect_psa_key_derivation_abort(psa_key_derivation_operation_t *operation,
				     psa_status_t result);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_CRYPTO_H */
