/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <string.h>
#include "mock_block_store.h"

#include <CppUTestExt/MockSupport.h>

void expect_block_store_get_partition_info(void *context,
					   const struct uuid_octets *partition_guid,
					   struct storage_partition_info *info, psa_status_t result)
{
	mock().expectOneCall("block_store_get_partition_info")
		.onObject(context)
		.withConstPointerParameter("partition_guid", partition_guid)
		.withOutputParameterReturning("info", info, sizeof(*info))
		.andReturnValue(result);
}

static psa_status_t mock_block_store_get_partition_info(void *context,
					    const struct uuid_octets *partition_guid,
					    struct storage_partition_info *info)
{
	return mock()
		.actualCall("block_store_get_partition_info")
		.onObject(context)
		.withConstPointerParameter("partition_guid", partition_guid)
		.withOutputParameter("info", info)
		.returnIntValue();
}

void expect_block_store_open(void *context, uint32_t client_id,
			     const struct uuid_octets *partition_guid,
			     storage_partition_handle_t *handle, psa_status_t result)
{
	mock().expectOneCall("block_store_open")
		.onObject(context)
		.withUnsignedIntParameter("client_id", client_id)
		.withConstPointerParameter("partition_guid", partition_guid)
		.withOutputParameterReturning("handle", handle, sizeof(*handle))
		.andReturnValue(result);
}

static psa_status_t mock_block_store_open(void *context, uint32_t client_id,
			      const struct uuid_octets *partition_guid,
			      storage_partition_handle_t *handle)
{
	return mock()
		.actualCall("block_store_open")
		.onObject(context)
		.withUnsignedIntParameter("client_id", client_id)
		.withConstPointerParameter("partition_guid", partition_guid)
		.withOutputParameter("handle", handle)
		.returnIntValue();
}

void expect_block_store_close(void *context, uint32_t client_id,
			      storage_partition_handle_t handle, psa_status_t result)
{
	mock().expectOneCall("block_store_close")
		.onObject(context)
		.withUnsignedIntParameter("client_id", client_id)
		.withUnsignedIntParameter("handle", handle)
		.andReturnValue(result);
}

static psa_status_t mock_block_store_close(void *context, uint32_t client_id,
			       storage_partition_handle_t handle)
{
	return mock()
		.actualCall("block_store_close")
		.onObject(context)
		.withUnsignedIntParameter("client_id", client_id)
		.withUnsignedIntParameter("handle", handle)
		.returnIntValue();
}

void expect_block_store_read(void *context, uint32_t client_id,
			     storage_partition_handle_t handle, uint64_t lba, size_t offset,
			     size_t buffer_size, uint8_t *buffer, size_t *data_len,
			     psa_status_t result)
{
	mock().expectOneCall("block_store_read")
		.onObject(context)
		.withUnsignedIntParameter("client_id", client_id)
		.withUnsignedIntParameter("handle", handle)
		.withUnsignedIntParameter("lba", lba)
		.withUnsignedIntParameter("offset", offset)
		.withUnsignedIntParameter("buffer_size", buffer_size)
		.withMemoryBufferParameter("buffer", buffer, buffer_size)
		.withOutputParameterReturning("data_len", data_len, sizeof(*data_len))
		.andReturnValue(result);
}

static psa_status_t mock_block_store_read(void *context, uint32_t client_id,
			      storage_partition_handle_t handle, uint64_t lba, size_t offset,
			      size_t buffer_size, uint8_t *buffer, size_t *data_len)
{
	return mock()
		.actualCall("block_store_read")
		.onObject(context)
		.withUnsignedIntParameter("client_id", client_id)
		.withUnsignedIntParameter("handle", handle)
		.withUnsignedIntParameter("lba", lba)
		.withUnsignedIntParameter("offset", offset)
		.withUnsignedIntParameter("buffer_size", buffer_size)
		.withOutputParameter("buffer", buffer)
		.withOutputParameter("data_len", data_len)
		.returnIntValue();
}

void expect_block_store_write(void *context, uint32_t client_id,
			      storage_partition_handle_t handle, uint64_t lba, size_t offset,
			      const uint8_t *data, size_t data_len, size_t *num_written,
			      psa_status_t result)
{
	mock().expectOneCall("block_store_write")
		.onObject(context)
		.withUnsignedIntParameter("client_id", client_id)
		.withUnsignedIntParameter("handle", handle)
		.withUnsignedIntParameter("lba", lba)
		.withUnsignedIntParameter("offset", offset)
		.withMemoryBufferParameter("data", data, data_len)
		.withUnsignedIntParameter("data_len", data_len)
		.withOutputParameterReturning("num_written", num_written, sizeof(*num_written))
		.andReturnValue(result);
}

static psa_status_t mock_block_store_write(void *context, uint32_t client_id,
			       storage_partition_handle_t handle, uint64_t lba, size_t offset,
			       const uint8_t *data, size_t data_len, size_t *num_written)
{
	return mock()
		.actualCall("block_store_write")
		.onObject(context)
		.withUnsignedIntParameter("client_id", client_id)
		.withUnsignedIntParameter("handle", handle)
		.withUnsignedIntParameter("lba", lba)
		.withUnsignedIntParameter("offset", offset)
		.withMemoryBufferParameter("data", data, data_len)
		.withUnsignedIntParameter("data_len", data_len)
		.withOutputParameter("num_written", num_written)
		.returnIntValue();
}

void expect_block_store_erase(void *context, uint32_t client_id,
			      storage_partition_handle_t handle, uint64_t begin_lba,
			      size_t num_blocks, psa_status_t result)
{
	mock().expectOneCall("block_store_erase")
		.onObject(context)
		.withUnsignedIntParameter("client_id", client_id)
		.withUnsignedIntParameter("handle", handle)
		.withUnsignedLongIntParameter("begin_lba", begin_lba)
		.withUnsignedIntParameter("num_blocks", num_blocks)
		.andReturnValue(result);
}

static psa_status_t mock_block_store_erase(void *context, uint32_t client_id,
			       storage_partition_handle_t handle, uint64_t begin_lba,
			       size_t num_blocks)
{
	return mock()
		.actualCall("block_store_erase")
		.onObject(context)
		.withUnsignedIntParameter("client_id", client_id)
		.withUnsignedIntParameter("handle", handle)
		.withUnsignedLongIntParameter("begin_lba", begin_lba)
		.withUnsignedIntParameter("num_blocks", num_blocks)
		.returnIntValue();
}


struct block_store *mock_block_store_init(
	struct mock_block_store *mock_block_store,
	const struct uuid_octets *disk_guid,
	size_t num_blocks,
	size_t block_size)
{
	/* Define concrete block store interface */
	static const struct block_store_interface interface =
	{
		mock_block_store_get_partition_info,
		mock_block_store_open,
		mock_block_store_close,
		mock_block_store_read,
		mock_block_store_write,
		mock_block_store_erase
	};

	mock_block_store->base_block_store.context = mock_block_store;
	mock_block_store->base_block_store.interface = &interface;

	return &mock_block_store->base_block_store;
}
