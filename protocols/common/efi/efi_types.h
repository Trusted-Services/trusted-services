/*
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef COMMON_EFI_TYPES_H
#define COMMON_EFI_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/**
 * Common EFI types
 */

/**
 * 128 bit buffer containing a unique identifier value.
 * Unless otherwise specified, aligned on a 64 bit boundary.
 */
typedef struct {
	uint32_t Data1;
	uint16_t Data2;
	uint16_t Data3;
	uint8_t  Data4[8];
} EFI_GUID;

/**
 * Common time representation
 */
typedef struct {
	uint16_t  Year;
	uint8_t   Month;
	uint8_t   Day;
	uint8_t   Hour;
	uint8_t   Minute;
	uint8_t   Second;
	uint8_t   Pad1;
	uint32_t  Nanosecond;
	uint16_t  TimeZone;
	uint8_t   Daylight;
	uint8_t   Pad2;
} EFI_TIME;

/**
 * Header structure of messages in the MM communication buffer.
 */
typedef struct {
	EFI_GUID HeaderGuid;
	uint64_t MessageLength;
	uint8_t Data[1];
} EFI_MM_COMMUNICATE_HEADER;

/**
 * Size of the EFI MM_COMMUNICATE header without the data field.
 */
#define EFI_MM_COMMUNICATE_HEADER_SIZE	\
	(offsetof(EFI_MM_COMMUNICATE_HEADER, Data))

/*
 * Returns whether the two guid-s equal. To avoid structure padding related error
 * the fields are checked separately instead of memcmp.
 */
static inline bool compare_guid(const EFI_GUID *guid1, const EFI_GUID *guid2)
{
	return guid1->Data1 == guid2->Data1 && guid1->Data2 == guid2->Data2 &&
	       guid1->Data3 == guid2->Data3 &&
	       !memcmp(&guid1->Data4, &guid2->Data4, sizeof(guid1->Data4));
}

#endif /* COMMON_EFI_TYPES_H */
