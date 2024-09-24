/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
 */

#ifndef TRACE_H_
#define TRACE_H_

#include "compiler.h"

#ifdef EXPORT_PUBLIC_INTERFACE_TRACE
#define TRACE_EXPORTED __attribute__((__visibility__("default")))
#else
#define TRACE_EXPORTED
#endif

#define TRACE_LEVEL_NONE	(0)
#define TRACE_LEVEL_ERROR	(1)
#define TRACE_LEVEL_INFO	(2)
#define TRACE_LEVEL_DEBUG	(3)

#ifndef TRACE_LEVEL
#define TRACE_LEVEL	TRACE_LEVEL_ERROR
#endif /* TRACE_LEVEL */

/**
 * no_trace_printf will be optimized out becase of the 'if (0)' but all the
 * checks will still run against the format string and the parameters.
 */
#define no_trace_printf(func, line, level, fmt, ...)				\
	do {									\
		if (0) {							\
			trace_printf(func, line, level, fmt, ##__VA_ARGS__);	\
		}								\
	} while (0)

extern void (*trace_puts_interface)(const char *str);
void trace_puts(const char *str);
TRACE_EXPORTED
void trace_printf(const char *func, int line, int level, const char *fmt, ...) __printf(4, 5);

#if TRACE_LEVEL >= TRACE_LEVEL_ERROR
#define EMSG(...)	trace_printf(__func__, __LINE__, TRACE_LEVEL_ERROR, __VA_ARGS__)
#else
#define EMSG(...)	no_trace_printf(__func__, __LINE__, TRACE_LEVEL_ERROR, __VA_ARGS__)
#endif /* TRACE_LEVEL >= TRACE_LEVEL_ERROR */

#if TRACE_LEVEL >= TRACE_LEVEL_INFO
#define IMSG(...)	trace_printf(__func__, __LINE__, TRACE_LEVEL_INFO, __VA_ARGS__)
#else
#define IMSG(...)	no_trace_printf(__func__, __LINE__, TRACE_LEVEL_INFO, __VA_ARGS__)
#endif /* TRACE_LEVEL >= TRACE_LEVEL_INFO */

#if TRACE_LEVEL >= TRACE_LEVEL_DEBUG
#define DMSG(...)	trace_printf(__func__, __LINE__, TRACE_LEVEL_DEBUG, __VA_ARGS__)
#else
#define DMSG(...)	no_trace_printf(__func__, __LINE__, TRACE_LEVEL_DEBUG, __VA_ARGS__)
#endif /* TRACE_LEVEL >= TRACE_LEVEL_DEBUG */

#endif /* TRACE_H_ */
