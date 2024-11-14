// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2020-2021, Arm Limited. All rights reserved.
 */

#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <stdint.h>
#include <string.h>
#include "mock_assert.h"
#include "../include/ffa_memory_descriptors.h"

#define BUFFER_SIZE (4096U)

static uint8_t tx_buffer_area[BUFFER_SIZE];
static uint8_t rx_buffer_area[BUFFER_SIZE];

TEST_GROUP(ffa_memory_descriptors)
{
	TEST_SETUP()
	{
		memset(tx_buffer_area, 0xff, sizeof(tx_buffer_area));
		memset(rx_buffer_area, 0xff, sizeof(rx_buffer_area));

		ffa_init_mem_transaction_buffer(
			tx_buffer_area, sizeof(tx_buffer_area), &tx_buffer);
		ffa_init_mem_transaction_buffer(
			rx_buffer_area, sizeof(rx_buffer_area), &rx_buffer);
	}

	TEST_TEARDOWN()
	{
		mock().checkExpectations();
		mock().clear();
	}

	template <typename Type> Type get_tx_value(uintptr_t offset)
	{
		return *(Type *)&tx_buffer_area[offset];
	}

	void validate_mem_transaction_descriptor(uint16_t sender_id, uint16_t mem_region_attr, uint32_t flags, uint64_t handle, uint64_t tag, uint32_t mem_desc_count)
	{
#if CFG_FFA_VERSION == FFA_VERSION_1_0
		UNSIGNED_LONGS_EQUAL(sender_id, get_tx_value<uint16_t>(0));
		BYTES_EQUAL(mem_region_attr, get_tx_value<uint8_t>(2));
		BYTES_EQUAL(0, get_tx_value<uint8_t>(3));
		UNSIGNED_LONGS_EQUAL(flags, get_tx_value<uint32_t>(4));
		UNSIGNED_LONGLONGS_EQUAL(handle, get_tx_value<uint64_t>(8));
		UNSIGNED_LONGLONGS_EQUAL(tag, get_tx_value<uint64_t>(16));
		UNSIGNED_LONGS_EQUAL(0, get_tx_value<uint32_t>(24));
		UNSIGNED_LONGS_EQUAL(mem_desc_count, get_tx_value<uint32_t>(28));
#elif CFG_FFA_VERSION >= FFA_VERSION_1_1
		uint8_t reserved[12] = { 0 };

		UNSIGNED_LONGS_EQUAL(sender_id, get_tx_value<uint16_t>(0));
		UNSIGNED_LONGS_EQUAL(mem_region_attr, get_tx_value<uint16_t>(2));
		UNSIGNED_LONGS_EQUAL(flags, get_tx_value<uint32_t>(4));
		UNSIGNED_LONGLONGS_EQUAL(handle, get_tx_value<uint64_t>(8));
		UNSIGNED_LONGLONGS_EQUAL(tag, get_tx_value<uint64_t>(16));
		UNSIGNED_LONGS_EQUAL(sizeof(struct ffa_mem_access_desc), get_tx_value<uint32_t>(24));
		UNSIGNED_LONGS_EQUAL(mem_desc_count, get_tx_value<uint32_t>(28));
		UNSIGNED_LONGS_EQUAL(sizeof(struct ffa_mem_transaction_desc),
				     get_tx_value<uint32_t>(32));
		MEMCMP_EQUAL(reserved, tx_buffer_area + 36, sizeof(reserved));
#endif /* CFG_FFA_VERSION */
	}

	struct ffa_mem_transaction_buffer tx_buffer;
	struct ffa_mem_transaction_buffer rx_buffer;

	const uint16_t sender_id = 0x0123;
	const uint16_t receiver_id = 0x4567;
	const uint8_t mem_region_attr = 0x89;
	const uint8_t mem_access_perm = 0xab;
	const uint32_t flags = 0xcdeffedc;
	const uint8_t flags2 = 0xc5;
	const uint64_t handle = 0xefcdab8967452301ULL;
	const uint64_t tag = 0x11223344556677ULL;
	const uint8_t *ptr = (uint8_t *)(0x0112233445566778ULL);
};

