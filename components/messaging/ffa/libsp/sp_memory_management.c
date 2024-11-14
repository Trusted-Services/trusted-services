// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2020-2023, Arm Limited and Contributors. All rights reserved.
 */

#include "sp_memory_management.h"
#include "sp_api.h"
#include "sp_rxtx.h"
#include <assert.h>
#include <string.h>

#define MEM_HANDLE_UNUSED	UINT64_C(0)

static sp_result get_tx_buffer(struct ffa_mem_transaction_buffer *buffer)
{
	void *buffer_address = NULL;
	size_t buffer_size = 0;
	sp_result result = SP_RESULT_OK;

	result = sp_rxtx_buffer_tx_get(&buffer_address, &buffer_size);
	if (result != SP_RESULT_OK)
		return result;

	ffa_init_mem_transaction_buffer(buffer_address, buffer_size, buffer);

	return SP_RESULT_OK;
}

static sp_result get_rx_buffer(struct ffa_mem_transaction_buffer *buffer)
{
	void *buffer_address = NULL;
	size_t buffer_size = 0;
	sp_result result = SP_RESULT_OK;

	result = sp_rxtx_buffer_rx_get((const void **)&buffer_address,
				       &buffer_size);
	if (result != SP_RESULT_OK)
		return result;

	ffa_init_mem_transaction_buffer(buffer_address, buffer_size, buffer);

	return SP_RESULT_OK;
}

static bool is_valid_buffer(struct ffa_mem_transaction_buffer *buffer)
{
	return buffer &&
	       !(((uintptr_t)buffer->buffer) & FFA_MEM_TRANSACTION_PAGE_MASK) &&
	       !(((uintptr_t)buffer->length) & FFA_MEM_TRANSACTION_PAGE_MASK);
}

static uint16_t build_mem_region_attr(enum sp_memory_type type,
				     union sp_memory_attr *sp_mem_attr)
{
	uint16_t mem_attr = 0;

	assert(type != sp_memory_type_reserved);
	mem_attr = SHIFT_U32(type, FFA_MEM_REGION_ATTR_MEMORY_TYPE_SHIFT);

	if (type == sp_memory_type_device_memory) {
		mem_attr |= SHIFT_U32(sp_mem_attr->device_memory,
				      FFA_MEM_REGION_ATTR_DEVICE_MEM_ATTR_SHIFT
				      );
	} else if (type == sp_memory_type_normal_memory) {
		enum sp_cacheability_attribute cacheability =
			sp_cacheability_non_cacheable;
		enum sp_shareablity_attribute shareability =
			sp_shareability_non_shareable;

		cacheability = sp_mem_attr->normal_memory.cacheability;
		shareability = sp_mem_attr->normal_memory.shareability;

		assert(cacheability == sp_cacheability_non_cacheable ||
		       cacheability == sp_cacheability_write_back);
		assert(shareability != sp_shareability_reserved);

		mem_attr |= SHIFT_U32(cacheability,
				      FFA_MEM_REGION_ATTR_CACHEABILITY_SHIFT);
		mem_attr |= SHIFT_U32(shareability,
				      FFA_MEM_REGION_ATTR_SHAREABILITY_SHIFT);
	}

#if CFG_FFA_VERSION >= FFA_VERSION_1_1
	if (sp_mem_attr->security_state == sp_memory_security_state_non_secure)
		mem_attr |= FFA_MEM_REGION_ATTR_NS_BIT;
#endif /* CFG_FFA_VERSION */

	return mem_attr;
}

static void parse_mem_region_attr(uint16_t attr, enum sp_memory_type *type,
				  union sp_memory_attr *sp_mem_attr)
{
	uint8_t temp = 0;

	temp = (attr >> FFA_MEM_REGION_ATTR_MEMORY_TYPE_SHIFT) &
	       FFA_MEM_REGION_ATTR_MEMORY_TYPE_MASK;
	*type = (enum sp_memory_type)temp;

