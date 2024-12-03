/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "mock_libc.h"

#include <CppUTestExt/MockSupport.h>

/*
 * Do not forget to redefine the mocked function with the test function on cmake level e.g:
 *
 * set_source_files_properties(<source.c> PROPERTIES
 *	COMPILE_DEFINITIONS calloc=MOCK_CALLOC
 * )
 */

static bool mock_libc_enabled = false;

void mock_libc_enable(void)
{
	mock_libc_enabled = true;
}

void mock_libc_disable(void)
{
	mock_libc_enabled = false;
}

void expect_malloc(void *result)
{
	mock().expectOneCall("MOCK_MALLOC")
		.andReturnValue(result);
}

void* MOCK_MALLOC(size_t size)
{
	if (!mock_libc_enabled)
		return malloc(size);

	void* result = mock().actualCall("MOCK_MALLOC")
		.returnPointerValue();

	if (result != NULL) {
		result = malloc(size);
	}

	return result;
}

void expect_calloc(void *result)
{
	mock().expectOneCall("MOCK_CALLOC")
		.andReturnValue(result);
}

void* MOCK_CALLOC(size_t nmemb, size_t size)
{
	if (!mock_libc_enabled)
		return calloc(nmemb, size);

	void* result = mock().actualCall("MOCK_CALLOC")
		.returnPointerValue();

	if (result != NULL) {
		result = calloc(nmemb, size);
	}

	return result;
}