TEST(ffa_memory_descriptors, ffa_init_mem_transaction_buffer)
{
	POINTERS_EQUAL(tx_buffer_area, tx_buffer.buffer);
	UNSIGNED_LONGS_EQUAL(sizeof(tx_buffer_area), tx_buffer.length);
	UNSIGNED_LONGS_EQUAL(0, tx_buffer.used);

	POINTERS_EQUAL(rx_buffer_area, rx_buffer.buffer);
	UNSIGNED_LONGS_EQUAL(sizeof(rx_buffer_area), rx_buffer.length);
	UNSIGNED_LONGS_EQUAL(0, rx_buffer.used);
}

TEST(ffa_memory_descriptors, ffa_init_mem_transaction_desc)
{
	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);

	validate_mem_transaction_descriptor(sender_id, mem_region_attr, flags, handle, tag, 0);
	UNSIGNED_LONGS_EQUAL(sizeof(struct ffa_mem_transaction_desc), tx_buffer.used);
}

TEST(ffa_memory_descriptors, ffa_init_mem_transaction_desc_buffer_overflow)
{
	assert_environment_t assert_env;

	tx_buffer.length = sizeof(struct ffa_mem_transaction_desc) - 1;

	if (SETUP_ASSERT_ENVIRONMENT(assert_env)) {
		ffa_init_mem_transaction_desc(&tx_buffer, sender_id,
					      mem_region_attr, flags, handle,
					      tag);
	}

	UNSIGNED_LONGS_EQUAL(0, tx_buffer.used);
}

TEST(ffa_memory_descriptors, ffa_get_mem_transaction_desc)
{
	const struct ffa_mem_transaction_desc *transaction = NULL;

	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);

	transaction = ffa_get_mem_transaction_desc(&tx_buffer);

#if CFG_FFA_VERSION == FFA_VERSION_1_0
	UNSIGNED_LONGS_EQUAL(sender_id, transaction->sender_id);
	BYTES_EQUAL(mem_region_attr, transaction->mem_region_attr);
	BYTES_EQUAL(0, transaction->reserved_mbz0);
	UNSIGNED_LONGS_EQUAL(flags, transaction->flags);
	UNSIGNED_LONGLONGS_EQUAL(handle, transaction->handle);
	UNSIGNED_LONGLONGS_EQUAL(tag, transaction->tag);
	UNSIGNED_LONGS_EQUAL(0, transaction->reserved_mbz1);
	UNSIGNED_LONGS_EQUAL(0, transaction->mem_access_desc_count);
#elif CFG_FFA_VERSION >= FFA_VERSION_1_1
	uint8_t reserved[12] = { 0 };

	UNSIGNED_LONGS_EQUAL(sender_id, transaction->sender_id);
	UNSIGNED_LONGS_EQUAL(mem_region_attr, transaction->mem_region_attr);
	UNSIGNED_LONGS_EQUAL(flags, transaction->flags);
	UNSIGNED_LONGLONGS_EQUAL(handle, transaction->handle);
	UNSIGNED_LONGLONGS_EQUAL(tag, transaction->tag);
	UNSIGNED_LONGS_EQUAL(sizeof(ffa_mem_access_desc), transaction->mem_access_desc_size);
	UNSIGNED_LONGS_EQUAL(0, transaction->mem_access_desc_count);
	MEMCMP_EQUAL(reserved, transaction->reserved_mbz0, sizeof(reserved));
#endif /* CFG_FFA_VERSION */
}

TEST(ffa_memory_descriptors, ffa_reserve_mem_access_desc)
{
	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);

	ffa_reserve_mem_access_desc(&tx_buffer, 3);

	validate_mem_transaction_descriptor(sender_id, mem_region_attr, flags, handle, tag, 0);
	UNSIGNED_LONGS_EQUAL(
		sizeof(struct ffa_mem_transaction_desc) + sizeof(struct ffa_mem_access_desc) * 3,
		tx_buffer.used);
}

