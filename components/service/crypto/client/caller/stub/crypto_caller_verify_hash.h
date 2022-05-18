/*
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef STUB_CRYPTO_CALLER_VERIFY_HASH_H
#define STUB_CRYPTO_CALLER_VERIFY_HASH_H

#include <psa/crypto.h>
#include <service/common/client/service_client.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline psa_status_t crypto_caller_verify_hash(struct service_client *context,
	psa_key_id_t id,
	psa_algorithm_t alg,
	const uint8_t *hash, size_t hash_length,
	const uint8_t *signature, size_t signature_length)
{
	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_verify_message(struct service_client *context,
	psa_key_id_t id,
	psa_algorithm_t alg,
	const uint8_t *input, size_t input_length,
	const uint8_t *signature, size_t signature_length)
{
	return PSA_ERROR_NOT_SUPPORTED;
}

#ifdef __cplusplus
}
#endif

#endif /* STUB_CRYPTO_CALLER_VERIFY_HASH_H */
