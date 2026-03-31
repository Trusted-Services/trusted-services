/*
 * Copyright (c) 2026, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "be.h"

uint8_t  load_u8_be(const void *base, size_t offset)
{
	const uint8_t *v = (const uint8_t *)base + offset;
	uint8_t r = v[0];

	return r;
}

uint16_t load_u16_be(const void *base, size_t offset)
{
	const uint8_t *v = (const uint8_t *)base + offset;
	uint16_t r = 0;

	r |= (uint16_t)v[1];
	r |= (uint16_t)v[0] << 8;

	return r;
}

uint32_t load_u32_be(const void *base, size_t offset)
{
	const uint8_t *v = (const uint8_t *)base + offset;
	uint32_t r = 0;

	r |= (uint32_t)v[3];
	r |= (uint32_t)v[2] << 8;
	r |= (uint32_t)v[1] << 16;
	r |= (uint32_t)v[0] << 24;

	return r;
}

uint64_t load_u64_be(const void *base, size_t offset)
{
	const uint8_t *v = (const uint8_t *)base + offset;
	uint64_t r = 0;

	r |= (uint64_t)v[7];
	r |= (uint64_t)v[6] << 8;
	r |= (uint64_t)v[5] << 16;
	r |= (uint64_t)v[4] << 24;
	r |= (uint64_t)v[3] << 32;
	r |= (uint64_t)v[2] << 40;
	r |= (uint64_t)v[1] << 48;
	r |= (uint64_t)v[0] << 56;

	return r;
}

void store_u8_be(void *base, size_t offset, uint8_t val)
{
	uint8_t *v = (uint8_t *)base + offset;

	v[0] = val;
}

void store_u16_be(void *base, size_t offset, uint16_t val)
{
	uint8_t *v = (uint8_t *)base + offset;

	v[1] = (uint8_t)(val & 0xff);
	v[0] = (uint8_t)((val >> 8) & 0xff);
}

void store_u32_be(void *base, size_t offset, uint32_t val)
{
	uint8_t *v = (uint8_t *)base + offset;

	v[3] = (uint8_t)(val & 0xff);
	v[2] = (uint8_t)((val >> 8) & 0xff);
	v[1] = (uint8_t)((val >> 16) & 0xff);
	v[0] = (uint8_t)((val >> 24) & 0xff);
}

void store_u64_be(void *base, size_t offset, uint64_t val)
{
	uint8_t *v = (uint8_t *)base + offset;

	v[7] = (uint8_t)(val & 0xff);
	v[6] = (uint8_t)((val >> 8) & 0xff);
	v[5] = (uint8_t)((val >> 16) & 0xff);
	v[4] = (uint8_t)((val >> 24) & 0xff);
	v[3] = (uint8_t)((val >> 32) & 0xff);
	v[2] = (uint8_t)((val >> 40) & 0xff);
	v[1] = (uint8_t)((val >> 48) & 0xff);
	v[0] = (uint8_t)((val >> 56) & 0xff);
}