TEST(ffa_memory_descriptors, ffa_reserve_mem_access_desc_shrink)
{
	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);

	ffa_reserve_mem_access_desc(&tx_buffer, 3);
	ffa_reserve_mem_access_desc(&tx_buffer, 1);

	validate_mem_transaction_descriptor(sender_id, mem_region_attr, flags, handle, tag, 0);
	/* It should not shrink the size */
	UNSIGNED_LONGS_EQUAL(
		sizeof(struct ffa_mem_transaction_desc) + sizeof(struct ffa_mem_access_desc) * 3,
		tx_buffer.used);
}

TEST(ffa_memory_descriptors, ffa_reserve_mem_access_desc_buffer_overflow)
{
	assert_environment_t assert_env;

	tx_buffer.length = sizeof(struct ffa_mem_transaction_desc) +
			   sizeof(struct ffa_mem_access_desc);

	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);

	if (SETUP_ASSERT_ENVIRONMENT(assert_env)) {
		ffa_reserve_mem_access_desc(&tx_buffer, 2);
	}
}

TEST(ffa_memory_descriptors, ffa_add_mem_access_desc)
{
	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);

	uint32_t desc_id = ffa_add_mem_access_desc(&tx_buffer, receiver_id,
						   mem_access_perm, flags2);

	UNSIGNED_LONGS_EQUAL(0, desc_id);

	validate_mem_transaction_descriptor(sender_id, mem_region_attr, flags, handle, tag, 1);

	uint32_t offset = sizeof(struct ffa_mem_transaction_desc);
	UNSIGNED_LONGS_EQUAL(receiver_id, get_tx_value<uint16_t>(offset));
	BYTES_EQUAL(mem_access_perm, get_tx_value<uint8_t>(offset + 2));
	BYTES_EQUAL(flags2, get_tx_value<uint8_t>(offset + 3));
	UNSIGNED_LONGS_EQUAL(0, get_tx_value<uint16_t>(offset + 4)); // Composite offset
	UNSIGNED_LONGLONGS_EQUAL(0, get_tx_value<uint64_t>(offset + 8)); // Reserved

	UNSIGNED_LONGS_EQUAL(
		sizeof(struct ffa_mem_transaction_desc) + sizeof(struct ffa_mem_access_desc),
		tx_buffer.used);
}

TEST(ffa_memory_descriptors, ffa_add_mem_access_desc_two)
{
	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);

	uint32_t desc_id = ffa_add_mem_access_desc(&tx_buffer, receiver_id,
						   mem_access_perm, flags2);
	uint32_t desc_id2 = ffa_add_mem_access_desc(&tx_buffer, ~receiver_id,
						    ~mem_access_perm, ~flags2);

	UNSIGNED_LONGS_EQUAL(0, desc_id);
	UNSIGNED_LONGS_EQUAL(1, desc_id2);

	validate_mem_transaction_descriptor(sender_id, mem_region_attr, flags, handle, tag, 2);

	uint32_t offset = sizeof(struct ffa_mem_transaction_desc);
	UNSIGNED_LONGS_EQUAL(receiver_id, get_tx_value<uint16_t>(offset));
	BYTES_EQUAL(mem_access_perm, get_tx_value<uint8_t>(offset + 2));
	BYTES_EQUAL(flags2, get_tx_value<uint8_t>(offset + 3));
	UNSIGNED_LONGS_EQUAL(
		0, get_tx_value<uint16_t>(offset + 4)); // Composite offset
	UNSIGNED_LONGLONGS_EQUAL(
		0, get_tx_value<uint64_t>(offset + 8)); // Reserved

	offset = sizeof(struct ffa_mem_transaction_desc) +
		 sizeof(struct ffa_mem_access_desc);

	UNSIGNED_LONGS_EQUAL((uint16_t)~receiver_id,
			     get_tx_value<uint16_t>(offset));
	BYTES_EQUAL((uint8_t)~mem_access_perm,
		    get_tx_value<uint8_t>(offset + 2));
	BYTES_EQUAL((uint8_t)~flags2, get_tx_value<uint8_t>(offset + 3));
	UNSIGNED_LONGS_EQUAL(
		0, get_tx_value<uint16_t>(offset + 4)); // Composite offset
	UNSIGNED_LONGLONGS_EQUAL(
		0, get_tx_value<uint64_t>(offset + 8)); // Reserved

	UNSIGNED_LONGS_EQUAL(
		sizeof(struct ffa_mem_transaction_desc) + sizeof(struct ffa_mem_access_desc) * 2,
		tx_buffer.used);
}

