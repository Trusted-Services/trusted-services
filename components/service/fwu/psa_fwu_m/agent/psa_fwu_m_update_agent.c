/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "psa_fwu_m_update_agent.h"
#include "common/uuid/uuid.h"
#include "service/fwu/psa_fwu_m/interface/update.h"
#include "protocols/service/fwu/fwu_proto.h"
#include "protocols/service/fwu/status.h"
#include "util.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifndef PSA_FWU_M_MAX_HANDLES
#define PSA_FWU_M_MAX_HANDLES	(8)
#endif /* PSA_FWU_M_MAX_HANDLES */

#define FWU_INVALID_HANDLE	(0xffffffff)

enum psa_fwu_m_state {
	regular,
	staging,
	trial,
};

struct psa_fwu_m_update_agent;

struct psa_fwu_m_image {
	struct uuid_octets uuid;
	psa_fwu_component_t component;
	bool selected_for_staging;
	bool accepted;
	int (*read)(struct psa_fwu_m_update_agent *agent, struct psa_fwu_m_image *image,
		    uint8_t *buf, size_t buf_size, size_t *read_len, size_t *total_len);
	int (*write)(struct psa_fwu_m_update_agent *agent, struct psa_fwu_m_image *image,
		     size_t data_offset, const uint8_t *data, size_t data_len);
};

struct psa_fwu_m_handle {
	bool used;
	size_t current_offset;
	uint8_t op_type;
	struct psa_fwu_m_image *image;
};

struct psa_fwu_m_update_agent {
	struct psa_fwu_m_image *images;
	size_t image_count;
	uint32_t max_payload_size;
	enum psa_fwu_m_state state;
	struct psa_fwu_m_handle handles[PSA_FWU_M_MAX_HANDLES];
};

static int cancel_staging(void *context);

static int psa_status_to_fwu_status(psa_status_t psa_status)
{
	switch (psa_status) {
	case PSA_SUCCESS:
		return FWU_STATUS_SUCCESS;

	case PSA_ERROR_DOES_NOT_EXIST:
		return FWU_STATUS_UNKNOWN;

	case PSA_ERROR_INVALID_ARGUMENT:
		return FWU_STATUS_OUT_OF_BOUNDS;

	case PSA_ERROR_INVALID_SIGNATURE:
		return FWU_STATUS_AUTH_FAIL;

	case PSA_ERROR_NOT_PERMITTED:
		return FWU_STATUS_NO_PERMISSION;

	case PSA_ERROR_BAD_STATE:
	default:
		return FWU_STATUS_DENIED;
	}
}

/* Image functions */
static struct psa_fwu_m_image *find_image(struct psa_fwu_m_update_agent *agent,
					  const struct uuid_octets *uuid)
{
	size_t i = 0;

	for (i = 0; i < agent->image_count; i++)
		if (uuid_is_equal(uuid->octets, agent->images[i].uuid.octets))
			return &agent->images[i];

	return NULL;
}

static int image_write(struct psa_fwu_m_update_agent *agent, struct psa_fwu_m_image *image,
		       size_t data_offset, const uint8_t *data, size_t data_len)
{
	return psa_status_to_fwu_status(
		psa_fwu_write(image->component, data_offset, data, data_len));
}

/* Image directory functions */
uint32_t image_version_to_uint(psa_fwu_image_version_t version)
{
	uint32_t result = 0;

	result |= ((uint32_t)version.major) << 24;
	result |= ((uint32_t)version.minor) << 16;
	result |= (uint32_t)version.patch;
	/* There's no room for the build number */

	return result;
}

