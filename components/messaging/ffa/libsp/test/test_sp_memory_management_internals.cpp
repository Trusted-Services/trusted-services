// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2020-2021, Arm Limited. All rights reserved.
 */

#include <CppUTestExt/MockSupport.h>
#include <CppUTest/TestHarness.h>
#include "../include/sp_memory_management.h"
#include "mock_assert.h"
#include "mock_ffa_api.h"
#include "mock_sp_rxtx.h"
#include <setjmp.h>
#include <stdint.h>
#include <string.h>

/* Picked functions */
extern "C" {
bool is_valid_buffer(struct ffa_mem_transaction_buffer *buffer);
uint8_t build_mem_region_attr(enum sp_memory_type type,
			      union sp_memory_attr *sp_mem_attr);
void parse_mem_region_attr(uint8_t attr, enum sp_memory_type *type,
			   union sp_memory_attr *sp_mem_attr);
uint32_t build_mem_flags(struct sp_memory_transaction_flags *flags);
void parse_mem_flags(uint32_t raw, struct sp_memory_transaction_flags *flags);
void parse_descriptors(struct ffa_mem_transaction_buffer *buffer,
		       struct sp_memory_descriptor *descriptor,
		       struct sp_memory_access_descriptor *acc_desc,
		       uint32_t acc_desc_count,
		       struct sp_memory_region regions[],
		       uint32_t *region_count);
}

TEST_GROUP(sp_memory_management_internals)
{
	TEST_SETUP()
	{
		memset(&attr, 0x00, sizeof(attr));
		memset(&flags, 0x00, sizeof(flags));
	}

	TEST_TEARDOWN()
	{
		mock().checkExpectations();
		mock().clear();
	}

	union sp_memory_attr attr;
	struct sp_memory_transaction_flags flags;
};

TEST(sp_memory_management_internals, is_valid_buffer_null)
{
	CHECK_FALSE(is_valid_buffer(NULL));
}

TEST(sp_memory_management_internals, is_valid_buffer_address_not_aligned)
{
	struct ffa_mem_transaction_buffer buffer = { .buffer = (void *)1,
						     .length = 0 };
	CHECK_FALSE(is_valid_buffer(&buffer));
}

TEST(sp_memory_management_internals, is_valid_buffer_size_not_aligned)
{
	struct ffa_mem_transaction_buffer buffer = { .buffer = (void *)0,
						     .length = 1 };
	CHECK_FALSE(is_valid_buffer(&buffer));
}

TEST(sp_memory_management_internals, is_valid_buffer)
{
	struct ffa_mem_transaction_buffer buffer = { .buffer = (void *)0,
						     .length = 0 };
	CHECK_TRUE(is_valid_buffer(&buffer));
}

TEST(sp_memory_management_internals, build_mem_region_attr_reserved_type)
{
	assert_environment_t env;

	attr.device_memory = sp_device_memory_nGnRnE;

	if (SETUP_ASSERT_ENVIRONMENT(env)) {
		build_mem_region_attr(sp_memory_type_reserved, &attr);
	}
}

TEST(sp_memory_management_internals, build_mem_region_attr_device_nGnRnE)
{
	uint8_t result = 0;
	uint8_t expected_result = 0;

	attr.device_memory = sp_device_memory_nGnRnE;

	expected_result = FFA_MEM_REGION_ATTR_MEMORY_TYPE_DEVICE
			  << FFA_MEM_REGION_ATTR_MEMORY_TYPE_SHIFT;
	expected_result |= FFA_MEM_REGION_ATTR_DEVICE_MEM_ATTR_NGNRNE
			   << FFA_MEM_REGION_ATTR_DEVICE_MEM_ATTR_SHIFT;

	result = build_mem_region_attr(sp_memory_type_device_memory, &attr);
	BYTES_EQUAL(expected_result, result);
}

