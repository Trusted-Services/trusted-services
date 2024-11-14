/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CRYPTO_CLIENT_H
#define CRYPTO_CLIENT_H

#include <stddef.h>
#include <stdint.h>

int verify_pkcs7_signature(const uint8_t *signature_cert, uint64_t signature_cert_len,
			   const uint8_t *hash, uint64_t hash_len, const uint8_t *public_key_cert,
			   uint64_t public_key_cert_len);

int get_uefi_priv_auth_var_fingerprint_handler(const uint8_t *signature_cert,
				    uint64_t signature_cert_len,
				    uint8_t *output);

#endif /* CRYPTO_CLIENT_H */
