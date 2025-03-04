/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "rpmb_block_store.h"
#include "util.h"

static psa_status_t rpmb_block_store_get_partition_info(void *context,
							const struct uuid_octets *partition_guid,
							struct storage_partition_info *info)
{
	struct rpmb_block_store *block_store = (struct rpmb_block_store *)context;
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;

	status = block_device_get_partition_info(&block_store->base_block_device, partition_guid,
						 info);

	return status;
}

static psa_status_t rpmb_block_store_open(void *context, uint32_t client_id,
					  const struct uuid_octets *partition_guid,
					  storage_partition_handle_t *handle)
{
	struct rpmb_block_store *block_store = (struct rpmb_block_store *)context;

	if (!block_store)
		return PSA_ERROR_INVALID_ARGUMENT;

	return block_device_open(&block_store->base_block_device, client_id, partition_guid,
				 handle);
}

static psa_status_t rpmb_block_store_close(void *context, uint32_t client_id,
					   storage_partition_handle_t handle)
{
	struct rpmb_block_store *block_store = (struct rpmb_block_store *)context;

	if (!block_store)
		return PSA_ERROR_INVALID_ARGUMENT;

	return block_device_close(&block_store->base_block_device, client_id, handle);
}

static psa_status_t rpmb_block_store_read(void *context, uint32_t client_id,
					  storage_partition_handle_t handle, uint64_t lba,
					  size_t offset, size_t buffer_size, uint8_t *buffer,
					  size_t *data_len)
{
	struct rpmb_block_store *block_store = (struct rpmb_block_store *)context;
	const struct storage_partition *storage_partition = NULL;
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;
	size_t copy_length = 0;
	uint64_t device_lba = 0;

	if (!block_store)
		return PSA_ERROR_INVALID_ARGUMENT;

	if (offset >= block_store->logical_block_size)
		return PSA_ERROR_INVALID_ARGUMENT;

	status = block_device_check_access_permitted(&block_store->base_block_device, client_id,
						     handle);
	if (status != PSA_SUCCESS)
		return status;

	storage_partition = &block_store->base_block_device.storage_partition;

	*data_len = 0;

	/* Calculate the lba and offset of the device */
	if (MUL_OVERFLOW(lba, block_store->block_ratio, &device_lba))
		return PSA_ERROR_GENERIC_ERROR;

	/* Clear the buffer */
	memset(block_store->block_buffer, 0, block_store->logical_block_size);

	/*
	 * If the end_lbs is accessible, then all LBAs should be ok.
	 */
	if (!storage_partition_is_lba_legal(storage_partition, lba))
		return PSA_ERROR_INVALID_ARGUMENT;

	status = rpmb_frontend_read(block_store->frontend, device_lba,
				    block_store->block_buffer,
				    block_store->block_ratio);
	if (status != PSA_SUCCESS)
		return status;

	copy_length = MIN(buffer_size, block_store->logical_block_size - offset);
	memcpy(buffer, block_store->block_buffer + offset, copy_length);

	*data_len = copy_length;

	return status;
}

static psa_status_t rpmb_block_store_write(void *context, uint32_t client_id,
					   storage_partition_handle_t handle, uint64_t lba,
					   size_t offset, const uint8_t *data, size_t data_len,
					   size_t *num_written)
{
	struct rpmb_block_store *block_store = (struct rpmb_block_store *)context;
	const struct storage_partition *storage_partition = NULL;
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;
	size_t copy_length = 0;
	size_t device_lba = 0;

	if (!block_store)
		return PSA_ERROR_INVALID_ARGUMENT;

	if (offset >= block_store->logical_block_size)
		return PSA_ERROR_INVALID_ARGUMENT;

	status = block_device_check_access_permitted(&block_store->base_block_device, client_id,
						     handle);
	if (status != PSA_SUCCESS)
		return status;

	storage_partition = &block_store->base_block_device.storage_partition;

	/* Calculate the lba and offset of the device */
	if (MUL_OVERFLOW(lba, block_store->block_ratio, &device_lba))
		return PSA_ERROR_GENERIC_ERROR;

	/* Clear the buffer */
	memset(block_store->block_buffer, 0, block_store->logical_block_size);

	/*
	 * If the last LBS is ok, all should be.
	 */
	if (!storage_partition_is_lba_legal(storage_partition, lba))
		return PSA_ERROR_INVALID_ARGUMENT;

	copy_length = MIN(data_len, block_store->logical_block_size - offset);
	if (copy_length != block_store->logical_block_size) {
		status = rpmb_frontend_read(block_store->frontend, device_lba,
					    block_store->block_buffer, block_store->block_ratio);
		if (status != PSA_SUCCESS)
			return status;
	}

	memcpy(block_store->block_buffer + offset, data, copy_length);

	status = rpmb_frontend_write(block_store->frontend, device_lba,
				     block_store->block_buffer, block_store->block_ratio);
	if (status != PSA_SUCCESS)
		return status;

	*num_written = copy_length;

	return status;
}

static psa_status_t rpmb_block_store_erase(void *context, uint32_t client_id,
					   storage_partition_handle_t handle, uint64_t begin_lba,
					   size_t num_blocks)
{
	(void)context;
	(void)client_id;
	(void)handle;
	(void)begin_lba;
	(void)num_blocks;

	return PSA_SUCCESS;
}

struct block_store *rpmb_block_store_init(struct rpmb_block_store *block_store,
					  const struct uuid_octets *disk_guid,
					  struct rpmb_frontend *frontend, size_t logical_block_size)
{
	static const struct block_store_interface interface = {
		rpmb_block_store_get_partition_info,
		rpmb_block_store_open,
		rpmb_block_store_close,
		rpmb_block_store_read,
		rpmb_block_store_write,
		rpmb_block_store_erase
	};
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;
	size_t num_blocks = 0;
	struct block_store *store = NULL;

	block_store->frontend = frontend;
	block_store->base_block_device.base_block_store.context = block_store;
	block_store->base_block_device.base_block_store.interface = &interface;

	status = rpmb_frontend_block_count(frontend, &num_blocks);
	if (status != PSA_SUCCESS)
		return NULL;

	status = rpmb_frontend_block_size(frontend, &block_store->block_size);
	if (status != PSA_SUCCESS)
		return NULL;

	/*
	 * The block size of the underlying device and the requested logical
	 * block must be power of 2 (which includes being non-zero).
	 */
	if (!IS_POWER_OF_TWO(logical_block_size) || !IS_POWER_OF_TWO(block_store->block_size))
		return NULL;

	/* Both being power of two, and non-zero means the division will be fractionless */
	block_store->block_ratio = logical_block_size / block_store->block_size;
	if (!block_store->block_ratio)
		return NULL;

	block_store->logical_block_size = logical_block_size;

	store = block_device_init(&block_store->base_block_device, disk_guid,
				  num_blocks / block_store->block_ratio,
				  block_store->logical_block_size);

	if (!store)
		return NULL;

	block_store->block_buffer =
		(uint8_t *)calloc(1, logical_block_size);

	if (!block_store->block_buffer) {
		block_device_deinit(&block_store->base_block_device);
		return NULL;
	}

	return store;
}

void rpmb_block_store_deinit(struct rpmb_block_store *block_store)
{
	/* TODO: close session */
	block_device_deinit(&block_store->base_block_device);

	free(block_store->block_buffer);
	block_store->block_buffer = NULL;
}
