/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/**
 * A storage factory that creates storage backends that use the Secure Flash Store
 * (SFS) with a shared block store residing in a separate SP. Selection of the
 * storage partition to use for block storage is based on the requested security
 * class.
 */
#include <stdbool.h>
#include <stddef.h>
#include "service/secure_storage/backend/secure_flash_store/secure_flash_store.h"
#include "service/secure_storage/backend/secure_flash_store/flash/block_store_adapter/sfs_flash_block_store_adapter.h"
#include "service/secure_storage/factory/storage_factory.h"
#include "service/block_storage/block_store/block_store.h"
#include "service/block_storage/factory/client/block_store_factory.h"
#include "media/disk/guid.h"

/* Overridable SFS configuration */

/* Configures the minimum flash block size presented to SFS by the
 * sfs_flash_block_store_adapter. SFS limits the maximum file size to
 * the underlying block size. This configuration allows larger objects
 * to be stored by aggregating multiple storage blocks and presenting
 * them to SFS as a super block. */
/*
 * Current value is set to be able to store a 16k NV block with
 * metadata header for ms-tpm.
 */
#ifndef CONFIG_SFS_MIN_FLASH_BLOCK_SIZE
#define CONFIG_SFS_MIN_FLASH_BLOCK_SIZE		(5 * 4096)
#endif

/* Configures the maximum number of objects held by SFS */
#ifndef CONFIG_SFS_MAX_NUM_FILES
#define CONFIG_SFS_MAX_NUM_FILES			(10)
#endif

/* The storage backed specialization constructed by this factory */
struct sfs_shared_block_store
{
	struct sfs_flash_block_store_adapter sfs_flash_adapter;
	struct block_store *block_store;
	bool in_use;
};

/* Only supports construction of a single instance */
static struct sfs_shared_block_store storage_instance = { .in_use = false };

struct storage_backend *storage_factory_create(
	enum storage_factory_security_class security_class)
{
	struct storage_backend *storage_backend = NULL;

	if (storage_instance.in_use)
		return NULL;

	/* Create client_block_store that uses shared storage provided by
	 * the block storage service provider */
	storage_instance.block_store =
		client_block_store_factory_create("sn:ffa:63646e80-eb52-462f-ac4f-8cdf3987519c:0");

	if (storage_instance.block_store) {

		struct uuid_octets guid;
		const struct sfs_flash_info_t *flash_info = NULL;

		if (security_class == storage_factory_security_class_INTERNAL_TRUSTED)
			uuid_guid_octets_from_canonical(&guid, DISK_GUID_UNIQUE_PARTITION_PSA_ITS);
		else
			uuid_guid_octets_from_canonical(&guid, DISK_GUID_UNIQUE_PARTITION_PSA_PS);

		psa_status_t status = sfs_flash_block_store_adapter_init(
			&storage_instance.sfs_flash_adapter,
			0,
			storage_instance.block_store,
			&guid,
			CONFIG_SFS_MIN_FLASH_BLOCK_SIZE,
			CONFIG_SFS_MAX_NUM_FILES,
			&flash_info);

		if (status == PSA_SUCCESS) {

			storage_backend = sfs_init(flash_info);
		}

		if ((status != PSA_SUCCESS) || !storage_backend) {

			client_block_store_factory_destroy(storage_instance.block_store);
			storage_instance.block_store = NULL;
		}
	}

	storage_instance.in_use = (storage_instance.block_store != NULL);

	return storage_backend;
}

void storage_factory_destroy(struct storage_backend *backend)
{
	if (backend && storage_instance.in_use) {

		sfs_flash_block_store_adapter_deinit(
			&storage_instance.sfs_flash_adapter);

		client_block_store_factory_destroy(storage_instance.block_store);
		storage_instance.block_store = NULL;

		storage_instance.in_use = false;
	}
}

