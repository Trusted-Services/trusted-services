/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "fw_directory.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "protocols/service/fwu/status.h"

void fw_directory_init(struct fw_directory *fw_directory)
{
	assert(fw_directory);

	/* The fw_directory is initially empty. It will be populated by a
	 * fw_inspector.
	 */
	memset(fw_directory, 0, sizeof(struct fw_directory));
}

void fw_directory_deinit(struct fw_directory *fw_directory)
{
	(void)fw_directory;
}

void fw_directory_set_boot_info(struct fw_directory *fw_directory,
				const struct boot_info *boot_info)
{
	assert(fw_directory);
	fw_directory->boot_info = *boot_info;
}

int fw_directory_add_image_info(struct fw_directory *fw_directory,
				const struct image_info *image_info)
{
	assert(fw_directory);
	assert(image_info);

	int status = FWU_STATUS_UNKNOWN;

	if (fw_directory->num_images < FWU_MAX_FW_DIRECTORY_ENTRIES) {
		uint32_t image_index = fw_directory->num_images;

		fw_directory->entries[image_index] = *image_info;
		fw_directory->entries[image_index].image_index = image_index;

		++fw_directory->num_images;

		status = FWU_STATUS_SUCCESS;
	}

	return status;
}

const struct image_info *fw_directory_find_image_info(const struct fw_directory *fw_directory,
						      const struct uuid_octets *img_type_uuid)
{
	assert(fw_directory);
	assert(img_type_uuid);

	const struct image_info *info = NULL;

	for (size_t i = 0; i < fw_directory->num_images; i++) {
		if (uuid_is_equal(img_type_uuid->octets,
				  fw_directory->entries[i].img_type_uuid.octets)) {
			info = &fw_directory->entries[i];
			break;
		}
	}

	return info;
}

const struct boot_info *fw_directory_get_boot_info(const struct fw_directory *fw_directory)
{
	assert(fw_directory);
	return &fw_directory->boot_info;
}

const struct image_info *fw_directory_get_image_info(const struct fw_directory *fw_directory,
						     size_t index)
{
	assert(fw_directory);

	const struct image_info *info = NULL;

	if (index < fw_directory->num_images)
		info = &fw_directory->entries[index];

	return info;
}

size_t fw_directory_num_images(const struct fw_directory *fw_directory)
{
	assert(fw_directory);
	return fw_directory->num_images;
}