int image_directory_read(struct psa_fwu_m_update_agent *agent, struct psa_fwu_m_image *image,
			 uint8_t *buf, size_t buf_size, size_t *read_len, size_t *total_len)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	psa_fwu_component_info_t component_info = { 0 };
	struct fwu_image_directory *directory = NULL;
	size_t image_count = agent->image_count - 1; /* Do not return Image directory */
	size_t image_info_size = 0;
	size_t i = 0;

	*read_len = 0;
	*total_len = 0;

	/* Calculate total length */
	if (MUL_OVERFLOW(sizeof(struct fwu_image_info_entry), image_count, &image_info_size))
		return FWU_STATUS_DENIED; /* LCOV_EXCL_LINE */

	if (ADD_OVERFLOW(sizeof(struct fwu_image_directory), image_info_size, total_len))
		return FWU_STATUS_DENIED; /* LCOV_EXCL_LINE */

	/*
	 * If the directory structure doesn't fit into the buffer return SUCCESS with total_len set
	 * and read_len = 0.
	 */
	if (*total_len > buf_size)
		return FWU_STATUS_SUCCESS;

	directory = (struct fwu_image_directory *)buf;
	directory->directory_version = FWU_IMAGE_DIRECTORY_VERSION;
	directory->img_info_offset = offsetof(struct fwu_image_directory, img_info_entry);
	directory->num_images = image_count;
	directory->correct_boot = 1; /* Set to 1 and then set to 0 if any image is not accepted */
	directory->img_info_size = sizeof(struct fwu_image_info_entry);
	directory->reserved = 0;

	for (i = 0; i < image_count; i++) {
		struct fwu_image_info_entry *entry = &directory->img_info_entry[i];
		struct psa_fwu_m_image *image = &agent->images[i];

		psa_status = psa_fwu_query(image->component, &component_info);
		if (psa_status != PSA_SUCCESS)
			return psa_status_to_fwu_status(psa_status);

		memcpy(entry->img_type_uuid, image->uuid.octets, sizeof(entry->img_type_uuid));
		entry->client_permissions = 0x1; /* Only write is supported by the API */
		entry->img_max_size = component_info.max_size;
		entry->lowest_accepted_version = 0; /* This information is not available */
		entry->img_version = image_version_to_uint(component_info.version);
		if (component_info.state == PSA_FWU_UPDATED) {
			entry->accepted = 1;
		} else {
			entry->accepted = 0;
			directory->correct_boot = 0;
		}
		entry->reserved = 0;
	}

	*read_len = *total_len;

	return FWU_STATUS_SUCCESS;
}

/* Image handle functions */
static uint32_t allocate_handle(struct psa_fwu_m_update_agent *agent, struct psa_fwu_m_image *image,
				uint8_t op_type)
{
	size_t i = 0;

	for (i = 0; i < ARRAY_SIZE(agent->handles); i++) {
		struct psa_fwu_m_handle *handle = &agent->handles[i];

		if (!handle->used) {
			handle->used = true;
			handle->current_offset = 0;
			handle->op_type = op_type;
			handle->image = image;

			return i;
		}
	}

	return FWU_INVALID_HANDLE;
}

static struct psa_fwu_m_handle *get_handle(struct psa_fwu_m_update_agent *agent,
					   uint32_t handle_index)
{
	if (handle_index >= ARRAY_SIZE(agent->handles))
		return NULL;

	if (!agent->handles[handle_index].used)
		return NULL;

	return &agent->handles[handle_index];
}

static void free_handle(struct psa_fwu_m_handle *handle)
{
	*handle = (struct psa_fwu_m_handle){ 0 };
}

/* Misc functions */
static int clean(void *context)
{
	struct psa_fwu_m_update_agent *agent = (struct psa_fwu_m_update_agent *)context;
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	uint32_t i = 0;

	for (i = 0; i < agent->image_count; i++) {
		struct psa_fwu_m_image *image = &agent->images[i];

		/* Skip read-only images */
		if (!image->write)
			continue;

		psa_status = psa_fwu_clean(image->component);
		if (psa_status != PSA_SUCCESS)
			return psa_status_to_fwu_status(psa_status);

		image->selected_for_staging = false;
		image->accepted = false;
	}

	return FWU_STATUS_SUCCESS;
}

void set_agent_state(struct psa_fwu_m_update_agent *agent, enum psa_fwu_m_state state)
{
	struct psa_fwu_m_image *image = NULL;
	size_t i = 0;

	if (state == regular) {
		for (i = 0; i < agent->image_count; i++) {
			image = &agent->images[i];

			image->selected_for_staging = false;
			image->accepted = false;
		}
	}

	agent->state = state;
}

