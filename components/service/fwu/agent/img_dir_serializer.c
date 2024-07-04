/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "img_dir_serializer.h"

#include <assert.h>
#include <string.h>

#include "fw_directory.h"
#include "protocols/service/fwu/fwu_proto.h"
#include "service/fwu/fw_store/fw_store.h"

int img_dir_serializer_serialize(const struct fw_directory *fw_dir, const struct fw_store *fw_store,
				 uint8_t *buf, size_t buf_size, size_t *data_len)
{
	size_t serialized_len = img_dir_serializer_get_len(fw_dir);

	*data_len = 0;

	if (buf_size < serialized_len)
		return FWU_STATUS_OUT_OF_BOUNDS;

	struct fwu_image_directory *output = (struct fwu_image_directory *)buf;

	/* Clear the output buffer */
	memset(buf, 0, serialized_len);

	/* Serialize boot info */
	const struct boot_info *boot_info = fw_directory_get_boot_info(fw_dir);

	assert(boot_info);

	output->directory_version = 2;
	output->img_info_offset = offsetof(struct fwu_image_directory, img_info_entry);
	output->num_images = fw_directory_num_images(fw_dir);
	output->correct_boot = (boot_info->active_index == boot_info->boot_index);
	output->img_info_size = sizeof(struct fwu_image_info_entry);
	output->reserved = 0;

	/* Serialize image info for each image */
	for (size_t image_index = 0; image_index < output->num_images; image_index++) {
		const struct image_info *image_info =
			fw_directory_get_image_info(fw_dir, image_index);

		assert(image_info);

		memcpy(output->img_info_entry[image_index].img_type_uuid,
		       image_info->img_type_uuid.octets, OSF_UUID_OCTET_LEN);

		output->img_info_entry[image_index].client_permissions = image_info->permissions;
		output->img_info_entry[image_index].img_max_size = image_info->max_size;
		output->img_info_entry[image_index].lowest_accepted_version =
			image_info->lowest_accepted_version;
		output->img_info_entry[image_index].img_version = image_info->active_version;
		output->img_info_entry[image_index].accepted =
			(uint32_t)fw_store_is_accepted(fw_store, image_info);
	}

	*data_len = serialized_len;

	return FWU_STATUS_SUCCESS;
}

size_t img_dir_serializer_get_len(const struct fw_directory *fw_dir)
{
	return offsetof(struct fwu_image_directory, img_info_entry) +
	       sizeof(struct fwu_image_info_entry) * fw_directory_num_images(fw_dir);
}