TEST(sp_memory_management_internals, build_mem_region_attr_device_nGnRE)
{
	uint8_t result = 0;
	uint8_t expected_result = 0;

	attr.device_memory = sp_device_memory_nGnRE;

	expected_result = FFA_MEM_REGION_ATTR_MEMORY_TYPE_DEVICE
			  << FFA_MEM_REGION_ATTR_MEMORY_TYPE_SHIFT;
	expected_result |= FFA_MEM_REGION_ATTR_DEVICE_MEM_ATTR_NGNRE
			   << FFA_MEM_REGION_ATTR_DEVICE_MEM_ATTR_SHIFT;

	result = build_mem_region_attr(sp_memory_type_device_memory, &attr);
	BYTES_EQUAL(expected_result, result);
}

TEST(sp_memory_management_internals, build_mem_region_attr_device_nGRE)
{
	uint8_t result = 0;
	uint8_t expected_result = 0;

	attr.device_memory = sp_device_memory_nGRE;

	expected_result = FFA_MEM_REGION_ATTR_MEMORY_TYPE_DEVICE
			  << FFA_MEM_REGION_ATTR_MEMORY_TYPE_SHIFT;
	expected_result |= FFA_MEM_REGION_ATTR_DEVICE_MEM_ATTR_NGRE
			   << FFA_MEM_REGION_ATTR_DEVICE_MEM_ATTR_SHIFT;

	result = build_mem_region_attr(sp_memory_type_device_memory, &attr);
	BYTES_EQUAL(expected_result, result);
}

TEST(sp_memory_management_internals, build_mem_region_attr_device_GRE)
{
	uint8_t result = 0;
	uint8_t expected_result = 0;

	attr.device_memory = sp_device_memory_GRE;

	expected_result = FFA_MEM_REGION_ATTR_MEMORY_TYPE_DEVICE
			  << FFA_MEM_REGION_ATTR_MEMORY_TYPE_SHIFT;
	expected_result |= FFA_MEM_REGION_ATTR_DEVICE_MEM_ATTR_GRE
			   << FFA_MEM_REGION_ATTR_DEVICE_MEM_ATTR_SHIFT;

	result = build_mem_region_attr(sp_memory_type_device_memory, &attr);
	BYTES_EQUAL(expected_result, result);
}

TEST(sp_memory_management_internals,
     build_mem_region_attr_normal_cache_reserved0)
{
	assert_environment_t env;

	attr.normal_memory.cacheability = sp_cacheability_reserved0;
	attr.normal_memory.shareability = sp_shareability_non_shareable;

	if (SETUP_ASSERT_ENVIRONMENT(env)) {
		build_mem_region_attr(sp_memory_type_normal_memory, &attr);
	}
}

TEST(sp_memory_management_internals,
     build_mem_region_attr_normal_cache_reserved2)
{
	assert_environment_t env;

	attr.normal_memory.cacheability = sp_cacheability_reserved2;
	attr.normal_memory.shareability = sp_shareability_non_shareable;

	if (SETUP_ASSERT_ENVIRONMENT(env)) {
		build_mem_region_attr(sp_memory_type_normal_memory, &attr);
	}
}

TEST(sp_memory_management_internals,
     build_mem_region_attr_normal_share_reserved)
{
	assert_environment_t env;

	attr.normal_memory.cacheability = sp_cacheability_non_cacheable;
	attr.normal_memory.shareability = sp_shareability_reserved;

	if (SETUP_ASSERT_ENVIRONMENT(env)) {
		build_mem_region_attr(sp_memory_type_normal_memory, &attr);
	}
}

TEST(sp_memory_management_internals, build_mem_region_attr_normal_non_non)
{
	uint8_t result = 0;
	uint8_t expected_result = 0;

	attr.normal_memory.cacheability = sp_cacheability_non_cacheable;
	attr.normal_memory.shareability = sp_shareability_non_shareable;

	expected_result = FFA_MEM_REGION_ATTR_MEMORY_TYPE_NORMAL
			  << FFA_MEM_REGION_ATTR_MEMORY_TYPE_SHIFT;
	expected_result |= FFA_MEM_REGION_ATTR_CACHEABILITY_NON_CACHEABLE
			   << FFA_MEM_REGION_ATTR_CACHEABILITY_SHIFT;
	expected_result |= FFA_MEM_REGION_ATTR_SHAREABILITY_NON_SHAREABLE
			   << FFA_MEM_REGION_ATTR_SHAREABILITY_SHIFT;

	result = build_mem_region_attr(sp_memory_type_normal_memory, &attr);
	BYTES_EQUAL(expected_result, result);
}

