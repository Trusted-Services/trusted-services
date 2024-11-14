/*
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "stream_manager.h"

#include <stddef.h>
#include <string.h>

#include "protocols/service/fwu/status.h"
#include "service/fwu/fw_store/fw_store.h"

static uint32_t generate_handle(struct stream_manager *subject,
				const struct stream_context *const context)
{
	/* Handle includes rolling count value to protect against use of a stale handle */
	uint32_t new_handle = context - subject->contexts;

	new_handle = (new_handle & 0xffff) | (subject->rolling_count << 16);
	++subject->rolling_count;
	return new_handle;
}

static uint32_t index_from_handle(uint32_t handle)
{
	return handle & 0xffff;
}

static void add_to_free_list(struct stream_manager *subject, struct stream_context *context)
{
	context->type = FWU_STREAM_TYPE_NONE;
	context->handle = 0;
	context->next = subject->free;
	context->prev = NULL;
	subject->free = context;
}

static struct stream_context *alloc_stream_context(struct stream_manager *subject,
						   enum fwu_stream_type type, uint32_t *handle)
{
	struct stream_context *context = NULL;

	/* Re-cycle least-recently used context if there are no free contexts */
	if (!subject->free && subject->active_tail) {
		stream_manager_close(subject, subject->active_tail->handle, false);
	}

	/* Active contexts are held in a linked list in most recently allocated order */
	if (subject->free) {
		context = subject->free;
		subject->free = context->next;

		context->next = subject->active_head;
		context->prev = NULL;
		subject->active_head = context;

		if (!subject->active_tail)
			subject->active_tail = context;

		if (context->next)
			context->next->prev = context;

		context->type = type;

		context->handle = generate_handle(subject, context);
		*handle = context->handle;
	}

	return context;
}

static void free_stream_context(struct stream_manager *subject, struct stream_context *context)
{
	/* Remove from active list */
	if (context->prev)
		context->prev->next = context->next;
	else
		subject->active_head = context->next;

	if (context->next)
		context->next->prev = context->prev;
	else
		subject->active_tail = context->prev;

	/* Add to free list */
	add_to_free_list(subject, context);
}

static struct stream_context *get_active_context(struct stream_manager *subject, uint32_t handle)
{
	struct stream_context *context = NULL;
	uint32_t index = index_from_handle(handle);

	if ((index < FWU_STREAM_MANAGER_POOL_SIZE) &&
	    (subject->contexts[index].type != FWU_STREAM_TYPE_NONE) &&
	    (subject->contexts[index].handle == handle)) {
		/* Handle qualifies an active stream context */
		context = &subject->contexts[index];
	}

	return context;
}

void stream_manager_init(struct stream_manager *subject)
{
	subject->free = NULL;
	subject->active_head = NULL;
	subject->active_tail = NULL;
	subject->rolling_count = 0;

	for (size_t i = 0; i < FWU_STREAM_MANAGER_POOL_SIZE; i++)
		add_to_free_list(subject, &subject->contexts[i]);
}

void stream_manager_deinit(struct stream_manager *subject)
{
	(void)subject;
}

int stream_manager_open_buffer_stream(struct stream_manager *subject, const uint8_t *data,
				      size_t data_len, uint32_t *handle)
{
	struct stream_context *context;
	int status = FWU_STATUS_UNKNOWN;

	/* First cancel any buffer streams left open associated with
	 * the same source buffer. Concurrent stream access to data in
	 * a buffer is prevented to avoid the possibility of a buffer
	 * being updated while a read stream is open.
	 */
	for (size_t i = 0; i < FWU_STREAM_MANAGER_POOL_SIZE; i++) {
		context = &subject->contexts[i];

		if ((context->type == FWU_STREAM_TYPE_BUFFER) &&
		    (context->variant.buffer.data == data))
			free_stream_context(subject, context);
	}

	/* Allocate and initialize a new stream */
	context = alloc_stream_context(subject, FWU_STREAM_TYPE_BUFFER, handle);

	if (context) {
		context->variant.buffer.data = data;
		context->variant.buffer.data_len = data_len;
		context->variant.buffer.pos = 0;

		status = FWU_STATUS_SUCCESS;
	}

	return status;
}