TEST(ffa_memory_descriptors, ffa_add_mem_access_desc_buffer_overflow)
{
	assert_environment_t assert_env;

	tx_buffer.length = sizeof(struct ffa_mem_transaction_desc);

	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);

	if (SETUP_ASSERT_ENVIRONMENT(assert_env)) {
		ffa_add_mem_access_desc(&tx_buffer, receiver_id,
					mem_access_perm, flags2);
	}

	UNSIGNED_LONGS_EQUAL(sizeof(struct ffa_mem_transaction_desc),
			     tx_buffer.used);
}

TEST(ffa_memory_descriptors, ffa_add_mem_access_desc_count_overflow)
{
	assert_environment_t assert_env;

	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);

	((struct ffa_mem_transaction_desc *)tx_buffer_area)->mem_access_desc_count = UINT32_MAX;

	if (SETUP_ASSERT_ENVIRONMENT(assert_env)) {
		ffa_add_mem_access_desc(&tx_buffer, receiver_id,
					mem_access_perm, flags2);
	}
}

TEST(ffa_memory_descriptors, ffa_add_mem_access_desc_two_then_shrink)
{
	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);

	uint32_t desc_id = ffa_add_mem_access_desc(&tx_buffer, receiver_id,
						   mem_access_perm, flags2);
	uint32_t desc_id2 = ffa_add_mem_access_desc(&tx_buffer, ~receiver_id,
						    ~mem_access_perm, ~flags2);

	ffa_reserve_mem_access_desc(&tx_buffer, 1);

	UNSIGNED_LONGS_EQUAL(0, desc_id);
	UNSIGNED_LONGS_EQUAL(1, desc_id2);

	validate_mem_transaction_descriptor(sender_id, mem_region_attr, flags, handle, tag, 2);

	uint32_t offset = sizeof(struct ffa_mem_transaction_desc);
	UNSIGNED_LONGS_EQUAL(receiver_id, get_tx_value<uint16_t>(offset));
	BYTES_EQUAL(mem_access_perm, get_tx_value<uint8_t>(offset + 2));
	BYTES_EQUAL(flags2, get_tx_value<uint8_t>(offset + 3));
	UNSIGNED_LONGS_EQUAL(
		0, get_tx_value<uint16_t>(offset + 4)); // Composite offset
	UNSIGNED_LONGLONGS_EQUAL(
		0, get_tx_value<uint64_t>(offset + 8)); // Reserved

	offset = sizeof(struct ffa_mem_transaction_desc) +
		 sizeof(struct ffa_mem_access_desc);
	UNSIGNED_LONGS_EQUAL((uint16_t)~receiver_id,
			     get_tx_value<uint16_t>(offset));
	BYTES_EQUAL((uint8_t)~mem_access_perm,
		    get_tx_value<uint8_t>(offset + 2));
	BYTES_EQUAL((uint8_t)~flags2, get_tx_value<uint8_t>(offset + 3));
	UNSIGNED_LONGS_EQUAL(
		0, get_tx_value<uint16_t>(offset + 4)); // Composite offset
	UNSIGNED_LONGLONGS_EQUAL(
		0, get_tx_value<uint64_t>(offset + 8)); // Reserved

	UNSIGNED_LONGS_EQUAL(sizeof(struct ffa_mem_transaction_desc) +
				     sizeof(struct ffa_mem_access_desc) * 2,
			     tx_buffer.used);
}

TEST(ffa_memory_descriptors, ffa_get_mem_access_desc)
{
	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);
	uint32_t desc_id = ffa_add_mem_access_desc(&tx_buffer, receiver_id,
						   mem_access_perm, flags2);
	const struct ffa_mem_access_desc *access_desc =
		ffa_get_mem_access_desc(&tx_buffer, desc_id);

	UNSIGNED_LONGS_EQUAL(receiver_id,
			     access_desc->mem_access_perm_desc.endpoint_id);
	BYTES_EQUAL(mem_access_perm,
		    access_desc->mem_access_perm_desc.mem_access_permissions);
	BYTES_EQUAL(flags2, access_desc->mem_access_perm_desc.flags);
	UNSIGNED_LONGS_EQUAL(0, access_desc->composite_mem_region_desc_offset);
	UNSIGNED_LONGLONGS_EQUAL(0, access_desc->reserved_mbz);
}

