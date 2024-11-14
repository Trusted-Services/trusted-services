/*
 * Copyright (c) 2022-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "update_agent.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "common/uuid/uuid.h"
#include "img_dir_serializer.h"
#include "protocols/service/fwu/fwu_proto.h"
#include "protocols/service/fwu/status.h"
#include "service/fwu/fw_store/fw_store.h"
#include "service/fwu/inspector/fw_inspector.h"
#include "trace.h"

/**
 * \brief Update process states
 *
 * The update_agent is responsible for ensuring that only a valid update flow
 * is followed by a client. To enforce the flow, public operations can only be
 * used in a valid state that reflects the FWU-A behavioral model.
 */
enum fwu_state {
	FWU_STATE_DEINITIALZED,
	FWU_STATE_INITIALIZING,
	FWU_STATE_REGULAR,
	FWU_STATE_STAGING,
	FWU_STATE_TRIAL_PENDING,
	FWU_STATE_TRIAL
};

/**
 * \brief update_agent structure definition
 *
 * An update_agent instance is responsible for coordinating firmware updates applied
 * to a fw_store. An update_agent performs a security role by enforcing that a
 * valid flow is performed to update the fw store.
 */
struct generic_update_agent {
	enum fwu_state state;
	fw_inspector_inspect fw_inspect_method;
	struct fw_store *fw_store;
	struct fw_directory fw_directory;
	struct stream_manager stream_manager;
	uint8_t *image_dir_buf;
	size_t image_dir_buf_size;
};

static int cancel_staging(void *context);

static int discover(void *context, struct fwu_discovery_result *result)
{
	result->service_status = 0;
	result->version_major = FWU_PROTOCOL_VERSION_MAJOR;
	result->version_minor = FWU_PROTOCOL_VERSION_MINOR;
	result->max_payload_size = 0;
	result->flags = 0;
	result->vendor_specific_flags = 0;

	return FWU_STATUS_SUCCESS;
}

static int begin_staging(void *context, uint32_t vendor_flags, uint32_t partial_update_count,
			 const struct uuid_octets *update_guid)
{
	int status = FWU_STATUS_DENIED;
	struct generic_update_agent *update_agent = (struct generic_update_agent *)context;

	/* If already staging, any previous installation state is discarded */
	cancel_staging(update_agent);

	if (update_agent->state == FWU_STATE_REGULAR) {
		status = fw_store_begin_install(update_agent->fw_store);

		/* Check if ready to install images */
		if (status == FWU_STATUS_SUCCESS)
			update_agent->state = FWU_STATE_STAGING;
	}

	return status;
}

static int end_staging(void *context)
{
	int status = FWU_STATUS_DENIED;
	struct generic_update_agent *update_agent = (struct generic_update_agent *)context;

	if (update_agent->state == FWU_STATE_STAGING) {
		/* The client is responsible for committing each installed image. If any
		 * install streams have been left open, not all images were committed.
		 */
		bool any_uncommitted = stream_manager_is_open_streams(&update_agent->stream_manager,
								      FWU_STREAM_TYPE_INSTALL);

		if (!any_uncommitted) {
			/* All installed images have been committed so we're
			 * ready for a trial.
			 */
			status = fw_store_finalize_install(update_agent->fw_store);

			if (status == FWU_STATUS_SUCCESS)
				/* Transition to TRAIL_PENDING state. The trial actually starts
				 * when installed images are activated through a system restart.
				 */
				update_agent->state = FWU_STATE_TRIAL_PENDING;

		} else {
			/* Client failed to commit all images installed */
			status = FWU_STATUS_BUSY;
		}
	}

	return status;
}

static int cancel_staging(void *context)
{
	int status = FWU_STATUS_DENIED;
	struct generic_update_agent *update_agent = (struct generic_update_agent *)context;

	if (update_agent->state == FWU_STATE_STAGING) {
		stream_manager_cancel_streams(&update_agent->stream_manager,
					      FWU_STREAM_TYPE_INSTALL);

		fw_store_cancel_install(update_agent->fw_store);

		update_agent->state = FWU_STATE_REGULAR;

		status = FWU_STATUS_SUCCESS;
	}

	return status;
}