	if (*type == sp_memory_type_device_memory) {
		enum sp_device_memory_attributes *dev_attr = NULL;

		temp = (attr >> FFA_MEM_REGION_ATTR_DEVICE_MEM_ATTR_SHIFT) &
		       FFA_MEM_REGION_ATTR_DEVICE_MEM_ATTR_MASK;

		dev_attr = &sp_mem_attr->device_memory;
		*dev_attr = (enum sp_device_memory_attributes)temp;
	} else if (*type == sp_memory_type_normal_memory) {
		struct sp_normal_memory_attributes *norm_attr = NULL;

		norm_attr = &sp_mem_attr->normal_memory;

		temp = (attr >> FFA_MEM_REGION_ATTR_CACHEABILITY_SHIFT) &
		       FFA_MEM_REGION_ATTR_CACHEABILITY_MASK;
		norm_attr->cacheability = (enum sp_cacheability_attribute)temp;

		temp = (attr >> FFA_MEM_REGION_ATTR_SHAREABILITY_SHIFT) &
		       FFA_MEM_REGION_ATTR_SHAREABILITY_MASK;
		norm_attr->shareability = (enum sp_shareablity_attribute)temp;
	}

#if CFG_FFA_VERSION >= FFA_VERSION_1_1
	if (attr & FFA_MEM_REGION_ATTR_NS_BIT)
		sp_mem_attr->security_state = sp_memory_security_state_non_secure;
	else
		sp_mem_attr->security_state = sp_memory_security_state_secure;
#endif /* CFG_FFA_VERSION */
}

static uint8_t
build_mem_access_perm(enum sp_instruction_access_permission instruction_access,
		      enum sp_data_access_permission data_access)
{
	uint8_t mem_access_perm = 0;

	mem_access_perm = SHIFT_U32(instruction_access,
				    FFA_MEM_ACCESS_PERM_INSTRUCTION_SHIFT);
	mem_access_perm |= SHIFT_U32(data_access,
				     FFA_MEM_ACCESS_PERM_DATA_SHIFT);

	return mem_access_perm;
}

static void
parse_mem_access_perm(uint8_t mem_access_perm,
		      enum sp_instruction_access_permission *instruction_access,
		      enum sp_data_access_permission *data_access)
{
	uint8_t temp = 0;

	temp = (mem_access_perm >> FFA_MEM_ACCESS_PERM_INSTRUCTION_SHIFT) &
	       FFA_MEM_ACCESS_PERM_INSTRUCTION_MASK;
	*instruction_access = (enum sp_instruction_access_permission)temp;

	temp = (mem_access_perm >> FFA_MEM_ACCESS_PERM_DATA_SHIFT) &
	       FFA_MEM_ACCESS_PERM_DATA_MASK;
	*data_access = (enum sp_data_access_permission)temp;
}

static uint32_t build_mem_flags(struct sp_memory_transaction_flags *flags)
{
	uint32_t res = 0;

	if (flags->zero_memory)
		res |= FFA_MEM_TRANSACTION_FLAGS_ZERO_MEMORY;

	if (flags->operation_time_slicing)
		res |= FFA_MEM_TRANSACTION_FLAGS_OPERATION_TIME_SLICING;

	if (flags->zero_memory_after_relinquish)
		res |= FFA_MEM_TRANSACTION_FLAGS_ZERO_MEMORY_AFTER_RELINQIUSH;

	res |= SHIFT_U32(flags->transaction_type,
			 FFA_MEM_TRANSACTION_FLAGS_TYPE_SHIFT);

	if (flags->alignment_hint != 0) {
		assert(flags->alignment_hint <=
		       FFA_MEM_TRANSACTION_FLAGS_ALIGNMENT_HINT_MASK);

		res |= FFA_MEM_TRANSACTION_FLAGS_ALIGNMENT_HINT_VALID;
		res |= SHIFT_U32(flags->alignment_hint,
				 FFA_MEM_TRANSACTION_FLAGS_ALIGNMENT_HINT_SHIFT
				 );
	}

	return res;
}

