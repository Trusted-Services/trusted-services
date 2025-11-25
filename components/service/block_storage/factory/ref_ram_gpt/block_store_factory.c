/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <service/block_storage/block_store/device/ram/ram_block_store.h>
#include <service/block_storage/block_store/partitioned/partitioned_block_store.h>
#include <service/block_storage/config/ref/ref_partition_configurator.h>
#include <service/block_storage/config/gpt/gpt_partition_configurator.h>
#include <media/volume/index/volume_index.h>
#include <media/volume/block_volume/block_volume.h>
#include <media/disk/disk_images/ref_partition.h>
#include <media/disk/formatter/disk_formatter.h>
#include "block_store_factory.h"
#include <trace.h>

struct block_store_assembly
{
	struct ram_block_store ram_block_store;
	struct partitioned_block_store partitioned_block_store;
	struct block_volume volume;
};

static void tear_down_assembly(struct block_store_assembly *assembly)
{
	volume_index_clear();

	partitioned_block_store_deinit(&assembly->partitioned_block_store);
	ram_block_store_deinit(&assembly->ram_block_store);
	block_volume_deinit(&assembly->volume);

	free(assembly);
}

struct block_store *ref_ram_gpt_block_store_factory_create(void)
{
	struct block_store *product = NULL;
	struct block_store_assembly *assembly =
		(struct block_store_assembly*)malloc(sizeof(struct block_store_assembly));

	if (assembly) {

		struct uuid_octets disk_guid;
		memset(&disk_guid, 0, sizeof(disk_guid));

		volume_index_init();

		/* Reference partition must be multiple of the block size */
		if (ref_partition_data_length % REF_PARTITION_BLOCK_SIZE) {
			EMSG("Reference partition size is not multiple of the block size");
			return NULL;
		}

		/* Initialise a ram_block_store to mimic the secure flash used
		 * to provide underlying storage.
		 */
		struct block_store *secure_flash = ram_block_store_init(
			&assembly->ram_block_store,
			&disk_guid,
			ref_partition_data_length / REF_PARTITION_BLOCK_SIZE,
			REF_PARTITION_BLOCK_SIZE);

		if (secure_flash) {

			/* Secure flash successfully initialized so create a block_volume to
			 * enable it to be accessed as a storage volume. The created io_dev is
			 * used to initialize the flash with the reference disk image.
			 */
			struct volume *volume = NULL;

			if (!block_volume_init(&assembly->volume,
					secure_flash, &disk_guid, &volume) &&
				!disk_formatter_clone(
					volume->dev_handle, volume->io_spec,
					ref_partition_data, ref_partition_data_length)) {

				volume_index_add(VOLUME_ID_SECURE_FLASH, volume);

				/* Stack a partitioned_block_store over the back store */
				product = partitioned_block_store_init(
					&assembly->partitioned_block_store, 0,
					&disk_guid,
					secure_flash,
					NULL);

				if (product) {

					/* Successfully created the block store stack so configure the
					 * partitions if there are any described in the GPT.
					 */
					gpt_partition_configure(
						&assembly->partitioned_block_store,
						VOLUME_ID_SECURE_FLASH);
				}
			}
		}

		if (!product) {

			/* Something went wrong! */
			tear_down_assembly(assembly);
		}
	}

	return product;
}

void ref_ram_gpt_block_store_factory_destroy(struct block_store *block_store)
{
	if (block_store) {

		size_t offset_into_assembly =
			offsetof(struct block_store_assembly, partitioned_block_store) +
			offsetof(struct partitioned_block_store, base_block_store);

		struct block_store_assembly *assembly = (struct block_store_assembly*)
			((uint8_t*)block_store - offset_into_assembly);

		tear_down_assembly(assembly);
	}
}
