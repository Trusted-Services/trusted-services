// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 */

#include <stdio.h>

#include "trace.h"

#if TRACE_LEVEL >= TRACE_LEVEL_ERROR

void trace_puts(const char *str)
{
	puts(str);
}

#endif /* TRACE_LEVEL >= TRACE_LEVEL_ERROR */
