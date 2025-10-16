/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <CppUTestExt/MockSupport.h>

#include "uefi_variable_store_test_if.h"

extern "C" void expect_variable_index_dump(size_t *data_len, bool *any_dirty,
					   efi_status_t result)
{
	MockExpectedCall &call = mock().expectOneCall("variable_index_dump")
					   .ignoreOtherParameters();

	if (data_len)
		call.withOutputParameterReturning("data_len", data_len, sizeof(*data_len));

	if (any_dirty)
		call.withOutputParameterReturning("any_dirty", any_dirty, sizeof(*any_dirty));

	call.andReturnValue(result);
}

extern "C" efi_status_t variable_index_dump(const struct variable_index *context,
					    size_t buffer_size, uint8_t *buffer,
					    size_t *data_len, bool *any_dirty)
{
	return mock()
		.actualCall("variable_index_dump")
		.withConstPointerParameter("context", context)
		.withUnsignedLongIntParameter("buffer_size", buffer_size)
		.withPointerParameter("buffer", buffer)
		.withOutputParameter("data_len", data_len)
		.withOutputParameter("any_dirty", any_dirty)
		.returnUnsignedLongLongIntValue();
}
