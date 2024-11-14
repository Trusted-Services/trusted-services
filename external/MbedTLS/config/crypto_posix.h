/*
 * Copyright (c) 2020-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CONFIG_CRYPTO_POSIX_H
#define CONFIG_CRYPTO_POSIX_H

/*
 * Mbed TLS configuration for using libmbedcrypto in
 * a Posix environment (normal world demo and test applications).
 */

/*
 * Enable using crypto_config.h, but do not define custom crypto header with
 * MBEDTLS_PSA_CRYPTO_CONFIG_FILE to enable all the supported algorithms.
 */
#define MBEDTLS_PSA_CRYPTO_CONFIG

#define MBEDTLS_BASE64_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_ECP_NIST_OPTIM
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_HAVE_ASM
#define MBEDTLS_LMS_C
#define MBEDTLS_NIST_KW_C
#define MBEDTLS_NO_UDBL_DIVISION
#define MBEDTLS_OID_C
#define MBEDTLS_PKCS12_C
#define MBEDTLS_PKCS5_C
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_PK_WRITE_C
#define MBEDTLS_PSA_CRYPTO_C
#define MBEDTLS_X509_CRL_PARSE_C
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_X509_USE_C

#endif /* CONFIG_CRYPTO_POSIX_H */