TEST(ffa_memory_descriptors, ffa_get_mem_access_desc_overflow)
{
	assert_environment_t assert_env;

	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);
	uint32_t desc_id = ffa_add_mem_access_desc(&tx_buffer, receiver_id,
						   mem_access_perm, flags2);

	tx_buffer.length = sizeof(struct ffa_mem_transaction_desc) +
			   sizeof(struct ffa_mem_access_desc) - 1;

	if (SETUP_ASSERT_ENVIRONMENT(assert_env)) {
		ffa_get_mem_access_desc(&tx_buffer, desc_id);
	}
}

TEST(ffa_memory_descriptors, ffa_get_mem_access_desc_empty)
{
	assert_environment_t assert_env;

	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);

	if (SETUP_ASSERT_ENVIRONMENT(assert_env)) {
		ffa_get_mem_access_desc(&tx_buffer, 0);
	}
}

TEST(ffa_memory_descriptors, ffa_add_memory_region)
{
	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);
	uint32_t desc_id = ffa_add_mem_access_desc(&tx_buffer, receiver_id,
						   mem_access_perm, flags2);
	UNSIGNED_LONGS_EQUAL(0, desc_id);

	ffa_add_memory_region(&tx_buffer, ptr, 3);

	validate_mem_transaction_descriptor(sender_id, mem_region_attr, flags, handle, tag, 1);

	uint32_t offset = sizeof(struct ffa_mem_transaction_desc);
	uint32_t composite_offset = offset + sizeof(struct ffa_mem_access_desc);

	UNSIGNED_LONGS_EQUAL(receiver_id, get_tx_value<uint16_t>(offset));
	BYTES_EQUAL(mem_access_perm, get_tx_value<uint8_t>(offset + 2));
	BYTES_EQUAL(flags2, get_tx_value<uint8_t>(offset + 3));
	UNSIGNED_LONGS_EQUAL(
		composite_offset,
		get_tx_value<uint16_t>(offset + 4)); // Composite offset
	UNSIGNED_LONGLONGS_EQUAL(
		0, get_tx_value<uint64_t>(offset + 8)); // Reserved

	UNSIGNED_LONGS_EQUAL(3, get_tx_value<uint32_t>(composite_offset));
	UNSIGNED_LONGS_EQUAL(1, get_tx_value<uint32_t>(composite_offset + 4));
	UNSIGNED_LONGLONGS_EQUAL(
		0, get_tx_value<uint64_t>(composite_offset + 8)); // Reserved

	uint32_t constituent_offset =
		composite_offset + sizeof(struct ffa_composite_mem_region_desc);

	UNSIGNED_LONGLONGS_EQUAL((uint64_t)ptr,
				 get_tx_value<uint64_t>(constituent_offset));
	UNSIGNED_LONGS_EQUAL(3, get_tx_value<uint32_t>(constituent_offset + 8));
	UNSIGNED_LONGS_EQUAL(0,
			     get_tx_value<uint32_t>(constituent_offset + 12));
}

