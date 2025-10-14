/*
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef STUB_CRYPTO_CALLER_MAC_H
#define STUB_CRYPTO_CALLER_MAC_H

#include <psa/crypto.h>
#include <service/common/client/service_client.h>

#ifdef __cplusplus
extern "C" {
#endif


static inline psa_status_t crypto_caller_mac_sign_setup(struct service_client *context,
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

static inline psa_status_t crypto_caller_mac_verify_setup(struct service_client *context,
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

static inline psa_status_t crypto_caller_mac_update(struct service_client *context,
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

static inline psa_status_t crypto_caller_mac_sign_finish(struct service_client *context,
	uint32_t *op_handle,
	uint8_t *mac,
	size_t mac_size,
	size_t *mac_length)
{
	(void)context;
	(void)op_handle;
	(void)mac;
	(void)mac_size;
	(void)mac_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_mac_verify_finish(struct service_client *context,
	uint32_t *op_handle,
	const uint8_t *mac,
	size_t mac_length)
{
	(void)context;
	(void)op_handle;
	(void)mac;
	(void)mac_length;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline psa_status_t crypto_caller_mac_abort(struct service_client *context,
	uint32_t *op_handle)
{
	(void)context;
	(void)op_handle;

	return PSA_ERROR_NOT_SUPPORTED;
}

static inline size_t crypto_caller_mac_max_update_size(struct service_client *context)
{
	(void)context;

	return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* STUB_CRYPTO_CALLER_MAC_H */