static void parse_mem_flags(uint32_t raw,
			    struct sp_memory_transaction_flags *flags)
{
	flags->zero_memory = (raw & FFA_MEM_TRANSACTION_FLAGS_ZERO_MEMORY);
	flags->operation_time_slicing =
		(raw & FFA_MEM_TRANSACTION_FLAGS_OPERATION_TIME_SLICING);
	flags->zero_memory_after_relinquish =
		(raw & FFA_MEM_TRANSACTION_FLAGS_ZERO_MEMORY_AFTER_RELINQIUSH);
	flags->transaction_type =
		(raw >> FFA_MEM_TRANSACTION_FLAGS_TYPE_SHIFT) &
		FFA_MEM_TRANSACTION_FLAGS_TYPE_MASK;

	if (raw & FFA_MEM_TRANSACTION_FLAGS_ALIGNMENT_HINT_VALID) {
		uint32_t hint = 0;

		hint = (raw >> FFA_MEM_TRANSACTION_FLAGS_ALIGNMENT_HINT_SHIFT) &
		       FFA_MEM_TRANSACTION_FLAGS_ALIGNMENT_HINT_MASK;

		flags->alignment_hint = hint;
	} else {
		flags->alignment_hint = 0;
	}
}

static void setup_descriptors(struct ffa_mem_transaction_buffer *buffer,
			      struct sp_memory_descriptor *descriptor,
			      struct sp_memory_access_descriptor *acc_desc,
			      uint32_t acc_desc_count,
			      struct sp_memory_region regions[],
			      uint32_t region_count, uint64_t handle)
{
	uint16_t mem_region_attr = 0;
	uint8_t acc_perm = 0;
	uint32_t flags = 0;
	uint32_t i = 0;

	mem_region_attr = build_mem_region_attr(descriptor->memory_type,
						&descriptor->mem_region_attr);

	flags = build_mem_flags(&descriptor->flags);

	ffa_init_mem_transaction_desc(buffer, descriptor->sender_id,
				      mem_region_attr, flags, handle,
				      descriptor->tag);

	/* Adding memory access descriptors */
	for (i = 0; i < acc_desc_count; i++) {
		acc_perm = build_mem_access_perm(acc_desc[i].instruction_access,
						 acc_desc[i].data_access);

		/* Non-retrieval Borrower flag is not supported currently */
		ffa_add_mem_access_desc(buffer, acc_desc[i].receiver_id,
					acc_perm, 0);
	}

	/* Adding memory regions */
	for (i = 0; i < region_count; i++) {
		ffa_add_memory_region(buffer, regions[i].address,
				      regions[i].page_count);
	}
}

static void parse_descriptors(struct ffa_mem_transaction_buffer *buffer,
			      struct sp_memory_descriptor *descriptor,
			      struct sp_memory_access_descriptor *acc_desc,
			      uint32_t acc_desc_count,
			      struct sp_memory_region regions[],
			      uint32_t *region_count)
{
	uint32_t i = 0;
	const struct ffa_mem_transaction_desc *transaction = NULL;
	const struct ffa_mem_access_desc *acc = NULL;
	const struct ffa_composite_mem_region_desc *region_desc = NULL;
	uint16_t region_attr = 0;
	uint8_t acc_perm = 0;

	transaction = ffa_get_mem_transaction_desc(buffer);

	assert(descriptor->tag == transaction->tag);
	assert(transaction->mem_access_desc_count == 1);

	descriptor->sender_id = transaction->sender_id;

	region_attr = transaction->mem_region_attr;
	parse_mem_region_attr(region_attr, &descriptor->memory_type,
			      &descriptor->mem_region_attr);

	parse_mem_flags(transaction->flags, &descriptor->flags);

	/* Parsing memory access descriptors */
	assert(transaction->mem_access_desc_count <= acc_desc_count);
	for (i = 0; i < transaction->mem_access_desc_count; i++) {
		acc = ffa_get_mem_access_desc(buffer, i);

		acc_desc[i].receiver_id = acc->mem_access_perm_desc.endpoint_id;

		acc_perm = acc->mem_access_perm_desc.mem_access_permissions;
		parse_mem_access_perm(acc_perm, &acc_desc[i].instruction_access,
				      &acc_desc[i].data_access);
	}

	/* Parsing memory regions */
	region_desc = ffa_get_memory_region(buffer);
	assert(region_desc->address_range_count <= *region_count);
	*region_count = region_desc->address_range_count;

	for (i = 0; i < region_desc->address_range_count; i++) {
		const struct ffa_constituent_mem_region_desc *region = NULL;

		region = &region_desc->constituent_mem_region_desc[i];

		regions[i].address = (void *)region->address;
		regions[i].page_count = region->page_count;
	}
}

