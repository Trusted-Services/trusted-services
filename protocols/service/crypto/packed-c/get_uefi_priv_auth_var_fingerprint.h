/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TS_CRYPTO_GET_UEFI_PRIV_AUTH_VAR_FINGERPRINT_H
#define TS_CRYPTO_GET_UEFI_PRIV_AUTH_VAR_FINGERPRINT_H

#include <stdint.h>

/* Variable length output parameter tags */
enum {
	TS_CRYPTO_GET_UEFI_PRIV_AUTH_VAR_FINGERPRINT_OUT_TAG_IDENTIFIER = 1,
};

/* Variable length input parameter tags */
enum {
	TS_CRYPTO_GET_UEFI_PRIV_AUTH_VAR_FINGERPRINT_IN_TAG_SIGNATURE = 1,
};

#endif /* TS_CRYPTO_GET_UEFI_PRIV_AUTH_VAR_FINGERPRINT_H */
