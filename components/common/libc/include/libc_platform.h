/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2021-2024, Arm Limited and Contributors. All rights reserved.
 */

#ifndef LIBC_PLATFORM_H_
#define LIBC_PLATFORM_H_

#include <cdefs.h>

/*
 * Generic assert fail and abort handler function definitions.
 * Should be implemented by the environment.
 */
void __dead2 platform_assert(const char *file, int line, const char *func,
			     const char *failedexpr);

void __dead2 platform_abort(void);

#endif /* LIBC_PLATFORM_H_ */