static int accept(void *context, const struct uuid_octets *image_type_uuid)
{
	int status = FWU_STATUS_DENIED;
	struct generic_update_agent *update_agent = (struct generic_update_agent *)context;

	if (update_agent->state == FWU_STATE_TRIAL) {
		const struct image_info *image_info =
			fw_directory_find_image_info(&update_agent->fw_directory, image_type_uuid);

		if (image_info) {
			if (fw_store_notify_accepted(update_agent->fw_store, image_info)) {
				/* From the fw_store perspective, the update has
				 * been fully accepted.
				 */
				status = fw_store_commit_to_update(update_agent->fw_store);
				update_agent->state = FWU_STATE_REGULAR;

			} else
				/* Still more images to accept */
				status = FWU_STATUS_SUCCESS;
		} else
			/* Unrecognised image uuid */
			status = FWU_STATUS_UNKNOWN;
	}

	return status;
}

static int select_previous(void *context)
{
	int status = FWU_STATUS_DENIED;
	struct generic_update_agent *update_agent = (struct generic_update_agent *)context;

	if ((update_agent->state == FWU_STATE_TRIAL) ||
	    (update_agent->state == FWU_STATE_TRIAL_PENDING)) {
		status = fw_store_revert_to_previous(update_agent->fw_store);
		update_agent->state = FWU_STATE_REGULAR;
	}

	return status;
}

static bool open_image_directory(struct generic_update_agent *update_agent,
				 const struct uuid_octets *uuid, uint32_t *handle, int *status)
{
	struct uuid_octets target_uuid;

	uuid_guid_octets_from_canonical(&target_uuid, FWU_DIRECTORY_CANONICAL_UUID);

	if (uuid_is_equal(uuid->octets, target_uuid.octets)) {
		/* Serialize a fresh view of the image directory */
		size_t serialized_len = 0;

		*status = img_dir_serializer_serialize(&update_agent->fw_directory,
						       update_agent->fw_store,
						       update_agent->image_dir_buf,
						       update_agent->image_dir_buf_size,
						       &serialized_len);

		if (*status == FWU_STATUS_SUCCESS) {
			*status = stream_manager_open_buffer_stream(&update_agent->stream_manager,
								    update_agent->image_dir_buf,
								    serialized_len, handle);
		}

		return true;
	}

	return false;
}

static bool open_fw_store_object(struct generic_update_agent *update_agent,
				 const struct uuid_octets *uuid, uint32_t *handle, int *status)
{
	const uint8_t *exported_data;
	size_t exported_data_len;

	if (fw_store_export(update_agent->fw_store, uuid, &exported_data, &exported_data_len,
			    status)) {
		if (*status == FWU_STATUS_SUCCESS) {
			*status = stream_manager_open_buffer_stream(&update_agent->stream_manager,
								    exported_data,
								    exported_data_len, handle);
		}

		return true;
	}

	return false;
}

static bool open_fw_image(struct generic_update_agent *update_agent, const struct uuid_octets *uuid,
			  uint32_t *handle, int *status)
{
	const struct image_info *image_info =
		fw_directory_find_image_info(&update_agent->fw_directory, uuid);

	if (image_info) {
		if (update_agent->state == FWU_STATE_STAGING) {
			struct installer *installer;

			*status = fw_store_select_installer(update_agent->fw_store, image_info,
							    &installer);

			if (*status == FWU_STATUS_SUCCESS) {
				*status = stream_manager_open_install_stream(
					&update_agent->stream_manager, update_agent->fw_store,
					installer, image_info, handle);
			}
		} else {
			/* Attempting to open a fw image when not staging */
			*status = FWU_STATUS_DENIED;
		}

		return true;
	}

	return false;
}

static int open(void *context, const struct uuid_octets *uuid, uint8_t op_type, uint32_t *handle)
{
	int status = FWU_STATUS_SUCCESS;
	struct generic_update_agent *update_agent = (struct generic_update_agent *)context;

	/* Pass UUID along a chain-of-responsibility until it's handled */
	if (!open_image_directory(update_agent, uuid, handle, &status) &&
	    !open_fw_store_object(update_agent, uuid, handle, &status) &&
	    !open_fw_image(update_agent, uuid, handle, &status)) {
		/* UUID not recognised */
		status = FWU_STATUS_UNKNOWN;
	}

	return status;
}