static sp_result sp_mem_is_dynamic_supported(uint32_t func_id, bool *support)
{
	struct ffa_interface_properties interface_props = { 0 };
	uint32_t *props = NULL;
	ffa_result result = FFA_OK;

	if (!support)
		return SP_RESULT_INVALID_PARAMETERS;

	result = ffa_features(func_id, &interface_props);
	if (result != FFA_OK) {
		*support = false;
		return SP_RESULT_FFA(result);
	}

	props = interface_props.interface_properties;

	*support = props[FFA_FEATURES_MEM_DYNAMIC_BUFFER_SUPPORT_INDEX] &
		   FFA_FEATURES_MEM_DYNAMIC_BUFFER_SUPPORT;

	return SP_RESULT_OK;
}

sp_result sp_memory_donate(struct sp_memory_descriptor *descriptor,
			   struct sp_memory_access_descriptor *acc_desc,
			   struct sp_memory_region regions[],
			   uint32_t region_count, uint64_t *handle)
{
	struct ffa_mem_transaction_buffer buffer = { 0 };
	sp_result sp_res = SP_RESULT_OK;
	ffa_result ffa_res = FFA_OK;

	if (!handle)
		return SP_RESULT_INVALID_PARAMETERS;

	if (!descriptor || !acc_desc || !regions || !region_count) {
		*handle = UINT64_C(0);
		return SP_RESULT_INVALID_PARAMETERS;
	}

	sp_res = get_tx_buffer(&buffer);
	if (sp_res != SP_RESULT_OK) {
		*handle = UINT64_C(0);
		return sp_res;
	}

	setup_descriptors(&buffer, descriptor, acc_desc, 1, regions,
			  region_count, MEM_HANDLE_UNUSED);

	ffa_res = ffa_mem_donate_rxtx(buffer.used, buffer.used, handle);

	return SP_RESULT_FFA(ffa_res);
}

sp_result sp_memory_donate_dynamic(struct sp_memory_descriptor *descriptor,
				   struct sp_memory_access_descriptor *acc_desc,
				   struct sp_memory_region regions[],
				   uint32_t region_count, uint64_t *handle,
				   struct ffa_mem_transaction_buffer *buffer)
{
	uint32_t page_count = 0;
	ffa_result ffa_res = FFA_OK;

	if (!handle)
		return SP_RESULT_INVALID_PARAMETERS;

	if (!descriptor || !acc_desc || !regions || !region_count ||
	    !is_valid_buffer(buffer)) {
		*handle = UINT64_C(0);
		return SP_RESULT_INVALID_PARAMETERS;
	}

	setup_descriptors(buffer, descriptor, acc_desc, 1, regions,
			  region_count, MEM_HANDLE_UNUSED);

	page_count = buffer->length / FFA_MEM_TRANSACTION_PAGE_SIZE;
	ffa_res = ffa_mem_donate(buffer->used, buffer->used, buffer->buffer,
				 page_count, handle);

	return SP_RESULT_FFA(ffa_res);
}

