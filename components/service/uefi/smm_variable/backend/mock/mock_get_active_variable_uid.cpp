/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <psa/error.h>

#include <CppUTestExt/MockSupport.h>

#include "uefi_variable_store_test_if.h"

extern "C" void expect_get_active_variable_uid(psa_status_t result,
					       uint64_t *active_index_uid,
					       uint32_t *counter)
{
	MockExpectedCall &call = mock()
					 .expectOneCall("get_active_variable_uid")
					 .ignoreOtherParameters();

	if (active_index_uid)
		call.withOutputParameterReturning("active_index_uid", active_index_uid,
						  sizeof(*active_index_uid));

	if (counter)
		call.withOutputParameterReturning("counter", counter, sizeof(*counter));

	call.andReturnValue(result);
}

extern "C" psa_status_t get_active_variable_uid(struct uefi_variable_store *context,
						uint64_t *active_index_uid,
						uint32_t *counter)
{
	return mock()
		.actualCall("get_active_variable_uid")
		.withPointerParameter("context", context)
		.withOutputParameter("active_index_uid", active_index_uid)
		.withOutputParameter("counter", counter)
		.returnIntValue();
}
