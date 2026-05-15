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
#include <trace.h>
#include "block_store_factory.h"
#include "service/block_storage/block_store/device/semihosting/semihosting_block_store.h"
#include "service/block_storage/block_store/partitioned/partitioned_block_store.h"
#include "service/block_storage/config/gpt/gpt_partition_configurator.h"
#include "media/volume/index/volume_index.h"
#include "media/volume/block_volume/block_volume.h"

/* Most common block size for UEFI volumes */
#define SEMIHOSTING_BLOCK_SIZE		(512)

struct block_store_assembly
{
	struct semihosting_block_store semihosting_block_store;
	struct partitioned_block_store partitioned_block_store;
	struct block_volume volume;
};

static void tear_down_assembly(struct block_store_assembly *assembly)
{
	volume_index_clear();

	partitioned_block_store_deinit(&assembly->partitioned_block_store);
	semihosting_block_store_deinit(&assembly->semihosting_block_store);
	block_volume_deinit(&assembly->volume);

	free(assembly);
}

struct block_store *semihosting_block_store_factory_create(void)
{
	struct block_store *product = NULL;
	struct block_store_assembly *assembly =
		(struct block_store_assembly*)malloc(sizeof(struct block_store_assembly));

	if (assembly) {

		struct uuid_octets disk_guid;
		memset(&disk_guid, 0, sizeof(disk_guid));

		volume_index_init();

		/* Initialise a semihosting_block_store to provide underlying storage */
		struct block_store *secure_flash = semihosting_block_store_init(
			&assembly->semihosting_block_store,
			&disk_guid,
			"secure-flash.img",
			SEMIHOSTING_BLOCK_SIZE);

		if (secure_flash) {

			/* Secure flash successfully initialized so create a block_volume
			 * associated with secure flash block store to enable it to be
			 * accessed as a storage volume.
			 */
			struct volume *volume = NULL;

			int result = block_volume_init(&assembly->volume,
				secure_flash, &disk_guid,
				&volume);

			if (result == 0) {

				volume_index_add(VOLUME_ID_SECURE_FLASH, volume);

				/* Stack a partitioned_block_store over the back store */
				product = partitioned_block_store_init(
					&assembly->partitioned_block_store,
					0,
					&disk_guid,
					secure_flash,
					NULL);

				if (product) {

					/* Successfully created the block store stack so configure the
					 * partitions if there are any described in the GPT. No GPT
					 * is a valid configuration option so it's deliberately not
					 * treated as an error. To help a platform integrator who
					 * intended to use a GPT, a message is output if partitions
					 * weren't configured.
					 * */
					bool is_configured = gpt_partition_configure(
						&assembly->partitioned_block_store,
						VOLUME_ID_SECURE_FLASH);

					if (!is_configured)
						IMSG("No GPT detected\n");
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

void semihosting_block_store_factory_destroy(struct block_store *block_store)
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