TEST(sp_memory_management_internals, build_mem_region_attr_normal_writeback_non)
{
	uint8_t result = 0;
	uint8_t expected_result = 0;

	attr.normal_memory.cacheability = sp_cacheability_write_back;
	attr.normal_memory.shareability = sp_shareability_non_shareable;

	expected_result = FFA_MEM_REGION_ATTR_MEMORY_TYPE_NORMAL
			  << FFA_MEM_REGION_ATTR_MEMORY_TYPE_SHIFT;
	expected_result |= FFA_MEM_REGION_ATTR_CACHEABILITY_WRITE_BACK
			   << FFA_MEM_REGION_ATTR_CACHEABILITY_SHIFT;
	expected_result |= FFA_MEM_REGION_ATTR_SHAREABILITY_NON_SHAREABLE
			   << FFA_MEM_REGION_ATTR_SHAREABILITY_SHIFT;

	result = build_mem_region_attr(sp_memory_type_normal_memory, &attr);
	BYTES_EQUAL(expected_result, result);
}

TEST(sp_memory_management_internals, build_mem_region_attr_normal_non_inner)
{
	uint8_t result = 0;
	uint8_t expected_result = 0;

	attr.normal_memory.cacheability = sp_cacheability_non_cacheable;
	attr.normal_memory.shareability = sp_shareability_inner_shareable;

	expected_result = FFA_MEM_REGION_ATTR_MEMORY_TYPE_NORMAL
			  << FFA_MEM_REGION_ATTR_MEMORY_TYPE_SHIFT;
	expected_result |= FFA_MEM_REGION_ATTR_CACHEABILITY_NON_CACHEABLE
			   << FFA_MEM_REGION_ATTR_CACHEABILITY_SHIFT;
	expected_result |= FFA_MEM_REGION_ATTR_SHAREABILITY_INNER_SHAREABLE
			   << FFA_MEM_REGION_ATTR_SHAREABILITY_SHIFT;

	result = build_mem_region_attr(sp_memory_type_normal_memory, &attr);
	BYTES_EQUAL(expected_result, result);
}

TEST(sp_memory_management_internals, build_mem_region_attr_normal_non_outer)
{
	uint8_t result = 0;
	uint8_t expected_result = 0;

	attr.normal_memory.cacheability = sp_cacheability_non_cacheable;
	attr.normal_memory.shareability = sp_shareability_outer_shareable;

	expected_result = FFA_MEM_REGION_ATTR_MEMORY_TYPE_NORMAL
			  << FFA_MEM_REGION_ATTR_MEMORY_TYPE_SHIFT;
	expected_result |= FFA_MEM_REGION_ATTR_CACHEABILITY_NON_CACHEABLE
			   << FFA_MEM_REGION_ATTR_CACHEABILITY_SHIFT;
	expected_result |= FFA_MEM_REGION_ATTR_SHAREABILITY_OUTER_SHAREABLE
			   << FFA_MEM_REGION_ATTR_SHAREABILITY_SHIFT;

	result = build_mem_region_attr(sp_memory_type_normal_memory, &attr);
	BYTES_EQUAL(expected_result, result);
}

TEST(sp_memory_management_internals, parse_mem_region_attr_not_specified)
{
	enum sp_memory_type type = sp_memory_type_reserved;
	uint8_t raw = 0;

	parse_mem_region_attr(raw, &type, &attr);
	CHECK_EQUAL(sp_memory_type_not_specified, type);
}