sp_result sp_memory_donate_dynamic_is_supported(bool *supported)
{
	return sp_mem_is_dynamic_supported(FFA_MEM_DONATE_32, supported);
}

sp_result sp_memory_lend(struct sp_memory_descriptor *descriptor,
			 struct sp_memory_access_descriptor acc_desc[],
			 uint32_t acc_desc_count,
			 struct sp_memory_region regions[],
			 uint32_t region_count, uint64_t *handle)
{
	struct ffa_mem_transaction_buffer buffer = { 0 };
	sp_result sp_res = SP_RESULT_OK;
	ffa_result ffa_res = FFA_OK;

	if (!handle)
		return SP_RESULT_INVALID_PARAMETERS;

	if (!descriptor || !acc_desc || !acc_desc_count || !regions ||
	    !region_count) {
		*handle = UINT64_C(0);
		return SP_RESULT_INVALID_PARAMETERS;
	}

	sp_res = get_tx_buffer(&buffer);
	if (sp_res != SP_RESULT_OK) {
		*handle = UINT64_C(0);
		return sp_res;
	}

	setup_descriptors(&buffer, descriptor, acc_desc, acc_desc_count,
			  regions, region_count, MEM_HANDLE_UNUSED);

	ffa_res = ffa_mem_lend_rxtx(buffer.used, buffer.used, handle);

	return SP_RESULT_FFA(ffa_res);
}

sp_result sp_memory_lend_dynamic(struct sp_memory_descriptor *descriptor,
				 struct sp_memory_access_descriptor acc_desc[],
				 uint32_t acc_desc_count,
				 struct sp_memory_region regions[],
				 uint32_t region_count, uint64_t *handle,
				 struct ffa_mem_transaction_buffer *buffer)
{
	uint32_t page_count = 0;
	ffa_result ffa_res = FFA_OK;

	if (!handle)
		return SP_RESULT_INVALID_PARAMETERS;

	if (!descriptor || !acc_desc || !acc_desc_count || !regions ||
	    !region_count || !is_valid_buffer(buffer)) {
		*handle = UINT64_C(0);
		return SP_RESULT_INVALID_PARAMETERS;
	}

	setup_descriptors(buffer, descriptor, acc_desc, acc_desc_count, regions,
			  region_count, MEM_HANDLE_UNUSED);

	page_count = buffer->length / FFA_MEM_TRANSACTION_PAGE_SIZE;
	ffa_res = ffa_mem_lend(buffer->used, buffer->used, buffer->buffer,
			       page_count, handle);

	return SP_RESULT_FFA(ffa_res);
}

sp_result sp_memory_lend_dynamic_is_supported(bool *supported)
{
	return sp_mem_is_dynamic_supported(FFA_MEM_LEND_32, supported);
}

sp_result sp_memory_share(struct sp_memory_descriptor *descriptor,
			  struct sp_memory_access_descriptor acc_desc[],
			  uint32_t acc_desc_count,
			  struct sp_memory_region regions[],
			  uint32_t region_count, uint64_t *handle)
{
	struct ffa_mem_transaction_buffer buffer = { 0 };
	sp_result sp_res = SP_RESULT_OK;
	ffa_result ffa_res = FFA_OK;

	if (!handle)
		return SP_RESULT_INVALID_PARAMETERS;

	if (!descriptor || !acc_desc || !acc_desc_count || !regions ||
	    !region_count) {
		*handle = UINT64_C(0);
		return SP_RESULT_INVALID_PARAMETERS;
	}

	sp_res = get_tx_buffer(&buffer);
	if (sp_res != SP_RESULT_OK) {
		*handle = UINT64_C(0);
		return sp_res;
	}

	setup_descriptors(&buffer, descriptor, acc_desc, acc_desc_count,
			  regions, region_count, MEM_HANDLE_UNUSED);

	ffa_res = ffa_mem_share_rxtx(buffer.used, buffer.used, handle);

	return SP_RESULT_FFA(ffa_res);
}

