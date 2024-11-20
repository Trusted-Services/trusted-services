/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>

#include "psa/crypto_platform.h"
#include "psa/crypto_types.h"
#include "psa/crypto_values.h"

psa_status_t mbedtls_psa_external_get_random(mbedtls_psa_external_random_context_t *context,
					     uint8_t *output, size_t output_size,
					     size_t *output_length)
{
	(void)context;
	(void)output;
	(void)output_size;

	*output_length = 0;
	return PSA_ERROR_INSUFFICIENT_ENTROPY;
}
