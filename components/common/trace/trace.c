// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
 */

#include "trace.h"
#include <stdarg.h>
#include <stdio.h>

#if TRACE_LEVEL >= TRACE_LEVEL_ERROR
#ifndef TRACE_PREFIX
#error TRACE_PREFIX must be defined
#endif /* TRACE_PREFIX */

void (*trace_puts_interface)(const char *str) = &trace_puts;

void ts_trace_printf(const char *func, int line, int level, const char *fmt, ...)
{
	char buffer[256];
	char level_char = 0;
	int offset = 0;
	va_list ap;
	static const char levels[] = {'E', 'I', 'D'};

	if (TRACE_LEVEL_ERROR <= level && TRACE_LEVEL_DEBUG >= level)
		level_char = levels[level - TRACE_LEVEL_ERROR];
	else
		level_char = '?';

	offset = snprintf(buffer, sizeof(buffer), "%c/" TRACE_PREFIX ": %s:%d ",
			  level_char, func, line);

	if (offset < sizeof(buffer)) {
		va_start(ap, fmt);
		offset += vsnprintf(buffer + offset, sizeof(buffer) - offset, fmt, ap);
		va_end(ap);
	}

	if (offset < sizeof(buffer) - 2) {
		buffer[offset] = '\n';
		buffer[offset + 1] = '\0';
	} else {
		buffer[sizeof(buffer) - 2] = '\n';
		buffer[sizeof(buffer) - 1] = '\0';
	}

	(*trace_puts_interface)(buffer);
}
#endif
