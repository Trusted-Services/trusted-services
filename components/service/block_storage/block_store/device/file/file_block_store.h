/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef FILE_BLOCK_STORE_H
#define FILE_BLOCK_STORE_H

#include <stdint.h>
#include <stdio.h>

#include "service/block_storage/block_store/device/block_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief file_block_store structure
 *
 * A file_block_store is a block_device that uses a file for storage.
 * The file represents a real storage device organized as a series of
 * consecutive blocks. The file_block_store can be used for accessing disk
 * image files in a Posix environment.
 */
struct file_block_store {
	struct block_device base_block_device;
	FILE *file_handle;
	uint8_t erase_buf[256];
};

/**
 * \brief Initialize a file_block_store
 *
 * \param[in]  file_block_store  The subject file_block_store
 * \param[in]  filename          The host filename used for storage
 * \param[in]  block_size        The storage block size
 *
 * \return Pointer to block_store or NULL on failure
 */
struct block_store *file_block_store_init(struct file_block_store *file_block_store,
					  const struct uuid_octets *disk_guid,
					  const char *filename, size_t block_size);

/**
 * \brief De-initialize a file_block_store
 *
 * \param[in]  file_block_store  The subject file_block_store
 */
void file_block_store_deinit(struct file_block_store *file_block_store);

/**
 * \brief Configure the file_block_store
 *
 * \param[in]  file_block_store The subject file_block_store
 * \param[in]  disk_guid        The disk GUID (nil uuid for any)
 * \param[in]  num_blocks       The number of contiguous blocks
 * \param[in]  block_size       Block size in bytes
 *
 * \return PSA_SUCCESS if successful
 */
psa_status_t file_block_store_configure(struct file_block_store *file_block_store,
					const struct uuid_octets *disk_guid, size_t num_blocks,
					size_t block_size);

#ifdef __cplusplus
}
#endif

#endif /* FILE_BLOCK_STORE_H */
