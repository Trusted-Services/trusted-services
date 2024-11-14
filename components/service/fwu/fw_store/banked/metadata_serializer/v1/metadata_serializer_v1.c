/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "metadata_serializer_v1.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "common/uuid/uuid.h"
#include "media/volume/index/volume_index.h"
#include "media/volume/volume.h"
#include "protocols/service/fwu/metadata_v1.h"
#include "protocols/service/fwu/status.h"
#include "service/fwu/agent/fw_directory.h"
#include "service/fwu/fw_store/banked/bank_tracker.h"
#include "service/fwu/fw_store/banked/metadata_serializer/metadata_serializer.h"
#include "service/fwu/fw_store/banked/volume_id.h"

static int serialize_image_entries(struct fwu_metadata *metadata, const struct fw_directory *fw_dir,
				   const struct bank_tracker *bank_tracker)
{
	size_t image_index = 0;

	do {
		/* Image entry indices in the metadata correspond to the image index
		 * of the associate entry in the fw_directory.
		 */
		const struct image_info *image_info =
			fw_directory_get_image_info(fw_dir, image_index);

		if (!image_info)
			break;

		/* Information about storage for the image is retrieved from the configured
		 * volume objects that provide access to the banked storage. Both volumes
		 * are assumed to have the same parent location, identified by the location
		 * uuid.
		 */
		struct uuid_octets location_uuid = { 0 };
		struct fwu_image_entry *entry = &metadata->img_entry[image_index];

		/* Serialize bank storage info */
		for (size_t bank_index = 0; bank_index < BANK_SCHEME_NUM_BANKS; bank_index++) {
			struct uuid_octets img_uuid = { 0 };
			struct volume *volume = NULL;

			int status =
				volume_index_find(banked_volume_id(image_info->location_id,
								   banked_usage_id(bank_index)),
						  &volume);

			if (!status && volume)
				volume_get_storage_ids(volume, &img_uuid, &location_uuid);

			struct fwu_image_properties *properties = &entry->img_props[bank_index];

			memcpy(properties->img_uuid, img_uuid.octets, OSF_UUID_OCTET_LEN);
			properties->reserved = 0;
			properties->accepted =
				bank_tracker_is_accepted(bank_tracker, bank_index, image_index) ?
					1 :
					0;
		}

		/* Serialize per-image UUIDs */
		memcpy(entry->img_type_uuid, image_info->img_type_uuid.octets, OSF_UUID_OCTET_LEN);
		memcpy(entry->location_uuid, location_uuid.octets, OSF_UUID_OCTET_LEN);

		++image_index;

	} while (true);

	return FWU_STATUS_SUCCESS;
}

static size_t metadata_serializer_size(const struct fw_directory *fw_dir)
{
	return offsetof(struct fwu_metadata, img_entry) +
	       fw_directory_num_images(fw_dir) * sizeof(struct fwu_image_entry);
}

static size_t metadata_serializer_max_size(void)
{
	return offsetof(struct fwu_metadata, img_entry) +
	       FWU_MAX_FW_DIRECTORY_ENTRIES * sizeof(struct fwu_image_entry);
}

static int metadata_serializer_serialize(uint32_t active_index, uint32_t previous_active_index,
					 const struct fw_directory *fw_dir,
					 const struct bank_tracker *bank_tracker, uint8_t *buf,
					 size_t buf_size, size_t *metadata_len)
{
	int status = FWU_STATUS_UNKNOWN;
	size_t serialized_size = metadata_serializer_size(fw_dir);

	*metadata_len = 0;

	if (serialized_size <= buf_size) {
		struct fwu_metadata *metadata = (struct fwu_metadata *)buf;

		/* Serialize metadata header */
		metadata->crc_32 = 0;
		metadata->version = FWU_METADATA_VERSION;
		metadata->active_index = active_index;
		metadata->previous_active_index = previous_active_index;

		/* Serialize image entries */
		status = serialize_image_entries(metadata, fw_dir, bank_tracker);

		if (status == FWU_STATUS_SUCCESS)
			*metadata_len = serialized_size;
	}

	return status;
}

static void metadata_serializer_deserialize_bank_info(struct bank_tracker *bank_tracker,
						      const uint8_t *serialized_metadata,
						      size_t metadata_len)
{
	const struct fwu_metadata *metadata = (const struct fwu_metadata *)serialized_metadata;

	/* Assume referenced banks hold content */
	if (metadata->active_index < BANK_SCHEME_NUM_BANKS)
		bank_tracker_set_holds_content(bank_tracker, metadata->active_index);

	if (metadata->previous_active_index < BANK_SCHEME_NUM_BANKS)
		bank_tracker_set_holds_content(bank_tracker, metadata->previous_active_index);

	/* Deserialize image accept state */
	if (metadata_len >= offsetof(struct fwu_metadata, img_entry)) {
		size_t num_images = (metadata_len - offsetof(struct fwu_metadata, img_entry)) /
				    sizeof(struct fwu_image_entry);

		for (size_t image_index = 0; image_index < num_images; image_index++) {
			const struct fwu_image_entry *image_entry =
				&metadata->img_entry[image_index];

			for (size_t bank_index = 0; bank_index < BANK_SCHEME_NUM_BANKS;
			     bank_index++) {
				if (image_entry->img_props[bank_index].accepted)
					bank_tracker_accept(bank_tracker, bank_index, image_index);
			}
		}
	}
}

static void metadata_serializer_deserialize_active_indices(uint32_t *active_index,
							   uint32_t *previous_active_index,
							   const uint8_t *serialized_metadata,
							   size_t metadata_len)
{
	const struct fwu_metadata *metadata = (const struct fwu_metadata *)serialized_metadata;

	assert(metadata_len >= offsetof(struct fwu_metadata, img_entry));

	*active_index = metadata->active_index;
	*previous_active_index = metadata->previous_active_index;
}

const struct metadata_serializer *metadata_serializer_v1(void)
{
	static const struct metadata_serializer serializer = {
		metadata_serializer_serialize, metadata_serializer_size,
		metadata_serializer_max_size, metadata_serializer_deserialize_bank_info,
		metadata_serializer_deserialize_active_indices
	};

	return &serializer;
}
