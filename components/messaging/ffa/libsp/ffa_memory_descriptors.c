// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2020-2021, Arm Limited and Contributors. All rights reserved.
 */

#include "ffa_memory_descriptors.h"
#include "ffa_api.h"
#include <assert.h>
#include <string.h>

static struct ffa_mem_transaction_desc *
get_mem_transaction_desc(struct ffa_mem_transaction_buffer *buffer)
{
	assert(sizeof(struct ffa_mem_transaction_desc) <= buffer->length);
	return (struct ffa_mem_transaction_desc *)buffer->buffer;
}

static uintptr_t get_offset_in_buffer(struct ffa_mem_transaction_buffer *buffer,
				      void *object)
{
	return (uintptr_t)(((uint8_t *)object) - ((uint8_t *)buffer->buffer));
}

static struct ffa_composite_mem_region_desc *
get_composite_desc_by_offset(struct ffa_mem_transaction_buffer *buffer,
			     uint32_t offset)
{
	void *ptr = NULL;

	assert(offset + sizeof(struct ffa_composite_mem_region_desc) <=
	       buffer->length);
	ptr = (((uint8_t *)buffer->buffer) + offset);

	return (struct ffa_composite_mem_region_desc *)ptr;
}

static uint32_t
get_composite_desc_offset(struct ffa_mem_transaction_buffer *buffer)
{
	const struct ffa_mem_access_desc *access_desc = NULL;

	if (ffa_get_mem_access_desc_count(buffer) == 0)
		return 0;

	/*
	 * All access descriptors have the same offset so we use the first one
	 * if it exists.
	 */
	access_desc = ffa_get_mem_access_desc(buffer, 0);
	return access_desc->composite_mem_region_desc_offset;
}

static struct ffa_composite_mem_region_desc *
get_composite_desc(struct ffa_mem_transaction_buffer *buffer)
{
	uint32_t offset = 0;

	offset = get_composite_desc_offset(buffer);

	return get_composite_desc_by_offset(buffer, offset);
}

#if CFG_FFA_VERSION >= FFA_VERSION_1_1
static struct ffa_mem_access_desc *get_mem_access_desc_by_offset(
	struct ffa_mem_transaction_buffer *buffer, uint32_t offset)
{
	void *ptr = NULL;

	assert(offset <= buffer->length);
	ptr = (((uint8_t *)buffer->buffer) + offset);

	return (struct ffa_mem_access_desc *)ptr;
}
#endif /* CFG_FFA_VERSION */

struct ffa_mem_access_desc *get_mem_access_desc(struct ffa_mem_transaction_buffer *buffer,
						uint32_t descriptor_index)
{
	struct ffa_mem_transaction_desc *transaction = NULL;
	struct ffa_mem_access_desc *access_desc = NULL;
	size_t offset = 0;
	size_t __maybe_unused mem_desc_offset = 0;

	transaction = get_mem_transaction_desc(buffer);

	assert(descriptor_index < transaction->mem_access_desc_count);

#if CFG_FFA_VERSION == FFA_VERSION_1_0
	access_desc = &transaction->mem_access_desc[descriptor_index];
#elif CFG_FFA_VERSION >= FFA_VERSION_1_1
	mem_desc_offset = transaction->mem_access_desc_offset;
	mem_desc_offset += transaction->mem_access_desc_size * descriptor_index;
	access_desc = get_mem_access_desc_by_offset(buffer, mem_desc_offset);
#endif /* CFG_FFA_VERSION */

	/* Validating if the whole descriptor is within the buffer boundaries */
	offset = get_offset_in_buffer(buffer, access_desc);
	offset += sizeof(struct ffa_mem_access_desc);
	assert(offset <= buffer->length);

	return access_desc;
}

void ffa_init_mem_transaction_buffer(void *address, size_t length,
				     struct ffa_mem_transaction_buffer *buffer)
{
	buffer->buffer = address;
	buffer->length = length;
	buffer->used = 0;
}

