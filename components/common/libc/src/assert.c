/*
 * Copyright (c) 2013-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#ifndef NDEBUG
#include "libc_platform.h"

void __dead2 __assert(const char *file, int line, const char *func, const char *assertion)
{
	platform_assert(file, line, func, assertion);
}
#endif
