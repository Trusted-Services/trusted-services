/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "cmd_print_image_dir.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "common/uuid/uuid.h"
#include "print_uuid.h"
#include "protocols/service/fwu/fwu_proto.h"

void cmd_print_image_dir(fwu_app &app)
{
	std::vector<uint8_t> fetched_object;
	struct uuid_octets object_uuid;

	uuid_guid_octets_from_canonical(&object_uuid, FWU_DIRECTORY_CANONICAL_UUID);

	int status = app.read_object(object_uuid, fetched_object);

	if (status) {
		printf("Error: failed to read image directory\n");
		return;
	}

	if (fetched_object.size() < offsetof(fwu_image_directory, img_info_entry)) {
		printf("Error: invalid image directory size\n");
		return;
	}

	const struct fwu_image_directory *img_dir =
		(const struct fwu_image_directory *)fetched_object.data();

	printf("\nimage_directory (size %zu bytes) :\n", fetched_object.size());
	printf("\tdirectory_version : %d\n", img_dir->directory_version);
	printf("\tnum_images : %d\n", img_dir->num_images);
	printf("\tcorrect_boot : %d\n", img_dir->correct_boot);

	for (unsigned int i = 0; i < img_dir->num_images; i++) {
		printf("\timg_info_entry[%u]:\n", i);
		printf("\t\timg_type_uuid : %s\n",
		       print_uuid(img_dir->img_info_entry[i].img_type_uuid).c_str());
		printf("\t\tclient_permissions : 0x%x\n",
		       img_dir->img_info_entry[i].client_permissions);
		printf("\t\timg_max_size : %d\n", img_dir->img_info_entry[i].img_max_size);
		printf("\t\tlowest_accepted_version : %d\n",
		       img_dir->img_info_entry[i].lowest_accepted_version);
		printf("\t\timg_version : %d\n", img_dir->img_info_entry[i].img_version);
		printf("\t\taccepted : %d\n", img_dir->img_info_entry[i].accepted);
	}
}
