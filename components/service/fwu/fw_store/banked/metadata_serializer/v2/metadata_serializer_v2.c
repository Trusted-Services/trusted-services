/*
 * Copyright (c) 2023-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "metadata_serializer_v2.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "common/uuid/uuid.h"
#include "media/volume/index/volume_index.h"
#include "media/volume/volume.h"
#include "protocols/service/fwu/metadata_v2.h"
#include "protocols/service/fwu/status.h"
#include "service/fwu/agent/fw_directory.h"
#include "service/fwu/fw_store/banked/bank_tracker.h"
#include "service/fwu/fw_store/banked/metadata_serializer/metadata_serializer.h"
#include "service/fwu/fw_store/banked/volume_id.h"

static size_t metadata_serializer_size(const struct fw_directory *fw_dir)
{
	return sizeof(struct fwu_metadata) + offsetof(struct fwu_fw_store_desc, img_entry) +
	       sizeof(struct fwu_image_entry) * fw_directory_num_images(fw_dir);
}

static size_t metadata_serializer_max_size(void)
{
	return sizeof(struct fwu_metadata) + offsetof(struct fwu_fw_store_desc, img_entry) +
	       sizeof(struct fwu_image_entry) * FWU_MAX_FW_DIRECTORY_ENTRIES;
}

static int serialize_image_entries(struct fwu_fw_store_desc *fw_store_desc,
				   const struct fw_directory *fw_dir,
				   const struct bank_tracker *bank_tracker)
{
	for (size_t image_index = 0; image_index < fw_store_desc->num_images; image_index++) {
		/* Image entry indices in the fw_store_desc correspond to the image index
		 * of the associate entry in the fw_directory.
		 */
		const struct image_info *image_info =
			fw_directory_get_image_info(fw_dir, image_index);

		assert(image_info);

		/* Information about storage for the image is retrieved from the configured
		 * volume objects that provide access to the banked storage. Both volumes
		 * are assumed to have the same parent location, identified by the location
		 * uuid.
		 */
		struct uuid_octets location_uuid = { 0 };
		struct fwu_image_entry *entry = &fw_store_desc->img_entry[image_index];

		/* Serialize bank storage info */
		for (size_t bank_index = 0; bank_index < BANK_SCHEME_NUM_BANKS; bank_index++) {
			struct uuid_octets img_uuid = { 0 };
			struct volume *volume = NULL;

			int status =
				volume_index_find(banked_volume_id(image_info->location_id,
								   banked_usage_id(bank_index)),
						  &volume);

			if (!status && volume) {
				/* A concrete volume may not support retrieving storage IDs. If
				 * this is the case, volume_get_storage_ids() will return an error,
				 * which is deliberately ignored and UUIDs left in their default state.
				 * To ensure that all banks reflect the same parent location, the
				 * location uuid returned by the volume holding the first bank is
				 * used as the parent.
				 */
				struct uuid_octets *parent_uuid =
					(bank_index == 0) ? &location_uuid : NULL;

				volume_get_storage_ids(volume, &img_uuid, parent_uuid);
			}

			struct fwu_img_bank_info *bank_info = &entry->img_bank_info[bank_index];

			memcpy(bank_info->img_uuid, img_uuid.octets, OSF_UUID_OCTET_LEN);
			bank_info->reserved = 0;
			bank_info->accepted =
				bank_tracker_is_accepted(bank_tracker, bank_index, image_index) ?
					1 :
					0;
		}

		/* Serialize per-image UUIDs */
		memcpy(entry->img_type_uuid, image_info->img_type_uuid.octets, OSF_UUID_OCTET_LEN);
		memcpy(entry->location_uuid, location_uuid.octets, OSF_UUID_OCTET_LEN);
	}

	return FWU_STATUS_SUCCESS;
}