void ffa_init_mem_transaction_desc(struct ffa_mem_transaction_buffer *buffer,
				   uint16_t sender_id, uint16_t mem_region_attr,
				   uint32_t flags, uint64_t handle, uint64_t tag)
{
	struct ffa_mem_transaction_desc *transaction = NULL;

	transaction = get_mem_transaction_desc(buffer);

	memset(transaction, 0x00, sizeof(*transaction));

	transaction->sender_id = sender_id;
	transaction->mem_region_attr = mem_region_attr;
	transaction->flags = flags;
	transaction->handle = handle;
	transaction->tag = tag;
	transaction->mem_access_desc_count = 0;

#if CFG_FFA_VERSION >= FFA_VERSION_1_1
	transaction->mem_access_desc_size = sizeof(struct ffa_mem_access_desc);
	/* The memory access descriptors start right after the transaction descriptor */
	transaction->mem_access_desc_offset = sizeof(*transaction);
#endif /* CFG_FFA_VERSION */

	buffer->used = sizeof(*transaction);
}

const struct ffa_mem_transaction_desc *
ffa_get_mem_transaction_desc(struct ffa_mem_transaction_buffer *buffer)
{
	return get_mem_transaction_desc(buffer);
}

void ffa_reserve_mem_access_desc(struct ffa_mem_transaction_buffer *buffer,
				 size_t count)
{
	const struct ffa_mem_transaction_desc *transaction = NULL;
	size_t required_size = 0;
	uint32_t __maybe_unused composite_offset = 0;

	transaction = ffa_get_mem_transaction_desc(buffer);
	if (count <= transaction->mem_access_desc_count) {
		/* Prevent shrinking below the current count */
		return;
	}

	/* Checking if the descriptors fit in the buffer */
	required_size = sizeof(struct ffa_mem_transaction_desc);
	required_size += sizeof(struct ffa_mem_access_desc) * count;

	assert(required_size <= buffer->length);

	/*
	 * Checking if the descriptor array won't overlap with the composite
	 * memory region descriptor if it exists
	 */
	composite_offset = get_composite_desc_offset(buffer);
	/* The following assert's branches cannot be fully covered. */
	assert(composite_offset == 0 || required_size <= composite_offset);

	if (buffer->used < required_size)
		buffer->used = required_size;
}

uint32_t ffa_add_mem_access_desc(struct ffa_mem_transaction_buffer *buffer, uint16_t endpoint_id,
				 uint8_t mem_access_perm, uint8_t flags)
{
	struct ffa_mem_transaction_desc *transaction = NULL;
	struct ffa_mem_access_desc *access_desc = NULL;
	struct ffa_mem_access_perm_desc *access_perm_desc = NULL;
	uint32_t index = 0;
	uint32_t composite_offset = 0;
	size_t required_size = 0;
	size_t __maybe_unused mem_desc_offset = 0;

	/* Get next access descriptor index */
	transaction = get_mem_transaction_desc(buffer);
	index = transaction->mem_access_desc_count;

	assert(index != UINT32_MAX);

	/* Allocating access descriptor */
	required_size = sizeof(*transaction);
	required_size += sizeof(struct ffa_mem_access_desc) * (index + 1);
	assert(required_size <= buffer->length);

	/*
	 * Checking if the descriptor array won't overlap with the composite
	 * memory region descriptor if it exists
	 */
	composite_offset = get_composite_desc_offset(buffer);
	assert(composite_offset == 0 || required_size <= composite_offset);

	if (buffer->used < required_size)
		buffer->used = required_size;

	transaction->mem_access_desc_count++;

	/* Initializing access descriptor */
#if CFG_FFA_VERSION == FFA_VERSION_1_0
	access_desc = &transaction->mem_access_desc[index];
#elif CFG_FFA_VERSION >= FFA_VERSION_1_1
	mem_desc_offset = transaction->mem_access_desc_offset;
	mem_desc_offset += transaction->mem_access_desc_size * index;
	access_desc = get_mem_access_desc_by_offset(buffer, mem_desc_offset);
#endif
	access_perm_desc = &access_desc->mem_access_perm_desc;
	access_perm_desc->endpoint_id = endpoint_id;
	access_perm_desc->mem_access_permissions = mem_access_perm;
	access_perm_desc->flags = flags;

	/*
	 * Assigning the composite memory descriptor offset as it is either 0
	 * for indicating that the composite descriptor has not been added to
	 * the transaction buffer or setting it the point to the valid and
	 * only composite descriptor.
	 */
	access_desc->composite_mem_region_desc_offset = composite_offset;
	access_desc->reserved_mbz = 0;

	return index;
}

