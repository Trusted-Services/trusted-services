// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2024, Arm Limited and Contributors. All rights reserved.
 */

#include "libc_platform.h"
#include "trace.h"

/*
 * The generic trace function called on assert fail.
 */
void __noreturn platform_assert(const char *file, int line, const char *func,
				const char *failedexpr)
{
#if TRACE_LEVEL >= TRACE_LEVEL_ERROR
	ts_trace_printf(func, line, TRACE_LEVEL_ERROR, "assertion %s failed", failedexpr);
#endif /* TRACE_LEVEL */

	while (1)
		;
}

void __noreturn platform_abort(void)
{
#if TRACE_LEVEL >= TRACE_LEVEL_ERROR
	trace_puts("abort()");
#endif /* TRACE_LEVEL */

	while (1)
		;
}
