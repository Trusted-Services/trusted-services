/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "cmd_update_image.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "common/uuid/uuid.h"

int cmd_update_image(fwu_app &app, const std::string &img_type_uuid,
		     const std::string &img_filename)
{
	FILE *fp = fopen(img_filename.c_str(), "rb");

	if (!fp) {
		printf("Error: failed to open image file: %s\n", img_filename.c_str());
		return -1;
	}

	/* Get file size */
	fseek(fp, 0, SEEK_END);
	size_t img_size = ftell(fp);
	rewind(fp);

	/* Allocate buffer for image data */
	uint8_t *img_buf = (uint8_t *)malloc(img_size);

	if (!img_buf) {
		fclose(fp);
		printf("Error: failed to allocate image buffer\n");
		return -1;
	}

	/* Read file contents into buffer */
	if (fread(img_buf, 1, img_size, fp) < 0) {
		fclose(fp);
		free(img_buf);
		printf("Error: failed to read image file\n");
		return -1;
	}

	fclose(fp);

	/* Apply update */
	struct uuid_octets uuid;

	uuid_guid_octets_from_canonical(&uuid, img_type_uuid.c_str());

	int status = app.update_image(uuid, img_buf, img_size);

	if (status)
		printf("Error: update image failed\n");

	free(img_buf);

	return status;
}
