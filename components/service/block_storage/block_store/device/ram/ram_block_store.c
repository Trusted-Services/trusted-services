/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "ram_block_store.h"


#define RAM_BLOCK_STORE_ERASED_VALUE	    (0xff)

static psa_status_t ram_block_store_get_partition_info(void *context,
	const struct uuid_octets *partition_guid,
	struct storage_partition_info *info)
{
	struct ram_block_store *ram_block_store = (struct ram_block_store*)context;
	return block_device_get_partition_info(
		&ram_block_store->base_block_device, partition_guid, info);
}

static psa_status_t ram_block_store_open(void *context,
	uint32_t client_id,
	const struct uuid_octets *partition_guid,
	storage_partition_handle_t *handle)
{
	struct ram_block_store *ram_block_store = (struct ram_block_store*)context;
	return block_device_open(
		&ram_block_store->base_block_device, client_id, partition_guid, handle);
}

static psa_status_t ram_block_store_close(void *context,
	uint32_t client_id,
	storage_partition_handle_t handle)
{
	struct ram_block_store *ram_block_store = (struct ram_block_store*)context;
	return block_device_close(
		&ram_block_store->base_block_device, client_id, handle);
}

static psa_status_t ram_block_store_read(void *context,
	uint32_t client_id,
	storage_partition_handle_t handle,
	uint64_t lba,
	size_t offset,
	size_t buffer_size,
	uint8_t *buffer,
	size_t *data_len)
{
	struct ram_block_store *ram_block_store = (struct ram_block_store*)context;
	psa_status_t status = block_device_check_access_permitted(
		&ram_block_store->base_block_device, client_id, handle);

	if (status == PSA_SUCCESS) {

		const struct storage_partition *storage_partition =
			&ram_block_store->base_block_device.storage_partition;

		if (storage_partition_is_lba_legal(storage_partition, lba) &&
			(offset < storage_partition->block_size)) {

			size_t bytes_remaining = storage_partition->block_size - offset;
			size_t bytes_to_read = (buffer_size < bytes_remaining) ?
				buffer_size :
				bytes_remaining;

			const uint8_t *block_start =
				&ram_block_store->ram_back_store[lba * storage_partition->block_size];

			memcpy(buffer, &block_start[offset], bytes_to_read);
			*data_len = bytes_to_read;
		}
		else {

			status = PSA_ERROR_INVALID_ARGUMENT;
		}
	}

	return status;
}

static psa_status_t ram_block_store_write(void *context,
	uint32_t client_id,
	storage_partition_handle_t handle,
	uint64_t lba,
	size_t offset,
	const uint8_t *data,
	size_t data_len,
	size_t *num_written)
{
	struct ram_block_store *ram_block_store = (struct ram_block_store*)context;
	psa_status_t status = block_device_check_access_permitted(
		&ram_block_store->base_block_device, client_id, handle);

	if (status == PSA_SUCCESS) {

		const struct storage_partition *storage_partition =
			&ram_block_store->base_block_device.storage_partition;

		if (storage_partition_is_lba_legal(storage_partition, lba) &&
			(offset < storage_partition->block_size)) {

			size_t bytes_remaining = storage_partition->block_size - offset;
			uint8_t *block_start =
				&ram_block_store->ram_back_store[lba * storage_partition->block_size];

			size_t bytes_to_write = (data_len < bytes_remaining) ?
				data_len :
				bytes_remaining;

			memcpy(&block_start[offset], data, bytes_to_write);
			*num_written = bytes_to_write;
		}
		else {

			status = PSA_ERROR_INVALID_ARGUMENT;
		}
	}

	return status;
}

static psa_status_t ram_block_store_erase(void *context,
	uint32_t client_id,
	storage_partition_handle_t handle,
	uint64_t begin_lba,
	size_t num_blocks)
{
	struct ram_block_store *ram_block_store = (struct ram_block_store*)context;
	const struct storage_partition *storage_partition =
		&ram_block_store->base_block_device.storage_partition;
	psa_status_t status = block_device_check_access_permitted(
		&ram_block_store->base_block_device, client_id, handle);

	/* Sanitize the range of LBAs to erase */
	if ((status == PSA_SUCCESS) &&
		!storage_partition_is_lba_legal(storage_partition, begin_lba)) {

		status = PSA_ERROR_INVALID_ARGUMENT;
	}

	if (status == PSA_SUCCESS) {

		size_t blocks_to_erase = storage_partition_clip_num_blocks(storage_partition,
			begin_lba, num_blocks);

		uint8_t *erase_from =
			&ram_block_store->ram_back_store[begin_lba * storage_partition->block_size];
		size_t erase_len =
			blocks_to_erase * storage_partition->block_size;

		memset(erase_from, RAM_BLOCK_STORE_ERASED_VALUE, erase_len);
	}

	return status;
}

struct block_store *ram_block_store_init(
	struct ram_block_store *ram_block_store,
	const struct uuid_octets *disk_guid,
	size_t num_blocks,
	size_t block_size)
{
	struct block_store *retval = NULL;

	/* Define concrete block store interface */
	static const struct block_store_interface interface =
	{
		ram_block_store_get_partition_info,
		ram_block_store_open,
		ram_block_store_close,
		ram_block_store_read,
		ram_block_store_write,
		ram_block_store_erase
	};

	/* Publish the public interface */
	ram_block_store->base_block_device.base_block_store.context = ram_block_store;
	ram_block_store->base_block_device. base_block_store.interface = &interface;

	/* Allocate storage and set all to the erased state */
	size_t back_store_size = num_blocks * block_size;
	ram_block_store->ram_back_store = (uint8_t*)malloc(back_store_size);

	if (ram_block_store->ram_back_store) {

		memset(ram_block_store->ram_back_store, RAM_BLOCK_STORE_ERASED_VALUE, back_store_size);

		retval = block_device_init(
			&ram_block_store->base_block_device, disk_guid, num_blocks, block_size);
	}

	return retval;
}

void ram_block_store_deinit(
	struct ram_block_store *ram_block_store)
{
	free(ram_block_store->ram_back_store);

	block_device_deinit(&ram_block_store->base_block_device);
}

psa_status_t ram_block_store_modify(
	struct ram_block_store *ram_block_store,
	size_t offset,
	const uint8_t *data,
	size_t data_len)
{
	const struct storage_partition *storage_partition =
		&ram_block_store->base_block_device.storage_partition;

	size_t back_store_size = storage_partition->block_size * storage_partition->num_blocks;

	if (offset < back_store_size) {

		size_t write_len = (offset + data_len < back_store_size) ?
			data_len : back_store_size - offset;

		memcpy(&ram_block_store->ram_back_store[offset], data, write_len);
	} else
		return PSA_ERROR_INVALID_ARGUMENT;

	return PSA_SUCCESS;
}
