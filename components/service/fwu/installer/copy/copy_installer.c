/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "copy_installer.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include "media/volume/index/volume_index.h"
#include "protocols/service/fwu/status.h"
#include "util.h"

#define COPY_CHUNK_SIZE (4096)

static int close_volumes_on_error(struct copy_installer *subject)
{
	volume_close(subject->source_volume);
	volume_close(subject->destination_volume);

	return FWU_STATUS_UNKNOWN;
}

static int copy_volume_contents(struct copy_installer *subject, size_t target_copy_len)
{
	int status = FWU_STATUS_SUCCESS;
	size_t copy_len = 0;
	uint8_t *copy_buf = malloc(COPY_CHUNK_SIZE);

	if (!copy_buf)
		return FWU_STATUS_UNKNOWN;

	while (copy_len < target_copy_len) {
		size_t actual_read_len = 0;
		size_t actual_write_len = 0;
		size_t remaining_len = target_copy_len - copy_len;
		size_t requested_read_len = (remaining_len < COPY_CHUNK_SIZE) ? remaining_len :
										COPY_CHUNK_SIZE;

		status = volume_read(subject->source_volume, (uintptr_t)copy_buf,
				     requested_read_len, &actual_read_len);

		if (status)
			break;

		status = volume_write(subject->destination_volume, (const uintptr_t)copy_buf,
				      actual_read_len, &actual_write_len);

		if (status)
			break;

		if (actual_read_len != actual_write_len) {
			status = FWU_STATUS_UNKNOWN;
			break;
		}

		copy_len += actual_read_len;
	}

	free(copy_buf);

	return status;
}

static int copy_installer_begin(void *context, unsigned int current_volume_id,
				unsigned int update_volume_id)
{
	struct copy_installer *subject = (struct copy_installer *)context;

	int status = volume_index_find(update_volume_id, &subject->destination_volume);

	if (status == 0) {
		status = volume_index_find(current_volume_id, &subject->source_volume);
	}

	if (status != 0) {
		subject->destination_volume = NULL;
		subject->source_volume = NULL;
	}

	return status;
}

static int copy_installer_finalize(void *context)
{
	struct copy_installer *subject = (struct copy_installer *)context;

	assert(subject->source_volume);
	assert(subject->destination_volume);

	/* Open the source and destination volumes */
	int source_status = volume_open(subject->source_volume);

	if (source_status)
		return source_status;

	int destination_status = volume_open(subject->destination_volume);

	if (destination_status) {
		volume_close(subject->source_volume);
		return destination_status;
	}

	/* Prepare the destination volume for writes */
	destination_status = volume_erase(subject->destination_volume);

	if (destination_status)
		return close_volumes_on_error(subject);

	/* Determine how much to copy */
	size_t source_size = 0;
	size_t destination_size = 0;

	source_status = volume_size(subject->source_volume, &source_size);

	if (source_status)
		return close_volumes_on_error(subject);

	destination_status = volume_size(subject->source_volume, &destination_size);

	if (destination_status)
		return close_volumes_on_error(subject);

	/* Perform the copy */
	int copy_status = copy_volume_contents(subject, MIN(source_size, destination_size));

	/* All done */
	source_status = volume_close(subject->source_volume);
	destination_status = volume_close(subject->destination_volume);

	return (copy_status)	    ? copy_status :
	       (destination_status) ? destination_status :
	       (source_status)	    ? source_status :
				      FWU_STATUS_SUCCESS;
}

static void copy_installer_abort(void *context)
{
	struct copy_installer *subject = (struct copy_installer *)context;

	subject->source_volume = NULL;
	subject->destination_volume = NULL;
}

static int copy_installer_open(void *context, const struct image_info *image_info)
{
	(void)context;
	(void)image_info;

	return FWU_STATUS_DENIED;
}

static int copy_installer_commit(void *context)
{
	(void)context;

	return FWU_STATUS_DENIED;
}

static int copy_installer_write(void *context, const uint8_t *data, size_t data_len)
{
	(void)context;
	(void)data;
	(void)data_len;

	return FWU_STATUS_DENIED;
}

static int copy_installer_enumerate(void *context, uint32_t volume_id,
				    struct fw_directory *fw_directory)
{
	(void)volume_id;
	(void)fw_directory;

	/* A copy_installer can never be used to install externally provided images
	 * don't advertise any images via the fw_directory. Just quietly return success.
	 */
	return FWU_STATUS_SUCCESS;
}

void copy_installer_init(struct copy_installer *subject, const struct uuid_octets *location_uuid,
			 uint32_t location_id)
{
	/* Define concrete installer interface */
	static const struct installer_interface interface = {
		copy_installer_begin,	 copy_installer_finalize, copy_installer_abort,
		copy_installer_open,	 copy_installer_commit,	  copy_installer_write,
		copy_installer_enumerate
	};

	/* Initialize base installer - a copy_installer is a type of
	 * installer that always updates a whole volume by copying
	 * from another.
	 */
	installer_init(&subject->base_installer, INSTALL_TYPE_WHOLE_VOLUME_COPY, location_id,
		       location_uuid, subject, &interface);

	/* Initialize copy_installer specifics */
	subject->source_volume = NULL;
	subject->destination_volume = NULL;
}

void copy_installer_deinit(struct copy_installer *subject)
{
	(void)subject;
}