static int serialize_fw_store_desc(const struct fw_directory *fw_dir,
				   const struct bank_tracker *bank_tracker, uint8_t *buf)
{
	struct fwu_fw_store_desc *fw_store_desc = (struct fwu_fw_store_desc *)buf;

	fw_store_desc->num_banks = BANK_SCHEME_NUM_BANKS;
	fw_store_desc->num_images = fw_directory_num_images(fw_dir);
	fw_store_desc->img_entry_size = sizeof(struct fwu_image_entry);
	fw_store_desc->bank_info_entry_size = sizeof(struct fwu_img_bank_info);

	return serialize_image_entries(fw_store_desc, fw_dir, bank_tracker);
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
		metadata->version = 2;
		metadata->metadata_size = (uint32_t)serialized_size;
		metadata->descriptor_offset = (uint16_t)sizeof(struct fwu_metadata);
		metadata->active_index = active_index;
		metadata->previous_active_index = previous_active_index;

		for (unsigned int bank_index = 0; bank_index < FWU_METADATA_V2_NUM_BANK_STATES;
		     bank_index++) {
			if (bank_index < BANK_SCHEME_NUM_BANKS) {
				if (bank_tracker_is_all_accepted(bank_tracker, bank_index,
								 fw_directory_num_images(fw_dir)))
					metadata->bank_state[bank_index] =
						FWU_METADATA_V2_BANK_STATE_ACCEPTED;
				else if (bank_tracker_is_content(bank_tracker, bank_index))
					metadata->bank_state[bank_index] =
						FWU_METADATA_V2_BANK_STATE_VALID;
				else
					metadata->bank_state[bank_index] =
						FWU_METADATA_V2_BANK_STATE_INVALID;

			} else
				metadata->bank_state[bank_index] =
					FWU_METADATA_V2_BANK_STATE_INVALID;
		}

		/* Serialize optional fw store descriptor if required */
		if (serialized_size > metadata->descriptor_offset)
			status = serialize_fw_store_desc(fw_dir, bank_tracker,
							 &buf[metadata->descriptor_offset]);
		else
			status = FWU_STATUS_SUCCESS;

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

	/* Sanity check size values in header */
	if ((metadata->descriptor_offset > metadata_len) ||
	    (metadata->metadata_size > metadata_len))
		return;

	/* Deserialize bank state in header and update bank_tracker to reflect the same state */
	for (unsigned int bank_index = 0; bank_index < BANK_SCHEME_NUM_BANKS; bank_index++) {
		if (metadata->bank_state[bank_index] == FWU_METADATA_V2_BANK_STATE_ACCEPTED)
			bank_tracker_set_holds_accepted_content(bank_tracker, bank_index);
		else if (metadata->bank_state[bank_index] == FWU_METADATA_V2_BANK_STATE_VALID)
			bank_tracker_set_holds_content(bank_tracker, bank_index);
	}

	/* If present, deserialize the fw_store_desc */
	if (metadata->metadata_size >=
	    metadata->descriptor_offset + offsetof(struct fwu_fw_store_desc, img_entry)) {
		const struct fwu_fw_store_desc *fw_store_desc =
			(const struct fwu_fw_store_desc *)(serialized_metadata +
							   metadata->descriptor_offset);

		size_t fw_store_desc_size = metadata->metadata_size - metadata->descriptor_offset;
		size_t total_img_entries_size =
			fw_store_desc_size - offsetof(struct fwu_fw_store_desc, img_entry);
		size_t per_img_entry_bank_info_size =
			fw_store_desc->num_banks * fw_store_desc->bank_info_entry_size;

		/* Sanity check fw_store_desc values */
		if ((fw_store_desc->img_entry_size < sizeof(struct fwu_image_entry)) ||
		    (fw_store_desc->bank_info_entry_size < sizeof(struct fwu_img_bank_info)) ||
		    (fw_store_desc->num_banks > BANK_SCHEME_NUM_BANKS) ||
		    (fw_store_desc->img_entry_size <
		     offsetof(struct fwu_image_entry, img_bank_info) +
			     per_img_entry_bank_info_size) ||
		    (fw_store_desc->num_images > FWU_MAX_FW_DIRECTORY_ENTRIES) ||
		    (total_img_entries_size <
		     fw_store_desc->num_images * fw_store_desc->img_entry_size))
			return;

		/* Deserialize per-image info */
		for (size_t image_index = 0; image_index < fw_store_desc->num_images;
		     image_index++) {
			const struct fwu_image_entry *image_entry =
				(const struct fwu_image_entry
					 *)((const uint8_t *)fw_store_desc->img_entry +
					    image_index * fw_store_desc->img_entry_size);

			for (size_t bank_index = 0; bank_index < fw_store_desc->num_banks;
			     bank_index++) {
				const struct fwu_img_bank_info *bank_info =
					(const struct fwu_img_bank_info *)((const uint8_t *)
					image_entry->img_bank_info + bank_index *
					fw_store_desc->bank_info_entry_size);

				if (bank_info->accepted)
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

	assert(metadata_len >= sizeof(struct fwu_metadata));

	*active_index = metadata->active_index;
	*previous_active_index = metadata->previous_active_index;
}

const struct metadata_serializer *metadata_serializer_v2(void)
{
	static const struct metadata_serializer serializer = {
		metadata_serializer_serialize, metadata_serializer_size,
		metadata_serializer_max_size, metadata_serializer_deserialize_bank_info,
		metadata_serializer_deserialize_active_indices
	};

	return &serializer;
}
