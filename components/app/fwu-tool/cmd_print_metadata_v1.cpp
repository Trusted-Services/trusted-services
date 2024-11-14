/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "cmd_print_metadata_v1.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "common/uuid/uuid.h"
#include "print_uuid.h"
#include "protocols/service/fwu/fwu_proto.h"
#include "protocols/service/fwu/metadata_v1.h"

void cmd_print_metadata_v1(fwu_app &app)
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

	const struct fwu_metadata *metadata = (const struct fwu_metadata *)fetched_object.data();

	printf("\nfwu_metadata (size %zu bytes) :\n", fetched_object.size());
	printf("\tcrc_32 : 0x%x\n", metadata->crc_32);
	printf("\tversion : %d\n", metadata->version);
	printf("\tactive_index : %d\n", metadata->active_index);
	printf("\tprevious_active_index : %d\n", metadata->previous_active_index);

	for (unsigned int i = 0; i < FWU_METADATA_NUM_IMAGE_ENTRIES; i++) {
		printf("\timg_entry[%u]:\n", i);
		printf("\t\timg_type_uuid : %s\n",
		       print_uuid(metadata->img_entry[i].img_type_uuid).c_str());
		printf("\t\tlocation_uuid : %s\n",
		       print_uuid(metadata->img_entry[i].location_uuid).c_str());

		for (unsigned int bank_index = 0; bank_index < FWU_METADATA_NUM_BANKS;
		     bank_index++) {
			printf("\t\timg_props[%u]:\n", bank_index);
			printf("\t\t\timg_uuid : %s\n",
			       print_uuid(metadata->img_entry[i].img_props[bank_index].img_uuid)
				       .c_str());
			printf("\t\t\taccepted : %d\n",
			       metadata->img_entry[i].img_props[bank_index].accepted);
		}
	}
}
