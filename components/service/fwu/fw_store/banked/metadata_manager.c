/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "metadata_manager.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common/crc32/crc32.h"
#include "media/volume/index/volume_index.h"
#include "media/volume/volume.h"
#include "protocols/service/fwu/status.h"
#include "service/fwu/fw_store/banked/metadata_serializer/metadata_serializer.h"
#include "trace.h"
#include "volume_id.h"

static int load_and_check_metadata(struct volume *volume, uint8_t *buf, size_t expected_len)
{
	int status = FWU_STATUS_UNKNOWN;

	status = volume_open(volume);

	if (status == 0) {
		size_t len = 0;

		status = volume_read(volume, (uintptr_t)buf, expected_len, &len);

		if (status == 0) {
			uint32_t calc_crc =
				crc32(0U, buf + sizeof(uint32_t), expected_len - sizeof(uint32_t));

			uint32_t expected_crc = *((uint32_t *)buf);

			status = (calc_crc == expected_crc) ? FWU_STATUS_SUCCESS :
							      FWU_STATUS_UNKNOWN;
		}

		volume_close(volume);
	}

	return status;
}

static int store_metadata(struct volume *volume, const uint8_t *data, size_t data_len)
{
	int status = volume_open(volume);

	if (status == 0) {
		status = volume_erase(volume);

		if (status == 0) {
			size_t written_len = 0;

			status =
				volume_write(volume, (const uintptr_t)data, data_len, &written_len);
		}

		volume_close(volume);
	}

	return status;
}

int metadata_manager_init(struct metadata_manager *subject,
			  const struct metadata_serializer *serializer)
{
	assert(subject);
	assert(serializer);

	subject->serializer = serializer;

	/* Depending on the class of device, the storage volumes that hold FWU
	 * metadata may or may not be accessible by the metadata_manager. This
	 * should be reflected by which volumes have been added to the volume_index
	 * by deployment specific configuration code.
	 */
	subject->primary_metadata_volume = NULL;
	subject->backup_metadata_volume = NULL;

	volume_index_find(BANKED_VOLUME_ID_PRIMARY_METADATA, &subject->primary_metadata_volume);
	volume_index_find(BANKED_VOLUME_ID_BACKUP_METADATA, &subject->backup_metadata_volume);

	/* A cached copy of the metadata is held in memory. It will not initially
	 * hold a valid copy until one has been loaded from storage or a fresh
	 * version has been written.
	 */
	subject->is_dirty = false;
	subject->is_valid = false;
	subject->stored_crc = 0;
	subject->metadata_len = 0;
	subject->metadata_max_len = subject->serializer->max_size();
	subject->metadata_cache = malloc(subject->metadata_max_len);

	return (subject->metadata_cache) ? FWU_STATUS_SUCCESS : FWU_STATUS_UNKNOWN;
}

void metadata_manager_deinit(struct metadata_manager *subject)
{
	free(subject->metadata_cache);
}

int metadata_manager_check_and_repair(struct metadata_manager *subject,
				      const struct fw_directory *fw_dir)
{
	int primary_status = FWU_STATUS_UNKNOWN;
	int backup_status = FWU_STATUS_UNKNOWN;

	/* No need to perform operation if valid data is already held.*/
	if (subject->is_valid)
		return FWU_STATUS_SUCCESS;

	/* If no storage volume is accessible (e.g. with a single flash configuration),
	 * the operation can never succeed.
	 */
	if (!subject->primary_metadata_volume && !subject->backup_metadata_volume) {
		IMSG("FWU volume not accessible");
		return FWU_STATUS_UNKNOWN;
	}

	/* Loaded metadata length should be consistent with the view of firmware held
	 * by the fw_directory.
	 */
	subject->metadata_len = subject->serializer->size(fw_dir);

	if (subject->primary_metadata_volume) {
		primary_status = load_and_check_metadata(subject->primary_metadata_volume,
							 subject->metadata_cache,
							 subject->metadata_len);

		subject->is_valid = (primary_status == FWU_STATUS_SUCCESS);
	}

	if (subject->backup_metadata_volume) {
		if (subject->is_valid) {
			/* Already successfully loaded the primary copy. Just need to check
			 * the backup and repair it if necessary. During an update operation,
			 * the primary metadata is always written first. A hazard exists where
			 * both primary and backup are valid but they contain different data
			 * due to a power failure just before writing the backup. This
			 * condition needs to be checked for.
			 */
			uint8_t *backup_buf = malloc(subject->metadata_len);

			if (backup_buf) {
				backup_status =
					load_and_check_metadata(subject->backup_metadata_volume,
								backup_buf, subject->metadata_len);

				if ((backup_status == FWU_STATUS_SUCCESS) &&
				    (*(uint32_t *)backup_buf !=
				     *(uint32_t *)subject->metadata_cache)) {
					/* Both copies have valid CRC but CRSs are different. Force
					 * the backup to be repaired.
					 */
					backup_status = FWU_STATUS_UNKNOWN;
				}

				free(backup_buf);
			}
		} else {
			/* Primary must have failed so use the backup copy. */
			backup_status = load_and_check_metadata(subject->backup_metadata_volume,
								subject->metadata_cache,
								subject->metadata_len);

			subject->is_valid = (backup_status == FWU_STATUS_SUCCESS);
		}
	}

	/* Attempt a repair if necessary (and possible) */
	if (subject->is_valid) {
		if ((primary_status != FWU_STATUS_SUCCESS) && subject->primary_metadata_volume) {
			IMSG("Repairing primary FWU metadata");

			primary_status = store_metadata(subject->primary_metadata_volume,
							subject->metadata_cache,
							subject->metadata_len);
		}

		if ((backup_status != FWU_STATUS_SUCCESS) && subject->backup_metadata_volume) {
			IMSG("Repairing backup FWU metadata");

			backup_status = store_metadata(subject->backup_metadata_volume,
						       subject->metadata_cache,
						       subject->metadata_len);
		}
	}

	/* Synchronize the view of the stored CRC */
	if (subject->is_valid)
		subject->stored_crc = *(uint32_t *)subject->metadata_cache;

	return (subject->is_valid) ? FWU_STATUS_SUCCESS : FWU_STATUS_UNKNOWN;
}

