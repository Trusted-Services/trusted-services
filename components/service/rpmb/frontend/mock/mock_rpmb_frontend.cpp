/*
 * Copyright (c) 2026, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <CppUTestExt/MockSupport.h>

#include "components/service/common/include/psa/error.h"
#include "mock_rpmb_frontend.h"

#include <stdint.h>

void expect_rpmb_frontend_init(struct rpmb_frontend *context, psa_status_t result)
{
	mock().expectOneCall("rpmb_frontend_init").onObject(context).andReturnValue(result);
}

extern "C" psa_status_t rpmb_frontend_init(struct rpmb_frontend *context)
{
	return mock().actualCall("rpmb_frontend_init").onObject(context).returnIntValue();
}

void expect_rpmb_frontend_create(struct rpmb_frontend *context, uint32_t dev_id,
				 psa_status_t result)
{
	mock().expectOneCall("rpmb_frontend_create")
		.onObject(context)
		.withUnsignedIntParameter("dev_is", dev_id)
		.andReturnValue(result);
}

extern "C" psa_status_t rpmb_frontend_create(struct rpmb_frontend *context, uint32_t dev_id)
{
	return mock()
		.actualCall("rpmb_frontend_create")
		.onObject(context)
		.withUnsignedIntParameter("dev_is", dev_id)
		.returnIntValue();
}

void expect_rpmb_frontend_destroy(struct rpmb_frontend *context)
{
	mock().expectOneCall("rpmb_frontend_destroy").onObject(context);
}

extern "C" void rpmb_frontend_destroy(struct rpmb_frontend *context)
{
	mock().actualCall("rpmb_frontend_destroy").onObject(context).returnValue();
}

void expect_rpmb_frontend_block_size(struct rpmb_frontend *context, size_t block_size,
				     psa_status_t result)
{
	static size_t bs_copy;

	bs_copy = block_size;
	mock().expectOneCall("rpmb_frontend_block_size")
		.onObject(context)
		.withOutputParameterReturning("block_size", &bs_copy, sizeof(block_size))
		.andReturnValue(result);
}

extern "C" psa_status_t rpmb_frontend_block_size(struct rpmb_frontend *context, size_t *block_size)
{
	return mock()
		.actualCall("rpmb_frontend_block_size")
		.onObject(context)
		.withOutputParameter("block_size", block_size)
		.returnIntValue();
}

void expect_rpmb_frontend_block_count(struct rpmb_frontend *context, size_t block_count,
				      psa_status_t result)
{
	static size_t bc_copy;

	bc_copy = block_count;
	mock().expectOneCall("rpmb_frontend_block_count")
		.onObject(context)
		.withOutputParameterReturning("block_count", &bc_copy, sizeof(block_count))
		.andReturnValue(result);
}

extern "C" psa_status_t rpmb_frontend_block_count(struct rpmb_frontend *context, size_t *block_count)
{
	return mock()
		.actualCall("rpmb_frontend_block_count")
		.onObject(context)
		.withOutputParameter("block_count", block_count)
		.returnIntValue();
}

void expect_rpmb_frontend_write(struct rpmb_frontend *context, uint16_t block_index,
				const uint8_t *data, size_t data_len, size_t block_count,
				psa_status_t result)
{
	mock().expectOneCall("rpmb_frontend_write")
		.onObject(context)
		.withUnsignedIntParameter("block_index", block_index)
		.withMemoryBufferParameter("data", data, data_len)
		.withUnsignedIntParameter("block_count", block_count)
		.andReturnValue(result);
}

extern "C" psa_status_t rpmb_frontend_write(struct rpmb_frontend *context, uint16_t block_index,
				 const uint8_t *data, size_t block_count)
{
	return mock()
		.actualCall("rpmb_frontend_write")
		.onObject(context)
		.withUnsignedIntParameter("block_index", block_index)
		//TODO get the block size from somewhere.
		.withMemoryBufferParameter("data", data, 256 * block_count)
		.withUnsignedIntParameter("block_count", block_count)
		.returnIntValue();
}

void expect_rpmb_frontend_read(struct rpmb_frontend *context, uint16_t block_index, uint8_t *data,
			       size_t data_len, size_t block_count, psa_status_t result)
{
	mock().expectOneCall("rpmb_frontend_read")
		.onObject(context)
		.withUnsignedIntParameter("block_index", block_index)
		.withOutputParameterReturning("data", data, data_len)
		.withUnsignedIntParameter("block_count", block_count)
		.andReturnValue(result);
}

extern "C" psa_status_t rpmb_frontend_read(struct rpmb_frontend *context, uint16_t block_index, uint8_t *data,
				size_t block_count)
{
	return mock()
		.actualCall("rpmb_frontend_read")
		.onObject(context)
		.withUnsignedIntParameter("block_index", block_index)
		.withOutputParameter("data", data)
		.withUnsignedIntParameter("block_count", block_count)
		.returnIntValue();
}
