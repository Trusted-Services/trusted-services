/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "block_store_factory.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "service/block_storage/block_store/device/ram/ram_block_store.h"
#include "service/block_storage/block_store/encrypted/encrypted_block_store.h"
#include "service/block_storage/block_store/partitioned/partitioned_block_store.h"
#include "service/block_storage/config/ref/ref_partition_configurator.h"

struct block_store_assembly {
	struct partitioned_block_store partitioned_block_store;
	struct encrypted_block_store encrypted_block_store;
	struct ram_block_store ram_block_store;
};

struct block_store *ref_encrypt_ram_block_store_factory_create(void)
{
	struct block_store *product = NULL;
	struct uuid_octets back_store_guid;
	struct block_store *back_store = NULL;
	struct block_store_assembly *assembly =
		(struct block_store_assembly *)calloc(1, sizeof(struct block_store_assembly));

	if (!assembly)
		return NULL;

	memset(&back_store_guid, 0, sizeof(back_store_guid));

	/* Initialise a ram_block_store to provide underlying storage */
	back_store = ram_block_store_init(&assembly->ram_block_store,
								&back_store_guid,
								REF_PARTITION_BACK_STORE_SIZE,
								REF_PARTITION_BLOCK_SIZE);

	/* Stack an encrypted block store over the partitioned one */
	product = encrypted_block_store_init(&assembly->encrypted_block_store, 0,
						&back_store_guid, back_store);

	if (!product) {
		ram_block_store_deinit(&assembly->ram_block_store);
		free(assembly);
		return NULL;
	}

	/* Stack a partitioned_block_store over the back store */
	product = partitioned_block_store_init(&assembly->partitioned_block_store, 0,
						&back_store_guid, product, NULL);

	if (!product) {
		encrypted_block_store_deinit(&assembly->encrypted_block_store);
		ram_block_store_deinit(&assembly->ram_block_store);
		free(assembly);
		return NULL;
	}

	/* Use the reference partition configuration */
	ref_partition_configure(&assembly->partitioned_block_store);

	return product;
}

void ref_encrypt_ram_block_store_factory_destroy(struct block_store *block_store)
{
	struct block_store_assembly *assembly = NULL;

	if (!block_store)
		return;

	assembly = (struct block_store_assembly *)((uint8_t *)block_store);

	partitioned_block_store_deinit(&assembly->partitioned_block_store);
	encrypted_block_store_deinit(&assembly->encrypted_block_store);
	ram_block_store_deinit(&assembly->ram_block_store);

	free(assembly);
}
