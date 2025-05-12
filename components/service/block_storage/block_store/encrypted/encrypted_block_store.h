/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef ENCRYPTED_BLOCK_STORE_H
#define ENCRYPTED_BLOCK_STORE_H

#include <stdbool.h>

#include "psa/crypto.h"
#include "service/block_storage/block_store/block_store.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief encrypted_block_store structure
 *
 * A encrypted_block_store is a Stacked Block Store which encrypts the data to be
 * written to the store and decrypts the read data before providing it. This encryption
 * aims to provide data at rest and data in transit protection to prevent eavesdropping
 * if an attacker gains access to memory buffers or to the storage media
 * (e.g removal of eMMC or bus-sniffing).
 */
struct encrypted_block_store {
	struct block_store base_block_store;
	uint32_t local_client_id;
	storage_partition_handle_t back_store_handle;
	struct block_store *back_store;
	struct storage_partition_info back_store_info;
	psa_key_id_t data_encryption_key_id;
	psa_key_id_t essiv_key_id;
	/* Buffers for encryption and decryption capable of storing a single block of data */
	uint8_t *block_buffer_A;
	uint8_t *block_buffer_B;
};

/**
 * \brief Initialize an encrypted_block_store
 *
 * \param[in]  encrypted_block_store  	The subject encrypted_block_store
 * \param[in]  local_client_id   	Client ID corresponding to the current environment
 * \param[in]  back_store_guid   	The partition GUID to use in the underlying back store
 * \param[in]  back_store		The associated back store
 *
 * \return Pointer to block_store or NULL on failure
 */
struct block_store *encrypted_block_store_init(struct encrypted_block_store *encrypted_block_store,
					       uint32_t local_client_id,
					       const struct uuid_octets *back_store_guid,
					       struct block_store *back_store);

/**
 * \brief De-initialize an encrypted_block_store
 *
 *  Frees resources allocated during call to encrypted_block_store_init().
 *
 * \param[in]  encrypted_block_store  	The subject encrypted_block_store
 */
void encrypted_block_store_deinit(struct encrypted_block_store *encrypted_block_store);

#ifdef __cplusplus
}
#endif

#endif /* ENCRYPTED_BLOCK_STORE_H */
