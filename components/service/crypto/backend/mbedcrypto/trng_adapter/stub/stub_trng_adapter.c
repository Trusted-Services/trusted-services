/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <mbedtls/entropy.h>
#include <service/crypto/backend/mbedcrypto/trng_adapter/trng_adapter.h>
#include <psa/error.h>
#include <stddef.h>

int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen)
{
    (void)data;
    (void)output;
    (void)len;
    (void)olen;

    return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
}