/* Update agent interface */
static int discover(void *context, struct fwu_discovery_result *result)
{
	struct psa_fwu_m_update_agent *agent = (struct psa_fwu_m_update_agent *)context;

	result->service_status = 0;
	result->version_major = FWU_PROTOCOL_VERSION_MAJOR;
	result->version_minor = FWU_PROTOCOL_VERSION_MINOR;
	result->max_payload_size = agent->max_payload_size;
	result->flags = FWU_FLAG_PARTIAL_UPDATE;
	result->vendor_specific_flags = 0;

	return FWU_STATUS_SUCCESS;
}

static int begin_staging(void *context, uint32_t vendor_flags, uint32_t partial_update_count,
			 const struct uuid_octets *update_guid)
{
	struct psa_fwu_m_update_agent *agent = (struct psa_fwu_m_update_agent *)context;
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	int result = FWU_STATUS_DENIED;
	uint32_t i = 0;

	switch (agent->state) {
	case staging:
		/* Discard pending state */
		result = cancel_staging(context);
		if (result != FWU_STATUS_SUCCESS)
			goto error;

		result = clean(context);
		if (result != FWU_STATUS_SUCCESS)
			return result;

		/* fallthrough */

	case regular:
		if (partial_update_count) {
			/* Put selected images into staging state */
			for (i = 0; i < partial_update_count; i++) {
				struct psa_fwu_m_image *image = NULL;

				image = find_image(agent, &update_guid[i]);
				if (!image)
					goto error;

				/* Deny explicitly asked read-only images */
				if (!image->write)
					goto error;

				psa_status = psa_fwu_start(image->component, NULL, 0);
				if (psa_status != PSA_SUCCESS)
					goto error;

				image->selected_for_staging = true;
			}
		} else {
			/* Put all images into staging state */
			for (i = 0; i < agent->image_count; i++) {
				struct psa_fwu_m_image *image = &agent->images[i];

				/* Skip read-only images */
				if (!image->write)
					continue;

				psa_status = psa_fwu_start(image->component, NULL, 0);
				if (psa_status != PSA_SUCCESS)
					goto error;

				image->selected_for_staging = true;
			}
		}

		set_agent_state(agent, staging);

		return FWU_STATUS_SUCCESS;

	default:
		/* Calling begin_staging in other states is deined */
		return FWU_STATUS_DENIED;
	}

error:
	/* Revert everything to regular state */
	result = clean(context);
	if (result != FWU_STATUS_SUCCESS)
		return result;

	return FWU_STATUS_UNKNOWN;
}

static int end_staging(void *context)
{
	struct psa_fwu_m_update_agent *agent = (struct psa_fwu_m_update_agent *)context;
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	struct psa_fwu_m_image *image = NULL;
	bool all_images_accepted = true;
	size_t i = 0;

	if (agent->state != staging)
		return FWU_STATUS_DENIED;

	/* Check if there are open image handles */
	for (i = 0; i < ARRAY_SIZE(agent->handles); i++)
		if (agent->handles[i].used)
			return FWU_STATUS_BUSY;

	/* Finish images which were selected for staging */
	for (i = 0; i < agent->image_count; i++) {
		image = &agent->images[i];

		if (!image->selected_for_staging)
			continue;

		psa_status = psa_fwu_finish(image->component);
		if (psa_status != PSA_SUCCESS)
			return psa_status_to_fwu_status(psa_status);

		if (!image->accepted)
			all_images_accepted = false;
	}

	/* Invoke install step */
	psa_status = psa_fwu_install();
	if (psa_status != PSA_SUCCESS)
		return psa_status_to_fwu_status(psa_status);

	if (all_images_accepted) {
		/* If all images are accepted then accept the update and jump to regular state */
		psa_status = psa_fwu_accept();
		if (psa_status != PSA_SUCCESS)
			return psa_status_to_fwu_status(psa_status);

		set_agent_state(agent, regular);
	} else {
		/* There are images which are not accepted, switch to trial state */
		set_agent_state(agent, trial);
	}

	return FWU_STATUS_SUCCESS;
}