uint32_t
ffa_get_mem_access_desc_count(struct ffa_mem_transaction_buffer *buffer)
{
	const struct ffa_mem_transaction_desc *transaction = NULL;

	transaction = ffa_get_mem_transaction_desc(buffer);

	return transaction->mem_access_desc_count;
}

const struct ffa_mem_access_desc *ffa_get_mem_access_desc(struct ffa_mem_transaction_buffer *buffer,
							  uint32_t descriptor_index)
{
	return get_mem_access_desc(buffer, descriptor_index);
}

void ffa_add_memory_region(struct ffa_mem_transaction_buffer *buffer,
			   const void *address, uint32_t page_count)
{
	struct ffa_mem_transaction_desc *transaction = NULL;
	struct ffa_composite_mem_region_desc *comp_desc = NULL;
	struct ffa_constituent_mem_region_desc *region_desc = NULL;
	size_t required_size = 0;
	uint32_t range_index = 0;
	uint32_t offset = 0;

	transaction = get_mem_transaction_desc(buffer);

	assert(transaction->mem_access_desc_count > 0);

	offset = get_composite_desc_offset(buffer);

	if (offset == 0) {
		/* Adding new composite descriptor */
		struct ffa_mem_access_desc *access_desc = NULL;
		uint32_t i = 0;

		/*
		 * Allocate composite memory regions descriptor to the end of
		 * the currently used area
		 */
		required_size = buffer->used +
				sizeof(struct ffa_composite_mem_region_desc);

		/* Overflow and truncation checks */
		assert(required_size <= buffer->length);
		assert(buffer->used == (size_t)((uint32_t)buffer->used));

		offset = buffer->used;
		buffer->used = required_size;

		/*
		 * Setting composite memory region descriptor offset in all the
		 * memory access descriptors
		 */
		for (i = 0; i < transaction->mem_access_desc_count; i++) {
			access_desc = get_mem_access_desc(buffer, i);
			access_desc->composite_mem_region_desc_offset = offset;
		}

		/* Initializing composite descriptor values */
		comp_desc = get_composite_desc_by_offset(buffer, offset);
		comp_desc->total_page_count = 0;
		comp_desc->address_range_count = 0;
		comp_desc->reserved_mbz = 0;
	} else {
		/* The composite descriptor already exists */
		comp_desc = get_composite_desc_by_offset(buffer, offset);
	}

	range_index = comp_desc->address_range_count;
	required_size = buffer->used +
			sizeof(struct ffa_constituent_mem_region_desc);
	assert(required_size <= buffer->length);
	buffer->used = required_size;

	comp_desc->total_page_count += page_count;
	comp_desc->address_range_count++;

	region_desc = &comp_desc->constituent_mem_region_desc[range_index];

	region_desc->address = (uintptr_t)address;
	region_desc->page_count = page_count;
	region_desc->reserved_mbz = 0;
}

const struct ffa_composite_mem_region_desc *
ffa_get_memory_region(struct ffa_mem_transaction_buffer *buffer)
{
	struct ffa_composite_mem_region_desc *desc = NULL;
	size_t __maybe_unused end_offset = 0;

	desc = get_composite_desc(buffer);

	/*
	 * Checking if the constituent region descriptors are also in the buffer
	 */
	end_offset = ((uint8_t *)desc) - (uint8_t *)buffer->buffer;
	end_offset += sizeof(struct ffa_composite_mem_region_desc);
	end_offset += desc->address_range_count *
		      sizeof(struct ffa_constituent_mem_region_desc);
	assert(end_offset <= buffer->length);

	return desc;
}
