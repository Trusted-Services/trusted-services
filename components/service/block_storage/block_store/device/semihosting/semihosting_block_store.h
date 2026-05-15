/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef SEMIHOSTING_BLOCK_STORE_H
#define SEMIHOSTING_BLOCK_STORE_H

#include <stdint.h>
#include "service/block_storage/block_store/device/block_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief semihosting_block_store structure
 *
 * A semihosting_block_store is a block_device that can be used on virtual
 * platforms with semihosting capability to store blocks in a file residing
 * in the host machine's filesystem. The file contains the raw block data
 * which may include a partition header such as a GPT. The semihosting_block_store
 * is agnostic about the file contents.
 */
struct semihosting_block_store
{
	struct block_device base_block_device;
	long file_handle;
	uint8_t erase_buf[256];
};

/**
 * \brief Initialize a semihosting_block_store
 *
 * \param[in]  semihosting_block_store  The subject semihosting_block_store
 * \param[in]  disk_guid                The disk GUID (can be NULL)
 * \param[in]  filename                 The host filename used for storage
 * \param[in]  block_size               The storage block size
 *
 * \return Pointer to block_store or NULL on failure
 */
struct block_store *semihosting_block_store_init(
	struct semihosting_block_store *semihosting_block_store,
	const struct uuid_octets *disk_guid,
	const char *filename,
	size_t block_size);

/**
 * \brief De-initialize a semihosting_block_store
 *
 * \param[in]  semihosting_block_store  The subject semihosting_block_store
 */
void semihosting_block_store_deinit(
	struct semihosting_block_store *semihosting_block_store);

/**
 * \brief Configure the semihosting_block_store
 *
 * \param[in]  semihosting_block_store  The subject semihosting_block_store
 * \param[in]  disk_guid   		The disk GUID (nil uuid for any)
 * \param[in]  num_blocks       The number of contiguous blocks
 * \param[in]  block_size       Block size in bytes
 *
 * \return PSA_SUCCESS if successful
 */
psa_status_t semihosting_block_store_configure(
	struct semihosting_block_store *semihosting_block_store,
	const struct uuid_octets *disk_guid,
	size_t num_blocks,
	size_t block_size);

/**
 * \brief Wipe contents of the block store
 *
 * Test support function to wipe the contents of the block store file.
 *
 * \param[in]  filename			The host filename used for storage
 */
void semihosting_block_store_wipe(
	const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* SEMIHOSTING_BLOCK_STORE_H */
