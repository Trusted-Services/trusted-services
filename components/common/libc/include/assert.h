/*
 * Copyright (c) 2018-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ASSERT_H
#define ASSERT_H

#include <cdefs.h>

#ifndef NDEBUG
void __dead2 __assert(const char *file, int line, const char *func, const char *assertion);

#define assert(e)	((e) ? (void)0 : __assert(__FILE__, __LINE__, __func__, #e))
#else
#define assert(e)	((void)(e))
#endif

#endif /* ASSERT_H */