TEST(sp_memory_management_internals, parse_mem_region_attr_device)
{
	enum sp_memory_type type = sp_memory_type_reserved;
	uint8_t raw = 0;

	raw = FFA_MEM_REGION_ATTR_MEMORY_TYPE_DEVICE
	      << FFA_MEM_REGION_ATTR_MEMORY_TYPE_SHIFT;
	raw |= FFA_MEM_REGION_ATTR_DEVICE_MEM_ATTR_GRE
	       << FFA_MEM_REGION_ATTR_DEVICE_MEM_ATTR_SHIFT;

	parse_mem_region_attr(raw, &type, &attr);
	CHECK_EQUAL(sp_memory_type_device_memory, type);
	CHECK_EQUAL(sp_device_memory_GRE, attr.device_memory);
}

TEST(sp_memory_management_internals, parse_mem_region_attr_normal)
{
	enum sp_memory_type type = sp_memory_type_reserved;
	uint8_t raw = 0;

	raw = FFA_MEM_REGION_ATTR_MEMORY_TYPE_NORMAL
	      << FFA_MEM_REGION_ATTR_MEMORY_TYPE_SHIFT;
	raw |= FFA_MEM_REGION_ATTR_CACHEABILITY_WRITE_BACK
	       << FFA_MEM_REGION_ATTR_CACHEABILITY_SHIFT;
	raw |= FFA_MEM_REGION_ATTR_SHAREABILITY_INNER_SHAREABLE
	       << FFA_MEM_REGION_ATTR_SHAREABILITY_SHIFT;

	parse_mem_region_attr(raw, &type, &attr);
	CHECK_EQUAL(sp_memory_type_normal_memory, type);
	CHECK_EQUAL(sp_cacheability_write_back,
		    attr.normal_memory.cacheability);
	CHECK_EQUAL(sp_shareability_inner_shareable,
		    attr.normal_memory.shareability);
}

TEST(sp_memory_management_internals, build_mem_flags_none)
{
	uint32_t result = 0;

	result = build_mem_flags(&flags);
	UNSIGNED_LONGS_EQUAL(0, result);
}

TEST(sp_memory_management_internals, build_mem_flags_zero_memory)
{
	uint32_t result = 0;

	flags.zero_memory = true;
	result = build_mem_flags(&flags);
	UNSIGNED_LONGS_EQUAL(FFA_MEM_TRANSACTION_FLAGS_ZERO_MEMORY, result);
}

TEST(sp_memory_management_internals, build_mem_flags_time_slicing)
{
	uint32_t result = 0;

	flags.operation_time_slicing = true;
	result = build_mem_flags(&flags);
	UNSIGNED_LONGS_EQUAL(FFA_MEM_TRANSACTION_FLAGS_OPERATION_TIME_SLICING,
			     result);
}

TEST(sp_memory_management_internals, build_mem_flags_zero_mem_after_relinquish)
{
	uint32_t result = 0;

	flags.zero_memory_after_relinquish = true;
	result = build_mem_flags(&flags);
	UNSIGNED_LONGS_EQUAL(
		FFA_MEM_TRANSACTION_FLAGS_ZERO_MEMORY_AFTER_RELINQIUSH, result);
}

TEST(sp_memory_management_internals, build_mem_flags_type)
{
	uint32_t result = 0;
	uint32_t expected_result = 0;

	expected_result = FFA_MEM_TRANSACTION_FLAGS_TYPE_DONATE
			  << FFA_MEM_TRANSACTION_FLAGS_TYPE_SHIFT;

	flags.transaction_type = sp_memory_transaction_type_donate;
	result = build_mem_flags(&flags);
	UNSIGNED_LONGS_EQUAL(expected_result, result);
}

TEST(sp_memory_management_internals, build_mem_flags_alignment_hint_invalid)
{
	assert_environment_t env;

	flags.alignment_hint =
		FFA_MEM_TRANSACTION_FLAGS_ALIGNMENT_HINT_MASK + 1;

	if (SETUP_ASSERT_ENVIRONMENT(env)) {
		build_mem_flags(&flags);
	}
}