sp_result sp_memory_share_dynamic(struct sp_memory_descriptor *descriptor,
				  struct sp_memory_access_descriptor acc_desc[],
				  uint32_t acc_desc_count,
				  struct sp_memory_region regions[],
				  uint32_t region_count, uint64_t *handle,
				  struct ffa_mem_transaction_buffer *buffer)
{
	uint32_t page_count = 0;
	ffa_result ffa_res = FFA_OK;

	if (!handle)
		return SP_RESULT_INVALID_PARAMETERS;

	if (!descriptor || !acc_desc || !acc_desc_count || !regions ||
	    !region_count || !is_valid_buffer(buffer)) {
		*handle = UINT64_C(0);
		return SP_RESULT_INVALID_PARAMETERS;
	}

	setup_descriptors(buffer, descriptor, acc_desc, acc_desc_count, regions,
			  region_count, MEM_HANDLE_UNUSED);

	page_count = buffer->length / FFA_MEM_TRANSACTION_PAGE_SIZE;
	ffa_res = ffa_mem_share(buffer->used, buffer->used, buffer->buffer,
				page_count, handle);

	return SP_RESULT_FFA(ffa_res);
}

sp_result sp_memory_share_dynamic_is_supported(bool *supported)
{
	return sp_mem_is_dynamic_supported(FFA_MEM_SHARE_32, supported);
}

sp_result sp_memory_retrieve(struct sp_memory_descriptor *descriptor,
			     struct sp_memory_access_descriptor *acc_desc,
			     struct sp_memory_region regions[],
			     uint32_t in_region_count,
			     uint32_t *out_region_count, uint64_t handle)
{
	struct ffa_mem_transaction_buffer tx_buffer = { 0 };
	struct ffa_mem_transaction_buffer rx_buffer = { 0 };
	sp_result sp_res = SP_RESULT_OK;
	ffa_result ffa_res = FFA_OK;
	uint32_t resp_total_length = 0;
	uint32_t resp_fragment_length = 0;

	if (!out_region_count)
		return SP_RESULT_INVALID_PARAMETERS;

	/* in_region_count can be 0 */
	if (!descriptor || !acc_desc || !regions) {
		*out_region_count = UINT32_C(0);
		return SP_RESULT_INVALID_PARAMETERS;
	}

	sp_res = get_tx_buffer(&tx_buffer);
	if (sp_res != SP_RESULT_OK) {
		*out_region_count = UINT32_C(0);
		return sp_res;
	}

	sp_res = get_rx_buffer(&rx_buffer);
	if (sp_res != SP_RESULT_OK) {
		*out_region_count = UINT32_C(0);
		return sp_res;
	}

	setup_descriptors(&tx_buffer, descriptor, acc_desc, 1, regions,
			  in_region_count, handle);

	ffa_res = ffa_mem_retrieve_req_rxtx(tx_buffer.used, tx_buffer.used,
					    &resp_total_length,
					    &resp_fragment_length);

	if (ffa_res != FFA_OK) {
		*out_region_count = UINT32_C(0);
		return SP_RESULT_FFA(ffa_res);
	}

	/* Fragmentation is not supported currently */
	if (resp_total_length != resp_fragment_length) {
		*out_region_count = UINT32_C(0);
		sp_res = SP_RESULT_INTERNAL_ERROR;
		goto out;
	}

	rx_buffer.used = resp_total_length;
	parse_descriptors(&rx_buffer, descriptor, acc_desc, 1, regions,
			  out_region_count);

out:
	ffa_res = ffa_rx_release();
	if (ffa_res != FFA_OK) {
		/* Keep original error when there was already an error.*/
		if (sp_res == SP_RESULT_OK)
			return SP_RESULT_FFA(ffa_res);
	}

	return sp_res;
}

