/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef UTEST_MOCK_STORE_H
#define UTEST_MOCK_STORE_H

#include <service/secure_storage/backend/storage_backend.h>

#ifdef __cplusplus
extern "C" {
#endif

void expect_set(psa_status_t result);
void expect_get(size_t *data_length, psa_status_t result);
void expect_get_info(psa_storage_info_t *info, psa_status_t result);
void expect_remove(psa_status_t result);
void expect_create(psa_status_t result);
void expect_set_extended(psa_status_t result);
void expect_set_extended_with_length(size_t data_length, psa_status_t result);
void expect_get_support(uint32_t support);

#ifdef __cplusplus
}
#endif

#endif /* UTEST_MOCK_STORE_H */