TEST(sp_memory_management_internals, build_mem_flags_alignment_hint)
{
	uint32_t result = 0;
	uint32_t expected_result = 0;

	expected_result = FFA_MEM_TRANSACTION_FLAGS_ALIGNMENT_HINT_VALID;
	expected_result |= FFA_MEM_TRANSACTION_FLAGS_ALIGNMENT_HINT_MASK
			   << FFA_MEM_TRANSACTION_FLAGS_ALIGNMENT_HINT_SHIFT;

	flags.alignment_hint = FFA_MEM_TRANSACTION_FLAGS_ALIGNMENT_HINT_MASK;
	result = build_mem_flags(&flags);
	UNSIGNED_LONGS_EQUAL(expected_result, result);
}

TEST(sp_memory_management_internals, parse_mem_flags_none)
{
	uint32_t raw = 0;

	parse_mem_flags(raw, &flags);
	CHECK_FALSE(flags.zero_memory);
	CHECK_FALSE(flags.operation_time_slicing);
	CHECK_FALSE(flags.zero_memory_after_relinquish);
	CHECK_EQUAL(sp_memory_transaction_type_relayer_specified,
		    flags.transaction_type);
	UNSIGNED_LONGS_EQUAL(0, flags.alignment_hint);
}

TEST(sp_memory_management_internals, parse_mem_flags_zero_memory)
{
	uint32_t raw = 0;

	raw |= FFA_MEM_TRANSACTION_FLAGS_ZERO_MEMORY;

	parse_mem_flags(raw, &flags);
	CHECK_TRUE(flags.zero_memory);
	CHECK_FALSE(flags.operation_time_slicing);
	CHECK_FALSE(flags.zero_memory_after_relinquish);
	CHECK_EQUAL(sp_memory_transaction_type_relayer_specified,
		    flags.transaction_type);
	UNSIGNED_LONGS_EQUAL(0, flags.alignment_hint);
}

TEST(sp_memory_management_internals, parse_mem_flags_operation_time_slicing)
{
	uint32_t raw = 0;

	raw |= FFA_MEM_TRANSACTION_FLAGS_OPERATION_TIME_SLICING;

	parse_mem_flags(raw, &flags);
	CHECK_FALSE(flags.zero_memory);
	CHECK_TRUE(flags.operation_time_slicing);
	CHECK_FALSE(flags.zero_memory_after_relinquish);
	CHECK_EQUAL(sp_memory_transaction_type_relayer_specified,
		    flags.transaction_type);
	UNSIGNED_LONGS_EQUAL(0, flags.alignment_hint);
}

TEST(sp_memory_management_internals,
     parse_mem_flags_zero_memory_after_relinquish)
{
	uint32_t raw = 0;

	raw |= FFA_MEM_TRANSACTION_FLAGS_ZERO_MEMORY_AFTER_RELINQIUSH;

	parse_mem_flags(raw, &flags);
	CHECK_FALSE(flags.zero_memory);
	CHECK_FALSE(flags.operation_time_slicing);
	CHECK_TRUE(flags.zero_memory_after_relinquish);
	CHECK_EQUAL(sp_memory_transaction_type_relayer_specified,
		    flags.transaction_type);
	UNSIGNED_LONGS_EQUAL(0, flags.alignment_hint);
}

TEST(sp_memory_management_internals, parse_mem_flags_type)
{
	uint32_t raw = 0;

	raw |= FFA_MEM_TRANSACTION_FLAGS_TYPE_DONATE
	       << FFA_MEM_TRANSACTION_FLAGS_TYPE_SHIFT;

	parse_mem_flags(raw, &flags);
	CHECK_FALSE(flags.zero_memory);
	CHECK_FALSE(flags.operation_time_slicing);
	CHECK_FALSE(flags.zero_memory_after_relinquish);
	CHECK_EQUAL(sp_memory_transaction_type_donate, flags.transaction_type);
	UNSIGNED_LONGS_EQUAL(0, flags.alignment_hint);
}

