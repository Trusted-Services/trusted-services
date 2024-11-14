/*
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "cmd_print_metadata_v2.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "common/uuid/uuid.h"
#include "print_uuid.h"
#include "protocols/service/fwu/fwu_proto.h"
#include "protocols/service/fwu/metadata_v2.h"

void cmd_print_metadata_v2(fwu_app &app)
{
	std::vector<uint8_t> fetched_object;
	struct uuid_octets object_uuid;

	uuid_guid_octets_from_canonical(&object_uuid, FWU_METADATA_CANONICAL_UUID);

	int status = app.read_object(object_uuid, fetched_object);

	if (status) {
		printf("Error: failed to read metadata\n");
		return;
	}

	if (fetched_object.size() < sizeof(struct fwu_metadata)) {
		printf("Error: invalid metadata size\n");
		return;
	}

	/* Print mandatory metadata header */
	const struct fwu_metadata *metadata = (const struct fwu_metadata *)fetched_object.data();

	printf("\nfwu_metadata (size %zu bytes) :\n", fetched_object.size());
	printf("\tcrc_32 : 0x%x\n", metadata->crc_32);
	printf("\tversion : %d\n", metadata->version);
	printf("\tmetadata_size : %d\n", metadata->metadata_size);
	printf("\tdescriptor_offset : %d\n", metadata->descriptor_offset);
	printf("\tactive_index : %d\n", metadata->active_index);
	printf("\tprevious_active_index : %d\n", metadata->previous_active_index);
	printf("\tbank_state : 0x%x 0x%x\n", metadata->bank_state[0], metadata->bank_state[1]);

	if (metadata->metadata_size <= metadata->descriptor_offset)
		return;

	size_t fw_store_desc_size = metadata->metadata_size - metadata->descriptor_offset;

	if (fw_store_desc_size < sizeof(fwu_fw_store_desc)) {
		printf("\tInsufficient space for fw store descriptor\n");
		return;
	}

	/* Print optional fw store descriptor */
	struct fwu_fw_store_desc *fw_store_desc =
		(struct fwu_fw_store_desc *)&fetched_object[metadata->descriptor_offset];

	printf("\tfw_store_desc :\n");
	printf("\t\tnum_banks : %d\n", fw_store_desc->num_banks);
	printf("\t\tnum_images : %d\n", fw_store_desc->num_images);
	printf("\t\timg_entry_size : %d\n", fw_store_desc->img_entry_size);
	printf("\t\tbank_info_entry_size : %d\n", fw_store_desc->bank_info_entry_size);

	for (unsigned int i = 0; i < fw_store_desc->num_images; i++) {
		struct fwu_image_entry *img_entry = &fw_store_desc->img_entry[i];

		printf("\t\timg_entry[%u] :\n", i);
		printf("\t\t\timg_type_uuid : %s\n", print_uuid(img_entry->img_type_uuid).c_str());
		printf("\t\t\tlocation_uuid : %s\n", print_uuid(img_entry->location_uuid).c_str());

		for (unsigned int j = 0; j < fw_store_desc->num_banks; j++) {
			struct fwu_img_bank_info *bank_info = &img_entry->img_bank_info[j];

			printf("\t\t\timg_bank_info[%u] :\n", j);
			printf("\t\t\t\timg_uuid : %s\n", print_uuid(bank_info->img_uuid).c_str());
			printf("\t\t\t\taccepted : %d\n", bank_info->accepted);
		}
	}
}