TEST(ffa_memory_descriptors, ffa_add_memory_region_two)
{
	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);
	uint32_t desc_id = ffa_add_mem_access_desc(&tx_buffer, receiver_id,
						   mem_access_perm, flags2);
	UNSIGNED_LONGS_EQUAL(0, desc_id);

	ffa_add_memory_region(&tx_buffer, ptr, 3);
	ffa_add_memory_region(&tx_buffer, ptr + 1, 2);

	validate_mem_transaction_descriptor(sender_id, mem_region_attr, flags, handle, tag, 1);

	uint32_t offset = sizeof(struct ffa_mem_transaction_desc);
	uint32_t composite_offset = offset + sizeof(struct ffa_mem_access_desc);

	UNSIGNED_LONGS_EQUAL(receiver_id, get_tx_value<uint16_t>(offset));
	BYTES_EQUAL(mem_access_perm, get_tx_value<uint8_t>(offset + 2));
	BYTES_EQUAL(flags2, get_tx_value<uint8_t>(offset + 3));
	UNSIGNED_LONGS_EQUAL(composite_offset,
			     get_tx_value<uint16_t>(offset + 4));
	UNSIGNED_LONGLONGS_EQUAL(
		0, get_tx_value<uint64_t>(offset + 8)); // Reserved

	UNSIGNED_LONGS_EQUAL(5, get_tx_value<uint32_t>(composite_offset));
	UNSIGNED_LONGS_EQUAL(2, get_tx_value<uint32_t>(composite_offset + 4));
	UNSIGNED_LONGLONGS_EQUAL(
		0, get_tx_value<uint64_t>(composite_offset + 8)); // Reserved

	uint32_t constituent_offset =
		composite_offset + sizeof(struct ffa_composite_mem_region_desc);

	UNSIGNED_LONGLONGS_EQUAL((uint64_t)ptr,
				 get_tx_value<uint64_t>(constituent_offset));
	UNSIGNED_LONGS_EQUAL(3, get_tx_value<uint32_t>(constituent_offset + 8));
	UNSIGNED_LONGS_EQUAL(0,
			     get_tx_value<uint32_t>(constituent_offset + 12));

	constituent_offset += sizeof(struct ffa_composite_mem_region_desc);

	UNSIGNED_LONGLONGS_EQUAL((uint64_t)ptr + 1,
				 get_tx_value<uint64_t>(constituent_offset));
	UNSIGNED_LONGS_EQUAL(2, get_tx_value<uint32_t>(constituent_offset + 8));
	UNSIGNED_LONGS_EQUAL(0,
			     get_tx_value<uint32_t>(constituent_offset + 12));
}

TEST(ffa_memory_descriptors, ffa_add_memory_region_no_access_desc)
{
	assert_environment_t assert_env;

	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);

	if (SETUP_ASSERT_ENVIRONMENT(assert_env)) {
		ffa_add_memory_region(&tx_buffer, ptr, 3);
	}
}

TEST(ffa_memory_descriptors, ffa_add_memory_region_composite_overflow)
{
	assert_environment_t assert_env;

	tx_buffer.length = sizeof(struct ffa_mem_transaction_desc) +
			   sizeof(struct ffa_mem_access_desc);

	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);
	ffa_add_mem_access_desc(&tx_buffer, receiver_id, mem_access_perm,
				flags2);

	if (SETUP_ASSERT_ENVIRONMENT(assert_env)) {
		ffa_add_memory_region(&tx_buffer, ptr, 3);
	}
}

TEST(ffa_memory_descriptors, ffa_add_memory_region_constituent_overflow)
{
	assert_environment_t assert_env;

	tx_buffer.length = sizeof(struct ffa_mem_transaction_desc) +
			   sizeof(struct ffa_mem_access_desc) +
			   sizeof(struct ffa_composite_mem_region_desc);

	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);
	ffa_add_mem_access_desc(&tx_buffer, receiver_id, mem_access_perm,
				flags2);

	if (SETUP_ASSERT_ENVIRONMENT(assert_env)) {
		ffa_add_memory_region(&tx_buffer, ptr, 3);
	}
}

TEST(ffa_memory_descriptors, ffa_add_memory_region_max_offset)
{
	assert_environment_t assert_env;

	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);
	ffa_add_mem_access_desc(&tx_buffer, receiver_id, mem_access_perm,
				flags2);

	tx_buffer.used = 0x100000000ULL;
	tx_buffer.length = 0x200000000ULL;

	if (SETUP_ASSERT_ENVIRONMENT(assert_env)) {
		ffa_add_memory_region(&tx_buffer, ptr, 3);
	}
}

TEST(ffa_memory_descriptors, ffa_add_memory_region_invalid_offset)
{
	assert_environment_t assert_env;

	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);
	ffa_add_mem_access_desc(&tx_buffer, receiver_id, mem_access_perm,
				flags2);
	ffa_add_memory_region(&tx_buffer, ptr, 3);

	uint32_t offset = sizeof(struct ffa_mem_transaction_desc) + 4;
	*(uint32_t *)&tx_buffer_area[offset] = tx_buffer.length;

	if (SETUP_ASSERT_ENVIRONMENT(assert_env)) {
		ffa_add_memory_region(&tx_buffer, ptr, 3);
	}
}