sp_result
sp_memory_retrieve_dynamic(struct sp_memory_descriptor *descriptor,
			   struct sp_memory_access_descriptor *acc_desc,
			   struct sp_memory_region regions[],
			   uint32_t in_region_count, uint32_t *out_region_count,
			   uint64_t handle,
			   struct ffa_mem_transaction_buffer *buffer)
{
	sp_result sp_res = SP_RESULT_OK;
	uint32_t page_count = 0;
	uint32_t resp_total_length = 0;
	uint32_t resp_fragment_length = 0;

	if (!out_region_count)
		return SP_RESULT_INVALID_PARAMETERS;

	/* in_region_count can be 0 */
	if (!descriptor || !acc_desc || !regions || !handle ||
	    !is_valid_buffer(buffer)) {
		*out_region_count = UINT32_C(0);
		return SP_RESULT_INVALID_PARAMETERS;
	}

	setup_descriptors(buffer, descriptor, acc_desc, 1, regions,
			  in_region_count, handle);

	page_count = buffer->length / FFA_MEM_TRANSACTION_PAGE_SIZE;
	sp_res = ffa_mem_retrieve_req(buffer->used, buffer->used,
				      buffer->buffer, page_count,
				      &resp_total_length,
				      &resp_fragment_length);

	if (sp_res != FFA_OK) {
		*out_region_count = UINT32_C(0);
		return SP_RESULT_FFA(sp_res);
	}

	/* Fragmentation is not supported currently */
	if (resp_total_length != resp_fragment_length) {
		*out_region_count = UINT32_C(0);
		return SP_RESULT_INTERNAL_ERROR;
	}

	/* Same buffer is used for both TX and RX directions */
	buffer->used = resp_total_length;
	parse_descriptors(buffer, descriptor, acc_desc, 1, regions,
			  out_region_count);

	return SP_RESULT_OK;
}

sp_result sp_memory_retrieve_dynamic_is_supported(bool *supported)
{
	return sp_mem_is_dynamic_supported(FFA_MEM_RETRIEVE_REQ_32, supported);
}

sp_result sp_memory_relinquish(uint64_t handle, const uint16_t endpoints[],
			       uint32_t endpoint_count,
			       struct sp_memory_transaction_flags *flags)
{
	struct ffa_mem_transaction_buffer buffer = { 0 };
	sp_result sp_res = SP_RESULT_OK;
	ffa_result ffa_res = FFA_OK;
	struct ffa_mem_relinquish_desc *relinquish_desc = NULL;
	size_t required_size = 0;

	if (!endpoints || !endpoint_count || !flags)
		return SP_RESULT_INVALID_PARAMETERS;

	sp_res = get_tx_buffer(&buffer);
	if (sp_res != SP_RESULT_OK)
		return sp_res;

	required_size = sizeof(struct ffa_mem_relinquish_desc);
	required_size += endpoint_count * sizeof(uint16_t);

	assert(required_size < buffer.length);

	relinquish_desc = (struct ffa_mem_relinquish_desc *)buffer.buffer;
	relinquish_desc->handle = handle;
	relinquish_desc->flags = build_mem_flags(flags);
	relinquish_desc->endpoint_count = endpoint_count;
	memcpy(relinquish_desc->endpoints, endpoints,
	       endpoint_count * sizeof(uint16_t));

	ffa_res = ffa_mem_relinquish();

	return SP_RESULT_FFA(ffa_res);
}

sp_result sp_memory_reclaim(uint64_t handle, uint32_t flags)
{
	return SP_RESULT_FFA(ffa_mem_reclaim(handle, flags));
}

sp_result sp_memory_permission_get(const void *base_address,
				   struct sp_mem_perm *mem_perm)
{
	ffa_result result = FFA_OK;
	uint32_t raw_mem_perm = 0;

	/* Checking for invalid parameters*/
	if (!base_address || !mem_perm)
		return SP_RESULT_INVALID_PARAMETERS;

	/* Checking address alignment */
	if (((uintptr_t)base_address & FFA_MEM_PERM_PAGE_MASK) != 0)
		return SP_RESULT_INVALID_PARAMETERS;

