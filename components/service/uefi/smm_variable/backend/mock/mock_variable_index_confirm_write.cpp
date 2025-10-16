/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <CppUTestExt/MockSupport.h>

#include "uefi_variable_store_test_if.h"

extern "C" void expect_variable_index_confirm_write(struct variable_index *context)
{
	mock().expectOneCall("variable_index_confirm_write")
		.withPointerParameter("context", context);
}

extern "C" void variable_index_confirm_write(struct variable_index *context)
{
	mock().actualCall("variable_index_confirm_write")
		.withPointerParameter("context", context);
}
