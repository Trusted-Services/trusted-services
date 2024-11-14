// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2024, Arm Limited and Contributors. All rights reserved.
 */

#include "compiler.h"
#include <errno.h>
#include <stddef.h> /* dlmalloc requires size_t */
#include <stdint.h>
#include <string.h>

/* Allocating heap area */
#ifndef SP_HEAP_SIZE
#error "SP_HEAP_SIZE is undefined, please define it in the build system"
#endif

static uint8_t sp_heap[SP_HEAP_SIZE] __aligned(16);
static uint8_t *program_break = sp_heap;

/**
 * Basic sbrk implementation which increases the program break through the
 * sp_heap buffer.
 */
void *sbrk(ptrdiff_t incr)
{
	uint8_t *previous_break = program_break;
	uint8_t *new_break = program_break + incr;

	if ((new_break < sp_heap) || (new_break > (sp_heap + sizeof(sp_heap))))
		return (void *)(uintptr_t) -1;

	program_break += incr;

	return (void *) previous_break;
}



/*
 * There's no way of including a custom configuration file without modifying
 * malloc.c. As a workaround this file includes the missing stddef.h, sets the
 * configuration values of dlmalloc and then includes malloc.c.
 */

/* dlmalloc configuration */
#define USE_SPIN_LOCKS	0
#define HAVE_MORECORE	1
#define HAVE_MMAP	0
#define HAVE_MREMAP	0
#define MALLOC_FAILURE_ACTION	do {} while (0)
#define MMAP_CLEARS	0
#define MORECORE_CONTIGUOUS	1
#define MORECORE_CANNOT_TRIM	1
#define LACKS_SYS_PARAM_H	1
#define LACKS_SYS_TYPES_H	1
#define LACKS_TIME_H	1
#define LACKS_UNISTD_H	1
#define NO_MALLINFO	1
#define NO_MALLOC_STATS	1
#define DEFAULT_GRANULARITY 64

#include "malloc.c"