static int cancel_staging(void *context)
{
	struct psa_fwu_m_update_agent *agent = (struct psa_fwu_m_update_agent *)context;
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	size_t i = 0;

	if (agent->state != staging)
		return FWU_STATUS_DENIED;

	/* Close all images */
	for (i = 0; i < ARRAY_SIZE(agent->handles); i++)
		free_handle(&agent->handles[i]);

	/* Cancel all images */
	for (i = 0; i < agent->image_count; i++) {
		struct psa_fwu_m_image *image = &agent->images[i];

		if (!image->selected_for_staging)
			continue;

		psa_status = psa_fwu_cancel(image->component);
		if (psa_status != PSA_SUCCESS)
			return psa_status_to_fwu_status(psa_status);

		image->selected_for_staging = false;
		image->accepted = false;
	}

	set_agent_state(agent, regular);

	return FWU_STATUS_SUCCESS;
}

static int open(void *context, const struct uuid_octets *uuid, uint8_t op_type, uint32_t *handle)
{
	struct psa_fwu_m_update_agent *agent = (struct psa_fwu_m_update_agent *)context;
	struct psa_fwu_m_image *image = NULL;

	/* Write operation is only allowed in staging state */
	if (op_type == FWU_OP_TYPE_WRITE && agent->state != staging)
		return FWU_STATUS_DENIED;

	image = find_image(agent, uuid);
	if (!image)
		return FWU_STATUS_UNKNOWN;

	/* Check if the image was selected for staging of opening for write */
	if (op_type == FWU_OP_TYPE_WRITE && !image->selected_for_staging)
		return FWU_STATUS_DENIED;

	/* Check if the image supports the required operation type */
	if ((op_type == FWU_OP_TYPE_READ && !image->read) ||
	    (op_type == FWU_OP_TYPE_WRITE && !image->write))
		return FWU_STATUS_NOT_AVAILABLE;

	*handle = allocate_handle(agent, image, op_type);
	if (*handle == FWU_INVALID_HANDLE)
		return FWU_STATUS_NOT_AVAILABLE;

	return FWU_STATUS_SUCCESS;
}

static int write_stream(void *context, uint32_t handle, const uint8_t *data, size_t data_len)
{
	struct psa_fwu_m_update_agent *agent = (struct psa_fwu_m_update_agent *)context;
	struct psa_fwu_m_handle *handle_desc = NULL;
	int result = FWU_STATUS_DENIED;
	size_t offset_after_write = 0;

	if (agent->state != staging)
		return FWU_STATUS_DENIED;

	handle_desc = get_handle(agent, handle);
	if (!handle_desc)
		return FWU_STATUS_UNKNOWN;

	if (handle_desc->op_type != FWU_OP_TYPE_WRITE)
		return FWU_STATUS_NO_PERMISSION;

	if (ADD_OVERFLOW(handle_desc->current_offset, data_len, &offset_after_write))
		return FWU_STATUS_OUT_OF_BOUNDS;

	if (!handle_desc->image->selected_for_staging || !handle_desc->image->write)
		return FWU_STATUS_DENIED; /* LCOV_EXCL_LINE */

	result = handle_desc->image->write(agent, handle_desc->image, handle_desc->current_offset,
					   data, data_len);
	if (result != FWU_STATUS_SUCCESS)
		return result;

	handle_desc->current_offset = offset_after_write;

	return FWU_STATUS_SUCCESS;
}

static int read_stream(void *context, uint32_t handle, uint8_t *buf, size_t buf_size,
		       size_t *read_len, size_t *total_len)
{
	struct psa_fwu_m_update_agent *agent = (struct psa_fwu_m_update_agent *)context;
	struct psa_fwu_m_handle *handle_desc = NULL;

	handle_desc = get_handle(agent, handle);
	if (!handle_desc)
		return FWU_STATUS_UNKNOWN;

	if (handle_desc->op_type != FWU_OP_TYPE_READ)
		return FWU_STATUS_NO_PERMISSION;

	if (!handle_desc->image->read)
		return FWU_STATUS_DENIED; /* LCOV_EXCL_LINE */

	return handle_desc->image->read(agent, handle_desc->image, buf, buf_size, read_len,
					total_len);
}

static int commit(void *context, uint32_t handle, bool accepted, uint32_t max_atomic_len,
		  uint32_t *progress, uint32_t *total_work)
{
	struct psa_fwu_m_update_agent *agent = (struct psa_fwu_m_update_agent *)context;
	struct psa_fwu_m_handle *handle_desc = NULL;

	handle_desc = get_handle(agent, handle);
	if (!handle_desc)
		return FWU_STATUS_UNKNOWN;

	if (handle_desc->image->selected_for_staging)
		handle_desc->image->accepted = accepted;

	free_handle(handle_desc);

	*progress = 1;
	*total_work = 1;

	return FWU_STATUS_SUCCESS;
}

