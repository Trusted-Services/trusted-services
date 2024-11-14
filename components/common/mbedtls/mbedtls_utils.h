/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MBEDTLS_UTILS_H
#define MBEDTLS_UTILS_H

#include <mbedtls/asn1.h>

#ifdef __cplusplus
extern "C" {
#endif

const mbedtls_asn1_buf* findCommonName(const mbedtls_asn1_named_data *name);

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_UTILS_H */
