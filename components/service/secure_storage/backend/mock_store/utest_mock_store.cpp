/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <CppUTestExt/MockSupport.h>

#include "utest_mock_store.h"

extern "C" void expect_set(psa_status_t result)
{
	mock().expectOneCall("set").ignoreOtherParameters().andReturnValue(result);
}

extern "C" void expect_get(size_t *data_length, psa_status_t result)
{
	MockExpectedCall &call = mock().expectOneCall("get").ignoreOtherParameters();

	if (data_length)
		call.withOutputParameterReturning("p_data_length", data_length,
						  sizeof(*data_length));

	call.andReturnValue(result);
}

extern "C" void expect_get_info(psa_storage_info_t *info, psa_status_t result)
{
	MockExpectedCall &call = mock().expectOneCall("get_info").ignoreOtherParameters();

	if (info)
		call.withOutputParameterReturning("p_info", info, sizeof(*info));

	call.andReturnValue(result);
}

extern "C" void expect_remove(psa_status_t result)
{
	mock().expectOneCall("remove").ignoreOtherParameters().andReturnValue(result);
}

extern "C" void expect_create(psa_status_t result)
{
	mock().expectOneCall("create").ignoreOtherParameters().andReturnValue(result);
}

extern "C" void expect_set_extended(psa_status_t result)
{
	mock().expectOneCall("set_extended")
		.ignoreOtherParameters()
		.andReturnValue(result);
}

extern "C" void expect_set_extended_with_length(size_t data_length, psa_status_t result)
{
	mock().expectOneCall("set_extended")
		.withUnsignedLongIntParameter("data_length", data_length)
		.ignoreOtherParameters()
		.andReturnValue(result);
}

extern "C" void expect_get_support(uint32_t support)
{
	mock().expectOneCall("get_support")
		.ignoreOtherParameters()
		.andReturnValue(support);
}

extern "C" {
static psa_status_t set(void *context, uint32_t client_id, uint64_t uid,
			size_t data_length, const void *p_data,
			uint32_t create_flags)
{
	return mock()
		.actualCall("set")
		.withPointerParameter("context", context)
		.withUnsignedIntParameter("client_id", client_id)
		.withUnsignedLongIntParameter("uid", uid)
		.withUnsignedLongIntParameter("data_length", data_length)
		.withConstPointerParameter("p_data", p_data)
		.withUnsignedIntParameter("create_flags", create_flags)
		.returnIntValue();
}

static psa_status_t get(void *context, uint32_t client_id, uint64_t uid,
			size_t data_offset, size_t data_size, void *p_data,
			size_t *p_data_length)
{
	return mock()
		.actualCall("get")
		.withPointerParameter("context", context)
		.withUnsignedIntParameter("client_id", client_id)
		.withUnsignedLongIntParameter("uid", uid)
		.withUnsignedLongIntParameter("data_offset", data_offset)
		.withUnsignedLongIntParameter("data_size", data_size)
		.withMemoryBufferParameter("p_data", static_cast<const uint8_t *>(p_data),
					   data_size)
		.withOutputParameter("p_data_length", p_data_length)
		.returnIntValue();
}

static psa_status_t get_info(void *context, uint32_t client_id, uint64_t uid,
			     struct psa_storage_info_t *p_info)
{
	return mock()
		.actualCall("get_info")
		.withPointerParameter("context", context)
		.withUnsignedIntParameter("client_id", client_id)
		.withUnsignedLongIntParameter("uid", uid)
		.withOutputParameter("p_info", p_info)
		.returnIntValue();
}

static psa_status_t remove_item(void *context, uint32_t client_id, uint64_t uid)
{
	return mock()
		.actualCall("remove")
		.withPointerParameter("context", context)
		.withUnsignedIntParameter("client_id", client_id)
		.withUnsignedLongIntParameter("uid", uid)
		.returnIntValue();
}

static psa_status_t create(void *context, uint32_t client_id, uint64_t uid,
			   size_t capacity, uint32_t create_flags)
{
	return mock()
		.actualCall("create")
		.withPointerParameter("context", context)
		.withUnsignedIntParameter("client_id", client_id)
		.withUnsignedLongIntParameter("uid", uid)
		.withUnsignedLongIntParameter("capacity", capacity)
		.withUnsignedIntParameter("create_flags", create_flags)
		.returnIntValue();
}

static psa_status_t set_extended(void *context, uint32_t client_id, uint64_t uid,
				 size_t data_offset, size_t data_length,
				 const void *p_data)
{
	return mock()
		.actualCall("set_extended")
		.withPointerParameter("context", context)
		.withUnsignedIntParameter("client_id", client_id)
		.withUnsignedLongIntParameter("uid", uid)
		.withUnsignedLongIntParameter("data_offset", data_offset)
		.withUnsignedLongIntParameter("data_length", data_length)
		.withConstPointerParameter("p_data", p_data)
		.returnIntValue();
}

static uint32_t get_support(void *context, uint32_t client_id)
{
	return mock()
		.actualCall("get_support")
		.withPointerParameter("context", context)
		.withUnsignedIntParameter("client_id", client_id)
		.returnUnsignedIntValue();
}
} // extern "C"

extern "C" const struct storage_backend_interface uefi_variable_store_test_mock_storage_backend_interface = {
	.set = &set,
	.get = &get,
	.get_info = &get_info,
	.remove = &remove_item,
	.create = &create,
	.set_extended = &set_extended,
	.get_support = &get_support
};