int metadata_manager_update(struct metadata_manager *subject, uint32_t active_index,
			    uint32_t previous_active_index, const struct fw_directory *fw_dir,
			    const struct bank_tracker *bank_tracker)
{
	int primary_status = FWU_STATUS_SUCCESS;
	int backup_status = FWU_STATUS_SUCCESS;

	subject->metadata_len = subject->serializer->size(fw_dir);

	/* Serialize metadata into metadata cache */
	subject->serializer->serialize(active_index, previous_active_index, fw_dir, bank_tracker,
				       subject->metadata_cache, subject->metadata_max_len,
				       &subject->metadata_len);

	/* Update cache copy with valid crc */
	uint32_t calc_crc = crc32(0U, subject->metadata_cache + sizeof(uint32_t),
				  subject->metadata_len - sizeof(uint32_t));
	*(uint32_t *)subject->metadata_cache = calc_crc;

	bool was_valid = subject->is_valid;

	/* Cache has been updated so it now holds valid data */
	subject->is_valid = true;
	subject->is_dirty = true;

	/* To prevent unnecessary flash writes, if after serialization, there
	 * is no change to the metadata, skip the store operation.
	 */
	if (was_valid && (subject->stored_crc == *(uint32_t *)subject->metadata_cache))
		return FWU_STATUS_SUCCESS;

	/* Update NV storage - order of primary followed by backup is important to
	 * defend against a power failure after updating the primary but before the backup.
	 */
	if (subject->primary_metadata_volume) {
		primary_status = store_metadata(subject->primary_metadata_volume,
						subject->metadata_cache, subject->metadata_len);
	}

	if (subject->backup_metadata_volume) {
		backup_status = store_metadata(subject->backup_metadata_volume,
					       subject->metadata_cache, subject->metadata_len);
	}

	/* Updated the view of the stored data if successfully stored */
	if ((primary_status == FWU_STATUS_SUCCESS) && (backup_status == FWU_STATUS_SUCCESS))
		subject->stored_crc = *(uint32_t *)subject->metadata_cache;

	return (primary_status != FWU_STATUS_SUCCESS) ? primary_status :
	       (backup_status != FWU_STATUS_SUCCESS)  ? backup_status :
							FWU_STATUS_SUCCESS;
}

int metadata_manager_fetch(struct metadata_manager *subject, const uint8_t **data, size_t *data_len,
			   bool *is_dirty)
{
	int status = FWU_STATUS_UNKNOWN;

	if (subject->is_valid) {
		*data = subject->metadata_cache;
		*data_len = subject->metadata_len;
		*is_dirty = subject->is_dirty;

		subject->is_dirty = false;

		status = FWU_STATUS_SUCCESS;
	}

	return status;
}

int metadata_manager_get_active_indices(const struct metadata_manager *subject,
					uint32_t *active_index, uint32_t *previous_active_index)
{
	int status = FWU_STATUS_UNKNOWN;

	if (subject->is_valid) {
		subject->serializer->deserialize_active_indices(active_index, previous_active_index,
								subject->metadata_cache,
								subject->metadata_len);

		status = FWU_STATUS_SUCCESS;
	}

	return status;
}

void metadata_manager_cache_invalidate(struct metadata_manager *subject)
{
	subject->is_valid = false;
}

void metadata_manager_preload_bank_tracker(const struct metadata_manager *subject,
					   struct bank_tracker *bank_tracker)
{
	subject->serializer->deserialize_bank_info(bank_tracker, subject->metadata_cache,
						   subject->metadata_len);
}
