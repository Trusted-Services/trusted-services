/*
 * Copyright (c) 2020-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CONFIG_CRYPTO_PROVIDER_X509_H
#define CONFIG_CRYPTO_PROVIDER_X509_H

/*
 * MbedTLS configuration for building libmbedcrypto and libx509 to act as a backend
 * for the crypto service provider.
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
#define MBEDTLS_PSA_CRYPTO_STORAGE_C
#define MBEDTLS_X509_CRL_PARSE_C
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_X509_USE_C

#define MBEDTLS_PSA_CRYPTO_KEY_ID_ENCODES_OWNER
#define BACKEND_CRYPTO_API_ADD_PREFIX(f) __mbedtls_backend_##f
#include "../../../components/service/crypto/backend/prefixed_crypto_api.h"

#endif /* CONFIG_CRYPTO_PROVIDER_X509_H */