int stream_manager_open_install_stream(struct stream_manager *subject, struct fw_store *fw_store,
				       struct installer *installer,
				       const struct image_info *image_info, uint32_t *stream_handle)
{
	struct stream_context *context;
	int status = FWU_STATUS_UNKNOWN;

	/* First cancel any install streams left open associated with
	 * the same fw_store and installer. This defends against the
	 * possibility of data written via two streams being written to the
	 * same installer, resulting in image corruption.
	 */
	for (size_t i = 0; i < FWU_STREAM_MANAGER_POOL_SIZE; i++) {
		context = &subject->contexts[i];

		if ((context->type == FWU_STREAM_TYPE_INSTALL) &&
		    (context->variant.install.fw_store == fw_store) &&
		    (context->variant.install.installer == installer))
			free_stream_context(subject, context);
	}

	/* Allocate and initialize a new stream */
	context = alloc_stream_context(subject, FWU_STREAM_TYPE_INSTALL, stream_handle);

	if (context) {
		context->variant.install.fw_store = fw_store;
		context->variant.install.installer = installer;
		context->variant.install.image_info = image_info;

		status = FWU_STATUS_SUCCESS;
	}

	return status;
}

int stream_manager_close(struct stream_manager *subject, uint32_t handle, bool accepted)
{
	int status = FWU_STATUS_UNKNOWN;
	struct stream_context *context = get_active_context(subject, handle);

	if (context) {
		status = FWU_STATUS_SUCCESS;

		if (context->type == FWU_STREAM_TYPE_INSTALL) {
			status = fw_store_commit_image(context->variant.install.fw_store,
						       context->variant.install.installer,
						       context->variant.install.image_info,
						       accepted);
		}

		free_stream_context(subject, context);
	}

	return status;
}

void stream_manager_cancel_streams(struct stream_manager *subject, enum fwu_stream_type type)
{
	for (size_t i = 0; i < FWU_STREAM_MANAGER_POOL_SIZE; i++) {
		struct stream_context *context = &subject->contexts[i];

		if (context->type == type)
			free_stream_context(subject, context);
	}
}

bool stream_manager_is_open_streams(const struct stream_manager *subject, enum fwu_stream_type type)
{
	bool any_open = false;

	for (size_t i = 0; i < FWU_STREAM_MANAGER_POOL_SIZE; i++) {
		const struct stream_context *context = &subject->contexts[i];

		if (context->type == type) {
			any_open = true;
			break;
		}
	}

	return any_open;
}

int stream_manager_write(struct stream_manager *subject, uint32_t handle, const uint8_t *data,
			 size_t data_len)
{
	int status = FWU_STATUS_UNKNOWN;
	struct stream_context *context = get_active_context(subject, handle);

	if (context && context->type == FWU_STREAM_TYPE_INSTALL) {
		status = fw_store_write_image(context->variant.install.fw_store,
					      context->variant.install.installer, data, data_len);
	}

	return status;
}

int stream_manager_read(struct stream_manager *subject, uint32_t handle, uint8_t *buf,
			size_t buf_size, size_t *read_len, size_t *total_len)
{
	int status = FWU_STATUS_UNKNOWN;
	struct stream_context *context = get_active_context(subject, handle);

	if (context) {
		if (context->type == FWU_STREAM_TYPE_BUFFER) {
			size_t pos = context->variant.buffer.pos;
			size_t remaining_len = context->variant.buffer.data_len - pos;
			size_t len_to_read = (remaining_len <= buf_size) ? remaining_len : buf_size;

			memcpy(buf, &context->variant.buffer.data[pos], len_to_read);

			*read_len = len_to_read;
			*total_len = context->variant.buffer.data_len;
			context->variant.buffer.pos = pos + len_to_read;

			status = FWU_STATUS_SUCCESS;
		} else {
			/* Reading from other types of stream is forbidden */
			status = FWU_STATUS_DENIED;
		}
	}

	return status;
}
