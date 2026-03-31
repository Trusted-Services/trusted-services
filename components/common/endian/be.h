/*
 * Copyright (c) 2026, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ENDIAN_BE_H
#define ENDIAN_BE_H

#include <stddef.h>  // for size_t
#include <stdint.h>  // for uint16_t, uint32_t, uint64_t, uint8_t

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Functions for loading and storing integer values as unaligned
 * values in Big Endian byte order.  The address to load or
 * store the value is specified by a base address and an offset
 * to facilitate unaligned structure access.
 */
uint8_t  load_u8_be(const void *base, size_t offset);
uint16_t load_u16_be(const void *base, size_t offset);
uint32_t load_u32_be(const void *base, size_t offset);
uint64_t load_u64_be(const void *base, size_t offset);

void store_u8_be(void *base, size_t offset, uint8_t val);
void store_u16_be(void *base, size_t offset, uint16_t val);
void store_u32_be(void *base, size_t offset, uint32_t val);
void store_u64_be(void *base, size_t offset, uint64_t val);

#ifdef __cplusplus
}
#endif

#endif /* ENDIAN_BE_H */
