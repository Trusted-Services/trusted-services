/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BLK_ENCRYPT_CONFIG_H
#define BLK_ENCRYPT_CONFIG_H

/* MBEDTLS_PSA_CRYPTO_CONFIG_FILE is defined on CMAKE level for custom algorithms */
#define MBEDTLS_PSA_CRYPTO_CONFIG

#define MBEDTLS_HAVE_ASM
#define MBEDTLS_PSA_CRYPTO_C
#define MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG

#endif /* BLK_ENCRYPT_CONFIG_H */
