/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef MOCK_LIBC_H
#define MOCK_LIBC_H

#include "../include/malloc.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * libc is used everywhere in the sources so it useful to enable mocking
 * only for the realted testcases to avoid the necessity of filling the
 * tests with a huge amount of expect calls.
 */
void mock_libc_enable(void);
void mock_libc_disable(void);

void expect_malloc(void *result);
void* MOCK_MALLOC(size_t size);

void expect_calloc(void *result);
void* MOCK_CALLOC(size_t nmemb, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_LIBC_H */