/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef MOCK_BLOCK_STORE_H
#define MOCK_BLOCK_STORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "psa/crypto.h"
#include "service/block_storage/block_store/block_store.h"

/**
 * \brief mock_block_store structure
 *
 * A mock_block_store is a block_device that is only used for unit testing
 */
struct mock_block_store
{
	struct block_store base_block_store;
};

void expect_block_store_get_partition_info(void *context,
					   const struct uuid_octets *partition_guid,
					   struct storage_partition_info *info,
					   psa_status_t result);

void expect_block_store_open(void *context, uint32_t client_id,
			     const struct uuid_octets *partition_guid,
			     storage_partition_handle_t *handle, psa_status_t result);

void expect_block_store_close(void *context, uint32_t client_id,
			      storage_partition_handle_t handle, psa_status_t result);

void expect_block_store_read(void *context, uint32_t client_id,
			     storage_partition_handle_t handle, uint64_t lba, size_t offset,
			     size_t buffer_size, uint8_t *buffer, size_t *data_len,
			     psa_status_t result);

void expect_block_store_write(void *context, uint32_t client_id,
			      storage_partition_handle_t handle, uint64_t lba, size_t offset,
			      const uint8_t *data, size_t data_len, size_t *num_written,
			      psa_status_t result);

void expect_block_store_erase(void *context, uint32_t client_id,
			      storage_partition_handle_t handle, uint64_t begin_lba,
			      size_t num_blocks, psa_status_t result);

/**
 * \brief Initialize a mock_block_store
 *
 * \param[in]  mock_block_store  The subject mock_block_store
 * \param[in]  disk_guid   	 The disk GUID (nil uuid for any)
 * \param[in]  num_blocks        The number of contiguous blocks
 * \param[in]  block_size        Block size in bytes
 *
 * \return Pointer to block_store or NULL on failure
 */
struct block_store *mock_block_store_init(
	struct mock_block_store *mock_block_store,
	const struct uuid_octets *disk_guid,
	size_t num_blocks,
	size_t block_size);

#ifdef __cplusplus
}
#endif

#endif /* MOCK_BLOCK_STORE_H */
