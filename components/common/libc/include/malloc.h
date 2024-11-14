/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

/*
 * The declaration of this function is part of libc but it is implemented in
 * the allocator.
 */
void *memalign(size_t alignment, size_t size);

#endif /* MEMORY_H */