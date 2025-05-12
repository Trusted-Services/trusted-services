/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef REF_ENCRYPT_RAM_BLOCK_STORE_FACTORY_H
#define REF_ENCRYPT_RAM_BLOCK_STORE_FACTORY_H

#include "service/block_storage/block_store/block_store.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A block store factory that constructs an encrypted, partitioned block store
 * with a ram back store. The reference partition configuration is used to
 * provide a set of storage partitions that match the expectations of
 * test cases. This factory should only be used for test deployments.
 */

/**
 * \brief Factory method to create a block_store
 *
 * \return A pointer to the constructed block_store (NULL on failure)
 */
struct block_store *ref_encrypt_ram_block_store_factory_create(void);

/**
 * \brief Destroys a block_store created with block_store_factory_create
 *
 * \param[in] block_store    The block store to destroy
 */
void ref_encrypt_ram_block_store_factory_destroy(struct block_store *block_store);

#ifdef __cplusplus
}
#endif

#endif /* REF_ENCRYPT_RAM_BLOCK_STORE_FACTORY_H */