TEST(ffa_memory_descriptors, ffa_add_memory_region_and_access)
{
	assert_environment_t assert_env;

	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);
	ffa_add_mem_access_desc(&tx_buffer, receiver_id, mem_access_perm,
				flags2);
	ffa_add_memory_region(&tx_buffer, ptr, 3);

	if (SETUP_ASSERT_ENVIRONMENT(assert_env)) {
		ffa_add_mem_access_desc(&tx_buffer, receiver_id,
					mem_access_perm, flags2);
	}
}

TEST(ffa_memory_descriptors, ffa_add_memory_region_two_and_access)
{
	assert_environment_t assert_env;

	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);
	ffa_add_mem_access_desc(&tx_buffer, receiver_id, mem_access_perm,
				flags2);
	ffa_add_mem_access_desc(&tx_buffer, receiver_id, mem_access_perm,
				flags2);
	ffa_add_memory_region(&tx_buffer, ptr, 3);
	ffa_add_memory_region(&tx_buffer, ptr, 3);

	if (SETUP_ASSERT_ENVIRONMENT(assert_env)) {
		ffa_add_mem_access_desc(&tx_buffer, receiver_id,
					mem_access_perm, flags2);
	}
}

TEST(ffa_memory_descriptors, ffa_add_memory_region_and_reserve)
{
	assert_environment_t assert_env;

	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);
	ffa_add_mem_access_desc(&tx_buffer, receiver_id, mem_access_perm,
				flags2);
	ffa_add_memory_region(&tx_buffer, ptr, 3);

	if (SETUP_ASSERT_ENVIRONMENT(assert_env)) {
		ffa_reserve_mem_access_desc(&tx_buffer, 2);
	}
}

TEST(ffa_memory_descriptors, ffa_add_memory_region_reserve_before_access)
{
	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);
	ffa_add_mem_access_desc(&tx_buffer, receiver_id, mem_access_perm,
				flags2);

	ffa_reserve_mem_access_desc(&tx_buffer, 2);
	ffa_add_memory_region(&tx_buffer, ptr, 3);

	ffa_add_mem_access_desc(&tx_buffer, receiver_id, mem_access_perm,
				flags2);
}

TEST(ffa_memory_descriptors, ffa_get_memory_region)
{
	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);
	ffa_add_mem_access_desc(&tx_buffer, receiver_id, mem_access_perm,
				flags2);

	ffa_add_memory_region(&tx_buffer, ptr, 3);

	const struct ffa_composite_mem_region_desc *mem_region =
		ffa_get_memory_region(&tx_buffer);

	UNSIGNED_LONGS_EQUAL(3, mem_region->total_page_count);
	UNSIGNED_LONGS_EQUAL(1, mem_region->address_range_count);
	UNSIGNED_LONGLONGS_EQUAL(0, mem_region->reserved_mbz);

	const ffa_constituent_mem_region_desc *constituent =
		&mem_region->constituent_mem_region_desc[0];
	UNSIGNED_LONGLONGS_EQUAL((uint64_t)ptr, constituent->address);
	UNSIGNED_LONGS_EQUAL(3, constituent->page_count);
	UNSIGNED_LONGS_EQUAL(0, constituent->reserved_mbz);
}

TEST(ffa_memory_descriptors, ffa_get_memory_region_overflow)
{
	assert_environment_t assert_env;

	ffa_init_mem_transaction_desc(&tx_buffer, sender_id, mem_region_attr,
				      flags, handle, tag);
	ffa_add_mem_access_desc(&tx_buffer, receiver_id, mem_access_perm,
				flags2);

	ffa_add_memory_region(&tx_buffer, ptr, 3);

	uint32_t offset = sizeof(struct ffa_mem_transaction_desc) +
			  sizeof(ffa_mem_access_desc) + 4;
	*(uint32_t *)&tx_buffer_area[offset] = 1000000;

	if (SETUP_ASSERT_ENVIRONMENT(assert_env)) {
		ffa_get_memory_region(&tx_buffer);
	}
}
