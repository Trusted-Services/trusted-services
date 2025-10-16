/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <CppUTestExt/MockSupport.h>

#include "uefi_variable_store_test_if.h"

extern "C" void expect_variable_index_restore(struct variable_index *context,
					      size_t data_len,
					      size_t result)
{
	mock().expectOneCall("variable_index_restore")
		.withPointerParameter("context", context)
		.withUnsignedLongIntParameter("data_len", data_len)
		.ignoreOtherParameters()
		.andReturnValue(result);
}

extern "C" size_t variable_index_restore(struct variable_index *context,
					 size_t data_len,
					 const uint8_t *buffer)
{
	return mock()
		.actualCall("variable_index_restore")
		.withPointerParameter("context", context)
		.withUnsignedLongIntParameter("data_len", data_len)
		.withConstPointerParameter("buffer", buffer)
		.returnUnsignedLongIntValue();
}
