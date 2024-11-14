/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "raw_installer.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "media/volume/index/volume_index.h"
#include "protocols/service/fwu/status.h"
#include "service/fwu/agent/fw_directory.h"

static int raw_installer_begin(void *context, unsigned int current_volume_id,
			       unsigned int update_volume_id)
{
	struct raw_installer *subject = (struct raw_installer *)context;

	(void)current_volume_id;

	int status = volume_index_find(update_volume_id, &subject->target_volume);

	if (status == 0) {
		assert(subject->target_volume);

		subject->commit_count = 0;
		subject->is_open = false;
	}

	return status;
}

static int raw_installer_finalize(void *context)
{
	struct raw_installer *subject = (struct raw_installer *)context;

	/* Close volume if left open */
	if (subject->is_open) {
		assert(subject->target_volume);

		volume_close(subject->target_volume);
		subject->is_open = false;
	}

	return FWU_STATUS_SUCCESS;
}

static void raw_installer_abort(void *context)
{
	raw_installer_finalize(context);
}

static int raw_installer_open(void *context, const struct image_info *image_info)
{
	struct raw_installer *subject = (struct raw_installer *)context;
	int status = FWU_STATUS_DENIED;

	/* Because the raw_installer uses a single image to update the
	 * target volume, it only makes sense to commit a single image
	 * during an update transaction. Defend against the case where
	 * an input update package contains more than one raw image to
	 * install into a particular location.
	 */
	if (!subject->is_open && subject->commit_count < 1) {
		assert(subject->target_volume);

		status = volume_open(subject->target_volume);

		if (!status) {
			/* Prior to writing to the volume to install the image, ensure
			 * that the volume is erased.
			 */
			status = volume_erase(subject->target_volume);

			if (!status) {
				subject->is_open = true;
				subject->bytes_written = 0;
			} else {
				/* Failed to erase */
				volume_close(subject->target_volume);
			}
		}
	}

	return status;
}

static int raw_installer_commit(void *context)
{
	struct raw_installer *subject = (struct raw_installer *)context;
	int status = FWU_STATUS_DENIED;

	if (subject->is_open) {
		assert(subject->target_volume);

		status = volume_close(subject->target_volume);

		++subject->commit_count;
		subject->is_open = false;

		if (!status && !subject->bytes_written) {
			/* Installing a zero length image can imply an image delete
			 * operation. For certain types of installer, this is a legitimate
			 * operation. For a raw_installer, there really is no way to
			 * delete an image so return an error if an attempt was made.
			 */
			status = FWU_STATUS_NOT_AVAILABLE;
		}
	}

	return status;
}

static int raw_installer_write(void *context, const uint8_t *data, size_t data_len)
{
	struct raw_installer *subject = (struct raw_installer *)context;
	int status = FWU_STATUS_DENIED;

	if (subject->is_open) {
		assert(subject->target_volume);

		size_t len_written = 0;

		status = volume_write(subject->target_volume, (const uintptr_t)data, data_len,
				      &len_written);

		subject->bytes_written += len_written;

		/* Check for the volume full condition where not all the requested
		 * data was written.
		 */
		if (!status && (len_written != data_len))
			status = FWU_STATUS_OUT_OF_BOUNDS;
	}

	return status;
}

static int raw_installer_enumerate(void *context, uint32_t volume_id,
				   struct fw_directory *fw_directory)
{
	struct raw_installer *subject = (struct raw_installer *)context;
	struct volume *volume = NULL;

	int status = volume_index_find(volume_id, &volume);

	if (status != 0)
		return status;

	assert(volume);

	/* Found the active volume so query it for information in order to
	 * prepare an entry in the fw_directory to represent the whole volume
	 * as an advertised updatable image.
	 */
	struct image_info image_info = { 0 };

	/* Limit the advertised max size to the volume size. The volume needs
	 * to be open to query its size.
	 */
	if (!subject->is_open) {
		/* Open if necessary */
		status = volume_open(volume);
		if (status != 0)
			return status;
	}

	status = volume_size(volume, &image_info.max_size);
	if (status != 0)
		return status;

	if (!subject->is_open) {
		/* Leave volume in the same open state */
		status = volume_close(volume);
		if (status)
			return status;
	}

	/* These attributes will have been assigned during platform configuration */
	image_info.img_type_uuid = subject->base_installer.location_uuid;
	image_info.location_id = subject->base_installer.location_id;
	image_info.install_type = subject->base_installer.install_type;

	status = fw_directory_add_image_info(fw_directory, &image_info);

	return status;
}

void raw_installer_init(struct raw_installer *subject, const struct uuid_octets *location_uuid,
			uint32_t location_id)
{
	/* Define concrete installer interface */
	static const struct installer_interface interface = {
		raw_installer_begin,	raw_installer_finalize, raw_installer_abort,
		raw_installer_open,	raw_installer_commit,	raw_installer_write,
		raw_installer_enumerate
	};

	/* Initialize base installer - a raw_installer is a type of
	 * installer that always updates a whole volume.
	 */
	installer_init(&subject->base_installer, INSTALL_TYPE_WHOLE_VOLUME, location_id,
		       location_uuid, subject, &interface);

	/* Initialize raw_installer specifics */
	subject->target_volume = NULL;
	subject->commit_count = 0;
	subject->bytes_written = 0;
	subject->is_open = false;
}

void raw_installer_deinit(struct raw_installer *subject)
{
	(void)subject;
}