static int commit(void *context, uint32_t handle, bool accepted, uint32_t max_atomic_len,
		  uint32_t *progress, uint32_t *total_work)
{
	struct generic_update_agent *update_agent = (struct generic_update_agent *)context;
	int result = 0;

	result = stream_manager_close(&update_agent->stream_manager, handle, accepted);
	if (!result)
		*progress = 1;

	*total_work = 1;

	return result;
}

static int write_stream(void *context, uint32_t handle, const uint8_t *data, size_t data_len)
{
	struct generic_update_agent *update_agent = (struct generic_update_agent *)context;

	return stream_manager_write(&update_agent->stream_manager, handle, data, data_len);
}

static int read_stream(void *context, uint32_t handle, uint8_t *buf, size_t buf_size,
		       size_t *read_len, size_t *total_len)
{
	struct generic_update_agent *update_agent = (struct generic_update_agent *)context;

	return stream_manager_read(&update_agent->stream_manager, handle, buf, buf_size, read_len,
				   total_len);
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

static void deinit_context(struct generic_update_agent *context)
{
	if (!context)
		return;

	stream_manager_deinit(&context->stream_manager);
	fw_directory_deinit(&context->fw_directory);

	if (context->image_dir_buf)
		free(context->image_dir_buf);

	free(context);
}

struct update_agent *update_agent_init(unsigned int boot_index,
				       fw_inspector_inspect fw_inspect_method,
				       struct fw_store *fw_store)
{
	int status = FWU_STATUS_UNKNOWN;
	struct generic_update_agent *context = NULL;
	struct update_agent *agent = NULL;

	assert(fw_inspect_method);
	assert(fw_store);

	context = (struct generic_update_agent *)calloc(1, sizeof(*context));
	if (!context) {
		DMSG("Failed to allocate update agent context");
		return NULL;
	}

	context->state = FWU_STATE_INITIALIZING;
	context->fw_inspect_method = fw_inspect_method;
	context->fw_store = fw_store;
	context->image_dir_buf_size = 0;
	context->image_dir_buf = NULL;

	stream_manager_init(&context->stream_manager);

	/* Initialize and populate the fw_directory. The fw_inspector will
	 * obtain trustworthy information about the booted firmware and
	 * populate the fw_directory to reflect information about the booted
	 * firmware.
	 */
	fw_directory_init(&context->fw_directory);

	status = context->fw_inspect_method(&context->fw_directory, boot_index);
	if (status != FWU_STATUS_SUCCESS) {
		DMSG("Failed to run FW inspect: %d", status);
		deinit_context(context);
		return NULL;
	}

	/* Allow the associated fw_store to synchronize its state to the
	 * state of the booted firmware reflected by the fw_directory.
	 */
	status = fw_store_synchronize(context->fw_store, &context->fw_directory, boot_index);
	if (status != FWU_STATUS_SUCCESS) {
		DMSG("Failed synchronize FW store: %d", status);
		deinit_context(context);
		return NULL;
	}

	/* Allocate a buffer for holding the serialized image directory  */
	context->image_dir_buf_size = img_dir_serializer_get_len(&context->fw_directory);
	context->image_dir_buf = malloc(context->image_dir_buf_size);
	if (!context->image_dir_buf) {
		DMSG("Failed to allocate image_dir_buf");
		deinit_context(context);
		return NULL;
	}

	/* Transition to initial state */
	context->state = fw_store_is_trial(context->fw_store) ? FWU_STATE_TRIAL : FWU_STATE_REGULAR;

	agent = (struct update_agent *)calloc(1, sizeof(*agent));
	if (!agent) {
		DMSG("Failed to allocate update_agent");
		deinit_context(context);
		return NULL;
	}

	agent->context = context;
	agent->interface = &interface;

	return agent;
}

void update_agent_deinit(struct update_agent *update_agent)
{
	struct generic_update_agent *context = (struct generic_update_agent *)update_agent->context;

	deinit_context(context);
	free(update_agent);
}