	/* Mapping the buffers */
	result = ffa_mem_perm_get(base_address, &raw_mem_perm);
	if (result != FFA_OK)
		return SP_RESULT_FFA(result);

	/* Parsing permissions */
	switch (raw_mem_perm & FFA_MEM_PERM_DATA_ACCESS_PERM_MASK) {
	case FFA_MEM_PERM_DATA_ACCESS_PERM_NO_ACCESS:
		mem_perm->data_access = sp_mem_perm_data_perm_no_access;
		break;
	case FFA_MEM_PERM_DATA_ACCESS_PERM_RW:
		mem_perm->data_access = sp_mem_perm_data_perm_read_write;
		break;
	case FFA_MEM_PERM_DATA_ACCESS_PERM_RO:
		mem_perm->data_access = sp_mem_perm_data_perm_read_only;
		break;
	default:
		return SP_RESULT_INTERNAL_ERROR;
	}

	if ((raw_mem_perm & FFA_MEM_PERM_INSTRUCTION_ACCESS_PERM_MASK) ==
	    FFA_MEM_PERM_INSTRUCTION_ACCESS_PERM_X)
		mem_perm->instruction_access = sp_mem_perm_instruction_perm_executable;
	else
		mem_perm->instruction_access = sp_mem_perm_instruction_perm_non_executable;

	return SP_RESULT_OK;
}

sp_result sp_memory_permission_set(const void *base_address, size_t region_size,
				   const struct sp_mem_perm *mem_perm)
{
	uint32_t raw_mem_perm = 0;

	/* Checking for invalid parameters*/
	if (!base_address || !region_size || !mem_perm)
		return SP_RESULT_INVALID_PARAMETERS;

	/* RW and X permissions is prohibited */
	if (mem_perm->data_access == sp_mem_perm_data_perm_read_write &&
	    mem_perm->instruction_access == sp_mem_perm_instruction_perm_executable)
		return SP_RESULT_INVALID_PARAMETERS;

	switch (mem_perm->data_access) {
	case sp_mem_perm_data_perm_no_access:
		raw_mem_perm |= FFA_MEM_PERM_DATA_ACCESS_PERM_NO_ACCESS <<
				FFA_MEM_ACCESS_PERM_DATA_SHIFT;
		break;
	case sp_mem_perm_data_perm_read_write:
		raw_mem_perm |= FFA_MEM_PERM_DATA_ACCESS_PERM_RW <<
				FFA_MEM_ACCESS_PERM_DATA_SHIFT;
		break;
	case sp_mem_perm_data_perm_read_only:
		raw_mem_perm |= FFA_MEM_PERM_DATA_ACCESS_PERM_RO <<
				FFA_MEM_ACCESS_PERM_DATA_SHIFT;
		break;
	default:
		return SP_RESULT_INVALID_PARAMETERS;
	}

	switch (mem_perm->instruction_access) {
	case sp_mem_perm_instruction_perm_executable:
		raw_mem_perm |= FFA_MEM_PERM_INSTRUCTION_ACCESS_PERM_X;
		break;
	case sp_mem_perm_instruction_perm_non_executable:
		raw_mem_perm |= FFA_MEM_PERM_INSTRUCTION_ACCESS_PERM_NX;
		break;
	default:
		return SP_RESULT_INVALID_PARAMETERS;
	}

	/* Checking address alignment */
	if (((uintptr_t)base_address & FFA_MEM_PERM_PAGE_MASK) != 0 ||
	    (region_size & FFA_MEM_PERM_PAGE_MASK) != 0)
		return SP_RESULT_INVALID_PARAMETERS;

	/* Calculating page count */
	region_size /= FFA_MEM_PERM_PAGE_SIZE;
	if (region_size > FFA_MEM_PERM_PAGE_COUNT_MAX)
		return SP_RESULT_INVALID_PARAMETERS;

	/* Changing memory permissions */
	return SP_RESULT_FFA(ffa_mem_perm_set(base_address, region_size, raw_mem_perm));
}
