/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

/*
 * Enable using crypto_config.h, but do not define custom crypto header with
 * MBEDTLS_PSA_CRYPTO_CONFIG_FILE to enable all the supported algorithms.
 */
#define MBEDTLS_PSA_CRYPTO_CONFIG

#define MBEDTLS_BASE64_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_ECP_NIST_OPTIM
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_ENTROPY_HARDWARE_ALT
#define MBEDTLS_GENPRIME
#define MBEDTLS_HAVE_ASM
#define MBEDTLS_LMS_C
#define MBEDTLS_NIST_KW_C
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_NO_UDBL_DIVISION
#define MBEDTLS_OID_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_PEM_PARSE_C
#define MBEDTLS_PEM_WRITE_C
#define MBEDTLS_PKCS12_C
#define MBEDTLS_PKCS5_C
#define MBEDTLS_PKCS7_C
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_PK_WRITE_C
#define MBEDTLS_PSA_CRYPTO_C
#define MBEDTLS_X509_CRL_PARSE_C
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_X509_USE_C

#endif /* MBEDTLS_CONFIG_H */