static int accept(void *context, const struct uuid_octets *image_type_uuid)
{
	struct psa_fwu_m_update_agent *agent = (struct psa_fwu_m_update_agent *)context;
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	struct psa_fwu_m_image *image = NULL;
	size_t i = 0;

	if (agent->state != trial)
		return FWU_STATUS_DENIED;

	image = find_image(agent, image_type_uuid);
	if (!image)
		return FWU_STATUS_UNKNOWN;

	if (!image->selected_for_staging)
		return FWU_STATUS_DENIED;

	image->accepted = true;

	/* Accept update if all images has been accepted */
	for (i = 0; i < agent->image_count; i++) {
		image = &agent->images[i];

		if (!image->selected_for_staging)
			continue;

		if (!image->accepted)
			return FWU_STATUS_SUCCESS;
	}

	psa_status = psa_fwu_accept();
	if (psa_status != PSA_SUCCESS)
		return psa_status_to_fwu_status(psa_status);

	set_agent_state(agent, regular);

	return FWU_STATUS_SUCCESS;
}

static int select_previous(void *context)
{
	struct psa_fwu_m_update_agent *agent = (struct psa_fwu_m_update_agent *)context;
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;

	if (agent->state != trial)
		return FWU_STATUS_DENIED;

	psa_status = psa_fwu_reject(0);
	if (psa_status != PSA_SUCCESS)
		return psa_status_to_fwu_status(psa_status);

	set_agent_state(agent, regular);

	return FWU_STATUS_SUCCESS;
}

static const struct update_agent_interface interface = {
	.discover = discover,
	.begin_staging = begin_staging,
	.end_staging = end_staging,
	.cancel_staging = cancel_staging,
	.open = open,
	.write_stream = write_stream,
	.read_stream = read_stream,
	.commit = commit,
	.accept_image = accept,
	.select_previous = select_previous,
};

struct update_agent *psa_fwu_m_update_agent_init(
	const struct psa_fwu_m_image_mapping image_mapping[], size_t image_count,
	uint32_t max_payload_size)
{
	struct psa_fwu_m_update_agent *context = NULL;
	struct psa_fwu_m_image *images = NULL;
	struct update_agent *agent = NULL;
	size_t i = 0;

	/* Allocate +1 image for the Image directory */
	images = (struct psa_fwu_m_image *)calloc(image_count + 1, sizeof(*images));
	if (!images)
		return NULL; /* LCOV_EXCL_LINE */

	context = (struct psa_fwu_m_update_agent *)calloc(1, sizeof(*context));
	if (!context) {
		/* LCOV_EXCL_START */
		free(images);
		return NULL;
		/* LCOV_EXCL_STOP */
	}

	agent = (struct update_agent *)calloc(1, sizeof(*agent));
	if (!agent) {
		/* LCOV_EXCL_START */
		free(images);
		free(context);
		return NULL;
		/* LCOV_EXCL_STOP */
	}

	for (i = 0; i < image_count; i++) {
		images[i].uuid = image_mapping[i].uuid;
		images[i].component = image_mapping[i].component;
		images[i].selected_for_staging = false;
		images[i].read = NULL; /* Cannot read images */
		images[i].write = image_write;
	}

	/* Insert Image directory as the last image */
	uuid_octets_from_canonical(&images[image_count].uuid, FWU_DIRECTORY_CANONICAL_UUID);
	images[image_count].component = 0;
	images[image_count].selected_for_staging = false;
	images[i].read = image_directory_read;
	images[i].write = NULL; /* Cannot write Images directory */

	context->images = images;
	context->image_count = image_count + 1;
	context->max_payload_size = max_payload_size;
	context->state = regular;

	agent->context = context;
	agent->interface = &interface;

	return agent;
}

void psa_fwu_m_update_agent_deinit(struct update_agent *update_agent)
{
	struct psa_fwu_m_update_agent *context =
		(struct psa_fwu_m_update_agent *)update_agent->context;

	free(context->images);
	free(context);
	free(update_agent);
}