TEST(sp_memory_management_internals, parse_mem_flags_alignment_hint)
{
	uint32_t raw = 0;

	raw |= FFA_MEM_TRANSACTION_FLAGS_ALIGNMENT_HINT_VALID;
	raw |= FFA_MEM_TRANSACTION_FLAGS_ALIGNMENT_HINT_MASK
	       << FFA_MEM_TRANSACTION_FLAGS_ALIGNMENT_HINT_SHIFT;

	parse_mem_flags(raw, &flags);
	CHECK_FALSE(flags.zero_memory);
	CHECK_FALSE(flags.operation_time_slicing);
	CHECK_FALSE(flags.zero_memory_after_relinquish);
	CHECK_EQUAL(sp_memory_transaction_type_relayer_specified,
		    flags.transaction_type);
	UNSIGNED_LONGS_EQUAL(FFA_MEM_TRANSACTION_FLAGS_ALIGNMENT_HINT_MASK,
			     flags.alignment_hint);
}

TEST(sp_memory_management_internals, parse_descriptors_different_tag)
{
	assert_environment_t env;
	uint8_t buffer_area[FFA_MEM_TRANSACTION_PAGE_SIZE] = { 0 };
	struct ffa_mem_transaction_buffer buffer = { 0 };
	struct sp_memory_descriptor desc = { 0 };

	desc.tag = 1;

	ffa_init_mem_transaction_buffer(buffer_area, sizeof(buffer_area),
					&buffer);
	ffa_init_mem_transaction_desc(&buffer, 0, 0, 0, 0, 2);

	if (SETUP_ASSERT_ENVIRONMENT(env)) {
		parse_descriptors(&buffer, &desc, NULL, 0, NULL, 0);
	}
}

TEST(sp_memory_management_internals, parse_descriptors_zero_descriptors)
{
	assert_environment_t env;
	uint8_t buffer_area[FFA_MEM_TRANSACTION_PAGE_SIZE] = { 0 };
	struct ffa_mem_transaction_buffer buffer = { 0 };
	struct sp_memory_descriptor desc = { 0 };

	ffa_init_mem_transaction_buffer(buffer_area, sizeof(buffer_area),
					&buffer);
	ffa_init_mem_transaction_desc(&buffer, 0, 0, 0, 0, 0);

	if (SETUP_ASSERT_ENVIRONMENT(env)) {
		parse_descriptors(&buffer, &desc, NULL, 0, NULL, 0);
	}
}

TEST(sp_memory_management_internals, parse_descriptors_invalid_desc_count)
{
	assert_environment_t env;
	uint8_t buffer_area[FFA_MEM_TRANSACTION_PAGE_SIZE] = { 0 };
	struct ffa_mem_transaction_buffer buffer = { 0 };
	struct sp_memory_descriptor desc = { 0 };

	ffa_init_mem_transaction_buffer(buffer_area, sizeof(buffer_area),
					&buffer);
	ffa_init_mem_transaction_desc(&buffer, 0, 0, 0, 0, 0);
	ffa_add_mem_access_desc(&buffer, 1, 0, 0);

	if (SETUP_ASSERT_ENVIRONMENT(env)) {
		parse_descriptors(&buffer, &desc, NULL, 0, NULL, 0);
	}
}

TEST(sp_memory_management_internals, parse_descriptors_invalid_region_count)
{
	assert_environment_t env;
	uint8_t buffer_area[FFA_MEM_TRANSACTION_PAGE_SIZE] = { 0 };
	struct ffa_mem_transaction_buffer buffer = { 0 };
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	uint32_t region_count = 0;

	ffa_init_mem_transaction_buffer(buffer_area, sizeof(buffer_area),
					&buffer);
	ffa_init_mem_transaction_desc(&buffer, 0, 0, 0, 0, 0);
	ffa_add_mem_access_desc(&buffer, 1, 0, 0);
	ffa_add_memory_region(&buffer, 0, 0);

	if (SETUP_ASSERT_ENVIRONMENT(env)) {
		parse_descriptors(&buffer, &desc, &acc_desc, 1, NULL,
				  &region_count);
	}
}
