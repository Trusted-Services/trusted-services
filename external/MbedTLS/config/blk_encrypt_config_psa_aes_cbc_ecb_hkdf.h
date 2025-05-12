/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BLK_ENCRYPT_CONFIG_PSA_AES_CBC_ECB_HKDF
#define BLK_ENCRYPT_CONFIG_PSA_AES_CBC_ECB_HKDF

/*
 * This file is a PSA crypto configuration file, provided by
 * MBEDTLS_PSA_CRYPTO_CONFIG_FILE macro for MbedTLS instead of
 * the MBEDTLS_CONFIG_FILE macro, which is the normal configuration
 * file.
 * This configuration helps decreasing the size of the library by
 * enabling only the necessary crypto algorithms.
 */

/* Enable AES with CBC and ECB modes */
#define PSA_WANT_KEY_TYPE_AES	    1
#define PSA_WANT_ALG_CBC_NO_PADDING 1
#define PSA_WANT_ALG_ECB_NO_PADDING 1

/* Enable HKDF key derivation with HMAC-SHA algorithms */
#define PSA_WANT_KEY_TYPE_DERIVE  1
#define PSA_WANT_ALG_HKDF	  1
#define PSA_WANT_ALG_HKDF_EXTRACT 1
#define PSA_WANT_ALG_HKDF_EXPAND  1
#define PSA_WANT_ALG_HMAC	  1
#define PSA_WANT_ALG_SHA_256	  1

#endif /* BLK_ENCRYPT_CONFIG_PSA_AES_CBC_ECB_HKDF */
