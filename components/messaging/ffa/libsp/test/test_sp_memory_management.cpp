// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2020-2022, Arm Limited. All rights reserved.
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

#define SP_MEM_DESC_C(sender_id_val, tag_val)                                                      \
	((const struct sp_memory_descriptor){                                                      \
		.sender_id = (sender_id_val),                                                      \
		.memory_type = sp_memory_type_not_specified,                                       \
		.mem_region_attr = { .normal_memory = { .cacheability =                            \
								sp_cacheability_reserved0,         \
							.shareability =                            \
								sp_shareability_non_shareable } }, \
		.flags = { 0 },                                                                    \
		.tag = (tag_val) })

#define FFA_MEM_DESC_C(sender_id_val, handle_val, tag_val)                     \
	((const struct ffa_mem_transaction_desc){ .sender_id =                 \
							  (sender_id_val),     \
						  .mem_region_attr = 0,        \
						  .reserved_mbz0 = 0,          \
						  .flags = 0,                  \
						  .handle = (handle_val),      \
						  .tag = (tag_val),            \
						  .reserved_mbz1 = 0 })

static uint8_t tx_buffer_area[FFA_MEM_TRANSACTION_PAGE_SIZE]
	__attribute__((aligned(FFA_MEM_TRANSACTION_PAGE_SIZE)));
static uint8_t rx_buffer_area[FFA_MEM_TRANSACTION_PAGE_SIZE]
	__attribute__((aligned(FFA_MEM_TRANSACTION_PAGE_SIZE)));
static void *tx_buffer = tx_buffer_area;
static const void *rx_buffer = rx_buffer_area;
static size_t buffer_size = sizeof(tx_buffer_area);

TEST_GROUP(sp_memory_management)
{
	TEST_SETUP()
	{
		memset(tx_buffer_area, 0xff, sizeof(tx_buffer_area));
		memset(rx_buffer_area, 0x00, sizeof(rx_buffer_area));

		ffa_init_mem_transaction_buffer(tx_buffer_area,
						sizeof(tx_buffer_area),
						&tx_mem_transaction_buffer);
		ffa_init_mem_transaction_buffer(rx_buffer_area,
						sizeof(rx_buffer_area),
						&rx_mem_transaction_buffer);
	}

	TEST_TEARDOWN()
	{
		mock().checkExpectations();
		mock().clear();
	}

	uint32_t get_expected_size(uint32_t region_count)
	{
		uint32_t expected_size =
			sizeof(struct ffa_mem_transaction_desc) +
			sizeof(struct ffa_mem_access_desc);
		if (region_count > 0) {
			expected_size +=
				sizeof(struct ffa_composite_mem_region_desc);
			expected_size +=
				sizeof(struct ffa_constituent_mem_region_desc) *
				region_count;
		}

		return expected_size;
	}

	void check_descriptors(
		const struct ffa_mem_transaction_desc *expected_transaction,
		const struct ffa_mem_access_perm_desc *expected_access_perm_desc,
		const struct ffa_constituent_mem_region_desc expected_regions[],
		uint32_t region_count)
	{
		const struct ffa_mem_transaction_desc *transaction = NULL;
		const struct ffa_mem_access_desc *mem_access_desc = NULL;
		const struct ffa_mem_access_perm_desc *mem_access_perm_desc =
			NULL;
		const struct ffa_composite_mem_region_desc *composite_desc =
			NULL;

		transaction = ffa_get_mem_transaction_desc(
			&tx_mem_transaction_buffer);
		UNSIGNED_LONGS_EQUAL(expected_transaction->sender_id,
				     transaction->sender_id);
		BYTES_EQUAL(expected_transaction->mem_region_attr,
			    transaction->mem_region_attr);
		UNSIGNED_LONGS_EQUAL(expected_transaction->flags,
				     transaction->flags);
		UNSIGNED_LONGLONGS_EQUAL(expected_transaction->handle,
					 transaction->handle);
		UNSIGNED_LONGLONGS_EQUAL(expected_transaction->tag,
					 transaction->tag);

		mem_access_desc =
			ffa_get_mem_access_desc(&tx_mem_transaction_buffer, 0);
		mem_access_perm_desc = &mem_access_desc->mem_access_perm_desc;
		UNSIGNED_LONGS_EQUAL(expected_access_perm_desc->endpoint_id,
				     mem_access_perm_desc->endpoint_id);
		BYTES_EQUAL(expected_access_perm_desc->mem_access_permissions,
			    mem_access_perm_desc->mem_access_permissions);
		BYTES_EQUAL(expected_access_perm_desc->flags,
			    mem_access_perm_desc->flags);

		composite_desc =
			ffa_get_memory_region(&tx_mem_transaction_buffer);
		UNSIGNED_LONGS_EQUAL(region_count,
				     composite_desc->address_range_count);
		uint32_t total_page_count = 0;
		for (uint32_t i = 0; i < region_count; i++) {
			const struct ffa_constituent_mem_region_desc *region =
				&composite_desc->constituent_mem_region_desc[i];

			UNSIGNED_LONGLONGS_EQUAL(expected_regions[i].address,
						 region->address);
			UNSIGNED_LONGS_EQUAL(expected_regions[i].page_count,
					     region->page_count);
			total_page_count += region->page_count;
		}
		UNSIGNED_LONGS_EQUAL(total_page_count,
				     composite_desc->total_page_count);
	}

	struct ffa_mem_transaction_buffer tx_mem_transaction_buffer;
	struct ffa_mem_transaction_buffer rx_mem_transaction_buffer;

	const uint16_t sender_id = 0x0123;
	const uint16_t receiver_id = 0x4567;
	const uint8_t mem_region_attr = 0x89;
	const uint8_t mem_access_perm = 0xab;
	const uint32_t flags = 0xcdeffedc;
	const uint8_t flags2 = 0xc5;
	const uint64_t handle = 0xefcdab8967452301ULL;
	const uint64_t tag = 0x11223344556677ULL;
	void *ptr = (void *)(0x0112233445566778ULL);
	const uint32_t page_count = 0x12345678;

	const sp_result result = -1;
};

TEST(sp_memory_management, sp_memory_donate_descriptor_null)
{
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_donate(NULL, &acc_desc, &regions, 1, &handle));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_donate_acc_descriptor_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_donate(&desc, NULL, &regions, 1, &handle));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_donate_regions_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_donate(&desc, &acc_desc, NULL, 1, &handle));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_donate_region_count_zero)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_donate(&desc, &acc_desc, &regions, 0, &handle));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_donate_handle_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_donate(&desc, &acc_desc, &regions, 1, NULL));
}

TEST(sp_memory_management, sp_memory_donate_get_tx_error)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;
	const sp_result result = SP_RESULT_INTERNAL_ERROR;

	expect_sp_rxtx_buffer_tx_get(&tx_buffer, &buffer_size, result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_memory_donate(&desc, &acc_desc, &regions, 1, &handle));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_donate_ffa_error)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint32_t region_count = 1;
	uint64_t handle = 1;
	const uint64_t expected_handle = 0;
	const sp_result result = FFA_ABORTED;
	const uint32_t expected_size = get_expected_size(region_count);

	expect_sp_rxtx_buffer_tx_get(&tx_buffer, &buffer_size, SP_RESULT_OK);
	expect_ffa_mem_donate_rxtx(expected_size, expected_size,
				   &expected_handle, result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_memory_donate(&desc, &acc_desc, &regions, region_count,
				     &handle));
	UNSIGNED_LONGLONGS_EQUAL(expected_handle, handle);
}

TEST(sp_memory_management, sp_memory_donate)
{
	struct sp_memory_descriptor desc = SP_MEM_DESC_C(sender_id, tag);
	struct sp_memory_access_descriptor acc_desc = { .receiver_id =
								receiver_id };
	struct sp_memory_region regions = { .address = ptr,
					    .page_count = page_count };
	uint32_t region_count = 1;
	uint64_t handle = 1;
	const uint64_t expected_handle = 0x123456789abcdef0U;
	const uint32_t expected_size = get_expected_size(region_count);

	expect_sp_rxtx_buffer_tx_get(&tx_buffer, &buffer_size, SP_RESULT_OK);
	expect_ffa_mem_donate_rxtx(expected_size, expected_size,
				   &expected_handle, FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK, sp_memory_donate(&desc, &acc_desc, &regions,
						   region_count, &handle));
	UNSIGNED_LONGLONGS_EQUAL(expected_handle, handle);

	const struct ffa_mem_transaction_desc expected_transaction =
		FFA_MEM_DESC_C(sender_id, 0, tag);
	const struct ffa_mem_access_perm_desc expected_access_perm_desc = {
		.endpoint_id = receiver_id
	};
	const struct ffa_constituent_mem_region_desc expected_regions = {
		.address = (uint64_t)ptr, .page_count = page_count
	};
	check_descriptors(&expected_transaction, &expected_access_perm_desc,
			  &expected_regions, 1);
}

TEST(sp_memory_management, sp_memory_donate_dynamic_descriptor_null)
{
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_donate_dynamic(NULL, &acc_desc, &regions, 1,
					     &handle,
					     &tx_mem_transaction_buffer));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_donate_dynamic_acc_descriptor_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_donate_dynamic(&desc, NULL, &regions, 1, &handle,
					     &tx_mem_transaction_buffer));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_donate_dynamic_regions_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_donate_dynamic(&desc, &acc_desc, NULL, 1, &handle,
					     &tx_mem_transaction_buffer));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_donate_dynamic_region_count_zero)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_donate_dynamic(&desc, &acc_desc, &regions, 0,
					     &handle,
					     &tx_mem_transaction_buffer));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_donate_dynamic_handle_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_donate_dynamic(&desc, &acc_desc, &regions, 1,
					     NULL, &tx_mem_transaction_buffer));
}

TEST(sp_memory_management, sp_memory_donate_dynamic_invalid_buffer)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_donate_dynamic(&desc, &acc_desc, &regions, 1,
					     &handle, NULL));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_donate_dynamic_ffa_error)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint32_t region_count = 1;
	uint64_t handle = 1;
	const uint64_t expected_handle = 0;
	const sp_result result = FFA_ABORTED;
	const uint32_t expected_size = get_expected_size(region_count);

	expect_ffa_mem_donate(expected_size, expected_size, tx_buffer,
			      buffer_size / FFA_MEM_TRANSACTION_PAGE_SIZE,
			      &expected_handle, result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_memory_donate_dynamic(&desc, &acc_desc, &regions,
					     region_count, &handle,
					     &tx_mem_transaction_buffer));
	UNSIGNED_LONGLONGS_EQUAL(expected_handle, handle);
}

TEST(sp_memory_management, sp_memory_donate_dynamic)
{
	struct sp_memory_descriptor desc = SP_MEM_DESC_C(sender_id, tag);
	struct sp_memory_access_descriptor acc_desc = { .receiver_id =
								receiver_id };
	struct sp_memory_region regions = { .address = ptr,
					    .page_count = page_count };
	uint32_t region_count = 1;
	uint64_t handle = 1;
	const uint64_t expected_handle = 0x123456789abcdef0U;
	const uint32_t expected_size = get_expected_size(region_count);

	expect_ffa_mem_donate(expected_size, expected_size, tx_buffer,
			      buffer_size / FFA_MEM_TRANSACTION_PAGE_SIZE,
			      &expected_handle, FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_memory_donate_dynamic(&desc, &acc_desc, &regions,
					     region_count, &handle,
					     &tx_mem_transaction_buffer));
	UNSIGNED_LONGLONGS_EQUAL(expected_handle, handle);

	const struct ffa_mem_transaction_desc expected_transaction =
		FFA_MEM_DESC_C(sender_id, 0, tag);
	const struct ffa_mem_access_perm_desc expected_access_perm_desc = {
		.endpoint_id = receiver_id
	};
	const struct ffa_constituent_mem_region_desc expected_regions = {
		.address = (uint64_t)ptr, .page_count = page_count
	};
	check_descriptors(&expected_transaction, &expected_access_perm_desc,
			  &expected_regions, 1);
}

TEST(sp_memory_management, sp_memory_donate_dynamic_is_supported_null)
{
	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_donate_dynamic_is_supported(NULL));
}

TEST(sp_memory_management, sp_memory_donate_dynamic_is_supported_ffa_error)
{
	bool flag = true;
	struct ffa_interface_properties props = { 0 };
	expect_ffa_features(FFA_MEM_DONATE_32, &props, result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_memory_donate_dynamic_is_supported(&flag));
	CHECK_FALSE(flag);
}

TEST(sp_memory_management, sp_memory_donate_dynamic_is_supported)
{
	bool flag = false;
	struct ffa_interface_properties props = { 0 };
	props.interface_properties[FFA_FEATURES_MEM_DYNAMIC_BUFFER_SUPPORT_INDEX] =
		FFA_FEATURES_MEM_DYNAMIC_BUFFER_SUPPORT;
	expect_ffa_features(FFA_MEM_DONATE_32, &props, FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK, sp_memory_donate_dynamic_is_supported(&flag));
	CHECK_TRUE(flag);
}

TEST(sp_memory_management, sp_memory_lend_descriptor_null)
{
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_lend(NULL, &acc_desc, 1, &regions, 1, &handle));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_lend_acc_descriptor_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_lend(&desc, NULL, 1, &regions, 1, &handle));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_lend_acc_desc_count_zero)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_lend(&desc, &acc_desc, 0, &regions, 1, &handle));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_lend_regions_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_lend(&desc, &acc_desc, 1, NULL, 1, &handle));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_lend_region_count_zero)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_lend(&desc, &acc_desc, 1, &regions, 0, &handle));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_lend_handle_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_lend(&desc, &acc_desc, 1, &regions, 1, NULL));
}

TEST(sp_memory_management, sp_memory_lend_get_tx_error)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;
	const sp_result result = SP_RESULT_INTERNAL_ERROR;

	expect_sp_rxtx_buffer_tx_get(&tx_buffer, &buffer_size, result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_memory_lend(&desc, &acc_desc, 1, &regions, 1, &handle));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_lend_ffa_error)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint32_t region_count = 1;
	uint64_t handle = 1;
	const uint64_t expected_handle = 0;
	const sp_result result = FFA_ABORTED;
	const uint32_t expected_size = get_expected_size(region_count);

	expect_sp_rxtx_buffer_tx_get(&tx_buffer, &buffer_size, SP_RESULT_OK);
	expect_ffa_mem_lend_rxtx(expected_size, expected_size, &expected_handle,
				 result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_memory_lend(&desc, &acc_desc, 1, &regions, region_count,
				   &handle));
	UNSIGNED_LONGLONGS_EQUAL(expected_handle, handle);
}

TEST(sp_memory_management, sp_memory_lend)
{
	struct sp_memory_descriptor desc = SP_MEM_DESC_C(sender_id, tag);
	struct sp_memory_access_descriptor acc_desc = { .receiver_id =
								receiver_id };
	struct sp_memory_region regions = { .address = ptr,
					    .page_count = page_count };
	uint32_t region_count = 1;
	uint64_t handle = 1;
	const uint64_t expected_handle = 0x123456789abcdef0U;
	const uint32_t expected_size = get_expected_size(region_count);

	expect_sp_rxtx_buffer_tx_get(&tx_buffer, &buffer_size, SP_RESULT_OK);
	expect_ffa_mem_lend_rxtx(expected_size, expected_size, &expected_handle,
				 FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK, sp_memory_lend(&desc, &acc_desc, 1, &regions,
						 region_count, &handle));
	UNSIGNED_LONGLONGS_EQUAL(expected_handle, handle);

	const struct ffa_mem_transaction_desc expected_transaction =
		FFA_MEM_DESC_C(sender_id, 0, tag);
	const struct ffa_mem_access_perm_desc expected_access_perm_desc = {
		.endpoint_id = receiver_id
	};
	const struct ffa_constituent_mem_region_desc expected_regions = {
		.address = (uint64_t)ptr, .page_count = page_count
	};
	check_descriptors(&expected_transaction, &expected_access_perm_desc,
			  &expected_regions, 1);
}

TEST(sp_memory_management, sp_memory_lend_dynamic_descriptor_null)
{
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_lend_dynamic(NULL, &acc_desc, 1, &regions, 1,
					   &handle,
					   &tx_mem_transaction_buffer));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_lend_dynamic_acc_descriptor_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_lend_dynamic(&desc, NULL, 1, &regions, 1, &handle,
					   &tx_mem_transaction_buffer));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_lend_dynamic_acc_desc_count_zero)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_lend_dynamic(&desc, &acc_desc, 0, &regions, 1,
					   &handle,
					   &tx_mem_transaction_buffer));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_lend_dynamic_regions_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_lend_dynamic(&desc, &acc_desc, 1, NULL, 1,
					   &handle,
					   &tx_mem_transaction_buffer));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_lend_dynamic_region_count_zero)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_lend_dynamic(&desc, &acc_desc, 1, &regions, 0,
					   &handle,
					   &tx_mem_transaction_buffer));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_lend_dynamic_handle_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_lend_dynamic(&desc, &acc_desc, 1, &regions, 1,
					   NULL, &tx_mem_transaction_buffer));
}

TEST(sp_memory_management, sp_memory_lend_dynamic_invalid_buffer)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_lend_dynamic(&desc, &acc_desc, 1, &regions, 1,
					   &handle, NULL));
}

TEST(sp_memory_management, sp_memory_lend_dynamic_ffa_error)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint32_t region_count = 1;
	uint64_t handle = 1;
	const uint64_t expected_handle = 0;
	const sp_result result = FFA_ABORTED;
	const uint32_t expected_size = get_expected_size(region_count);

	expect_ffa_mem_lend(expected_size, expected_size, tx_buffer,
			    buffer_size / FFA_MEM_TRANSACTION_PAGE_SIZE,
			    &expected_handle, result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_memory_lend_dynamic(&desc, &acc_desc, 1, &regions,
					   region_count, &handle,
					   &tx_mem_transaction_buffer));
	UNSIGNED_LONGLONGS_EQUAL(expected_handle, handle);
}

TEST(sp_memory_management, sp_memory_lend_dynamic)
{
	struct sp_memory_descriptor desc = SP_MEM_DESC_C(sender_id, tag);
	struct sp_memory_access_descriptor acc_desc = { .receiver_id =
								receiver_id };
	struct sp_memory_region regions = { .address = ptr,
					    .page_count = page_count };
	uint32_t region_count = 1;
	uint64_t handle = 1;
	const uint64_t expected_handle = 0x123456789abcdef0U;
	const uint32_t expected_size = get_expected_size(region_count);

	expect_ffa_mem_lend(expected_size, expected_size, tx_buffer,
			    buffer_size / FFA_MEM_TRANSACTION_PAGE_SIZE,
			    &expected_handle, FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_memory_lend_dynamic(&desc, &acc_desc, 1, &regions,
					   region_count, &handle,
					   &tx_mem_transaction_buffer));
	UNSIGNED_LONGLONGS_EQUAL(expected_handle, handle);

	const struct ffa_mem_transaction_desc expected_transaction =
		FFA_MEM_DESC_C(sender_id, 0, tag);
	const struct ffa_mem_access_perm_desc expected_access_perm_desc = {
		.endpoint_id = receiver_id
	};
	const struct ffa_constituent_mem_region_desc expected_regions = {
		.address = (uint64_t)ptr, .page_count = page_count
	};
	check_descriptors(&expected_transaction, &expected_access_perm_desc,
			  &expected_regions, region_count);
}

TEST(sp_memory_management, sp_memory_lend_dynamic_is_supported_null)
{
	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_lend_dynamic_is_supported(NULL));
}

TEST(sp_memory_management, sp_memory_lend_dynamic_is_supported_ffa_error)
{
	bool flag = true;
	struct ffa_interface_properties props = { 0 };
	expect_ffa_features(FFA_MEM_LEND_32, &props, result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_memory_lend_dynamic_is_supported(&flag));
	CHECK_FALSE(flag);
}

TEST(sp_memory_management, sp_memory_lend_dynamic_is_supported)
{
	bool flag = false;
	struct ffa_interface_properties props = { 0 };
	props.interface_properties[FFA_FEATURES_MEM_DYNAMIC_BUFFER_SUPPORT_INDEX] =
		FFA_FEATURES_MEM_DYNAMIC_BUFFER_SUPPORT;
	expect_ffa_features(FFA_MEM_LEND_32, &props, FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK, sp_memory_lend_dynamic_is_supported(&flag));
	CHECK_TRUE(flag);
}

TEST(sp_memory_management, sp_memory_share_descriptor_null)
{
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_share(NULL, &acc_desc, 1, &regions, 1, &handle));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_share_acc_descriptor_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_share(&desc, NULL, 1, &regions, 1, &handle));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_share_acc_desc_count_zero)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_share(&desc, &acc_desc, 0, &regions, 1, &handle));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_share_regions_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_share(&desc, &acc_desc, 1, NULL, 1, &handle));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_share_region_count_zero)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_share(&desc, &acc_desc, 1, &regions, 0, &handle));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_share_handle_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_share(&desc, &acc_desc, 1, &regions, 1, NULL));
}

TEST(sp_memory_management, sp_memory_share_get_tx_error)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;
	const sp_result result = SP_RESULT_INTERNAL_ERROR;

	expect_sp_rxtx_buffer_tx_get(&tx_buffer, &buffer_size, result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_memory_share(&desc, &acc_desc, 1, &regions, 1, &handle));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_share_ffa_error)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint32_t region_count = 1;
	uint64_t handle = 1;
	const uint64_t expected_handle = 0;
	const sp_result result = FFA_ABORTED;
	const uint32_t expected_size = get_expected_size(region_count);

	expect_sp_rxtx_buffer_tx_get(&tx_buffer, &buffer_size, SP_RESULT_OK);
	expect_ffa_mem_share_rxtx(expected_size, expected_size,
				  &expected_handle, result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_memory_share(&desc, &acc_desc, 1, &regions, region_count,
				    &handle));
	UNSIGNED_LONGLONGS_EQUAL(expected_handle, handle);
}

TEST(sp_memory_management, sp_memory_share)
{
	struct sp_memory_descriptor desc = SP_MEM_DESC_C(sender_id, tag);
	struct sp_memory_access_descriptor acc_desc = { .receiver_id =
								receiver_id };
	struct sp_memory_region regions = { .address = ptr,
					    .page_count = page_count };
	uint32_t region_count = 1;
	uint64_t handle = 1;
	const uint64_t expected_handle = 0x123456789abcdef0U;
	const uint32_t expected_size = get_expected_size(region_count);

	expect_sp_rxtx_buffer_tx_get(&tx_buffer, &buffer_size, SP_RESULT_OK);
	expect_ffa_mem_share_rxtx(expected_size, expected_size,
				  &expected_handle, FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK, sp_memory_share(&desc, &acc_desc, 1, &regions,
						  region_count, &handle));
	UNSIGNED_LONGLONGS_EQUAL(expected_handle, handle);

	const struct ffa_mem_transaction_desc expected_transaction =
		FFA_MEM_DESC_C(sender_id, 0, tag);
	const struct ffa_mem_access_perm_desc expected_access_perm_desc = {
		.endpoint_id = receiver_id
	};
	const struct ffa_constituent_mem_region_desc expected_regions = {
		.address = (uint64_t)ptr, .page_count = page_count
	};
	check_descriptors(&expected_transaction, &expected_access_perm_desc,
			  &expected_regions, region_count);
}

TEST(sp_memory_management, sp_memory_share_dynamic_descriptor_null)
{
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_share_dynamic(NULL, &acc_desc, 1, &regions, 1,
					    &handle,
					    &tx_mem_transaction_buffer));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_share_dynamic_acc_descriptor_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_share_dynamic(&desc, NULL, 1, &regions, 1,
					    &handle,
					    &tx_mem_transaction_buffer));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_share_dynamic_acc_desc_count_zero)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_share_dynamic(&desc, &acc_desc, 0, &regions, 1,
					    &handle,
					    &tx_mem_transaction_buffer));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_share_dynamic_regions_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_share_dynamic(&desc, &acc_desc, 1, NULL, 1,
					    &handle,
					    &tx_mem_transaction_buffer));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_share_dynamic_region_count_zero)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_share_dynamic(&desc, &acc_desc, 1, &regions, 0,
					    &handle,
					    &tx_mem_transaction_buffer));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_share_dynamic_handle_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_share_dynamic(&desc, &acc_desc, 1, &regions, 1,
					    NULL, &tx_mem_transaction_buffer));
}

TEST(sp_memory_management, sp_memory_share_dynamic_invalid_buffer)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint64_t handle = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_share_dynamic(&desc, &acc_desc, 1, &regions, 1,
					    &handle, NULL));
	UNSIGNED_LONGLONGS_EQUAL(0, handle);
}

TEST(sp_memory_management, sp_memory_share_dynamic_ffa_error)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint32_t region_count = 1;
	uint64_t handle = 1;
	const uint64_t expected_handle = 0;
	const sp_result result = FFA_ABORTED;
	const uint32_t expected_size = get_expected_size(region_count);

	expect_ffa_mem_share(expected_size, expected_size, tx_buffer,
			     buffer_size / FFA_MEM_TRANSACTION_PAGE_SIZE,
			     &expected_handle, result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_memory_share_dynamic(&desc, &acc_desc, 1, &regions,
					    region_count, &handle,
					    &tx_mem_transaction_buffer));
	UNSIGNED_LONGLONGS_EQUAL(expected_handle, handle);
}

TEST(sp_memory_management, sp_memory_share_dynamic)
{
	struct sp_memory_descriptor desc = SP_MEM_DESC_C(sender_id, tag);
	struct sp_memory_access_descriptor acc_desc = { .receiver_id =
								receiver_id };
	struct sp_memory_region regions = { .address = ptr,
					    .page_count = page_count };
	uint32_t region_count = 1;
	uint64_t handle = 1;
	const uint64_t expected_handle = 0x123456789abcdef0U;
	const uint32_t expected_size = get_expected_size(region_count);

	expect_ffa_mem_share(expected_size, expected_size, tx_buffer,
			     buffer_size / FFA_MEM_TRANSACTION_PAGE_SIZE,
			     &expected_handle, FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_memory_share_dynamic(&desc, &acc_desc, 1, &regions,
					    region_count, &handle,
					    &tx_mem_transaction_buffer));
	UNSIGNED_LONGLONGS_EQUAL(expected_handle, handle);

	const struct ffa_mem_transaction_desc expected_transaction =
		FFA_MEM_DESC_C(sender_id, 0, tag);
	const struct ffa_mem_access_perm_desc expected_access_perm_desc = {
		.endpoint_id = receiver_id
	};
	const struct ffa_constituent_mem_region_desc expected_regions = {
		.address = (uint64_t)ptr, .page_count = page_count
	};
	check_descriptors(&expected_transaction, &expected_access_perm_desc,
			  &expected_regions, region_count);
}

TEST(sp_memory_management, sp_memory_share_dynamic_is_supported_null)
{
	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_share_dynamic_is_supported(NULL));
}

TEST(sp_memory_management, sp_memory_share_dynamic_is_supported_ffa_error)
{
	bool flag = true;
	struct ffa_interface_properties props = { 0 };
	expect_ffa_features(FFA_MEM_SHARE_32, &props, result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_memory_share_dynamic_is_supported(&flag));
	CHECK_FALSE(flag);
}

TEST(sp_memory_management, sp_memory_share_dynamic_is_supported)
{
	bool flag = false;
	struct ffa_interface_properties props = { 0 };
	props.interface_properties[FFA_FEATURES_MEM_DYNAMIC_BUFFER_SUPPORT_INDEX] =
		FFA_FEATURES_MEM_DYNAMIC_BUFFER_SUPPORT;
	expect_ffa_features(FFA_MEM_SHARE_32, &props, FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK, sp_memory_share_dynamic_is_supported(&flag));
	CHECK_TRUE(flag);
}

TEST(sp_memory_management, sp_memory_retrieve_descriptor_null)
{
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint32_t out_region_count = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_retrieve(NULL, &acc_desc, &regions, 1,
				       &out_region_count, handle));
	UNSIGNED_LONGS_EQUAL(0, out_region_count);
}

TEST(sp_memory_management, sp_memory_retrieve_acc_desc_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint32_t out_region_count = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_retrieve(&desc, NULL, &regions, 1,
				       &out_region_count, handle));
	UNSIGNED_LONGS_EQUAL(0, out_region_count);
}

TEST(sp_memory_management, sp_memory_retrieve_regions_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	uint32_t out_region_count = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_retrieve(&desc, &acc_desc, NULL, 1,
				       &out_region_count, handle));
	UNSIGNED_LONGS_EQUAL(0, out_region_count);
}

TEST(sp_memory_management, sp_memory_retrieve_out_region_count_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_retrieve(&desc, &acc_desc, &regions, 1, NULL,
				       handle));
}

TEST(sp_memory_management, sp_memory_retrieve_get_tx_error)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint32_t out_region_count = 1;

	expect_sp_rxtx_buffer_tx_get(&tx_buffer, &buffer_size, result);
	LONGS_EQUAL(result, sp_memory_retrieve(&desc, &acc_desc, &regions, 1,
					       &out_region_count, handle));
	UNSIGNED_LONGS_EQUAL(0, out_region_count);
}

TEST(sp_memory_management, sp_memory_retrieve_get_rx_error)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint32_t out_region_count = 1;

	expect_sp_rxtx_buffer_tx_get(&tx_buffer, &buffer_size, SP_RESULT_OK);
	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &buffer_size, result);
	LONGS_EQUAL(result, sp_memory_retrieve(&desc, &acc_desc, &regions, 1,
					       &out_region_count, handle));
	UNSIGNED_LONGS_EQUAL(0, out_region_count);
}

TEST(sp_memory_management, sp_memory_retrieve_ffa_error)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint32_t in_region_count = 1;
	uint32_t out_region_count = 1;
	ffa_result result = FFA_ABORTED;
	const uint32_t expected_size = get_expected_size(in_region_count);
	const uint32_t resp_total_length = 0;
	const uint32_t resp_fragment_length = 0;

	expect_sp_rxtx_buffer_tx_get(&tx_buffer, &buffer_size, SP_RESULT_OK);
	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &buffer_size, SP_RESULT_OK);
	expect_ffa_mem_retrieve_req_rxtx(expected_size, expected_size,
					 &resp_total_length,
					 &resp_fragment_length, result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_memory_retrieve(&desc, &acc_desc, &regions,
				       in_region_count, &out_region_count,
				       handle));
	UNSIGNED_LONGS_EQUAL(0, out_region_count);
}

TEST(sp_memory_management, sp_memory_retrieve_ffa_rx_release_error)
{
	struct sp_memory_descriptor desc = SP_MEM_DESC_C(sender_id, tag);
	struct sp_memory_access_descriptor acc_desc = { .receiver_id =
								receiver_id };
	struct sp_memory_region regions = { .address = ptr,
					    .page_count = page_count };
	uint32_t in_region_count = 1;
	uint32_t out_region_count = 1;
	const uint32_t expected_size = get_expected_size(in_region_count);
	const uint32_t resp_total_length = expected_size;
	const uint32_t resp_fragment_length = expected_size;

	/* Filling RX buffer */
	ffa_init_mem_transaction_desc(&rx_mem_transaction_buffer, sender_id + 1,
				      0, 0, handle, tag);
	ffa_add_mem_access_desc(&rx_mem_transaction_buffer, receiver_id + 1, 0,
				0);
	ffa_add_memory_region(&rx_mem_transaction_buffer, ptr, page_count);

	/* Exercising */
	expect_sp_rxtx_buffer_tx_get(&tx_buffer, &buffer_size, SP_RESULT_OK);
	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &buffer_size, SP_RESULT_OK);
	expect_ffa_rx_release(FFA_DENIED);
	expect_ffa_mem_retrieve_req_rxtx(expected_size, expected_size,
					 &resp_total_length,
					 &resp_fragment_length, FFA_OK);
	LONGS_EQUAL(SP_RESULT_FFA(FFA_DENIED),
		    sp_memory_retrieve(&desc, &acc_desc, &regions,
				       in_region_count, &out_region_count,
				       handle));
}

TEST(sp_memory_management, sp_memory_retrieve_fragmented_response)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint32_t in_region_count = 1;
	uint32_t out_region_count = 1;
	const uint32_t expected_size = get_expected_size(in_region_count);
	const uint32_t resp_total_length = 1;
	const uint32_t resp_fragment_length = 0;

	expect_sp_rxtx_buffer_tx_get(&tx_buffer, &buffer_size, SP_RESULT_OK);
	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &buffer_size, SP_RESULT_OK);
	expect_ffa_rx_release(FFA_OK);
	expect_ffa_mem_retrieve_req_rxtx(expected_size, expected_size,
					 &resp_total_length,
					 &resp_fragment_length, FFA_OK);
	LONGS_EQUAL(SP_RESULT_INTERNAL_ERROR,
		    sp_memory_retrieve(&desc, &acc_desc, &regions,
				       in_region_count, &out_region_count,
				       handle));
	UNSIGNED_LONGS_EQUAL(0, out_region_count);
}

TEST(sp_memory_management, sp_memory_retrieve_in_out)
{
	struct sp_memory_descriptor desc = SP_MEM_DESC_C(sender_id, tag);
	struct sp_memory_access_descriptor acc_desc = { .receiver_id =
								receiver_id };
	struct sp_memory_region regions = { .address = ptr,
					    .page_count = page_count };
	uint32_t in_region_count = 1;
	uint32_t out_region_count = 1;
	const uint32_t expected_size = get_expected_size(in_region_count);
	const uint32_t resp_total_length = expected_size;
	const uint32_t resp_fragment_length = expected_size;

	/* Filling RX buffer */
	ffa_init_mem_transaction_desc(&rx_mem_transaction_buffer, sender_id + 1,
				      0, 0, handle, tag);
	ffa_add_mem_access_desc(&rx_mem_transaction_buffer, receiver_id + 1, 0,
				0);
	ffa_add_memory_region(&rx_mem_transaction_buffer, ptr, page_count);

	/* Exercising */
	expect_sp_rxtx_buffer_tx_get(&tx_buffer, &buffer_size, SP_RESULT_OK);
	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &buffer_size, SP_RESULT_OK);
	expect_ffa_rx_release(FFA_OK);
	expect_ffa_mem_retrieve_req_rxtx(expected_size, expected_size,
					 &resp_total_length,
					 &resp_fragment_length, FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_memory_retrieve(&desc, &acc_desc, &regions,
				       in_region_count, &out_region_count,
				       handle));
	UNSIGNED_LONGS_EQUAL(1, out_region_count);

	/* Checking TX buffer */
	const struct ffa_mem_transaction_desc expected_transaction =
		FFA_MEM_DESC_C(sender_id, handle, tag);
	const struct ffa_mem_access_perm_desc expected_access_perm_desc = {
		.endpoint_id = receiver_id
	};
	const struct ffa_constituent_mem_region_desc expected_regions = {
		.address = (uint64_t)ptr, .page_count = page_count
	};
	check_descriptors(&expected_transaction, &expected_access_perm_desc,
			  &expected_regions, in_region_count);

	/* Checking output */
	UNSIGNED_LONGS_EQUAL(sender_id + 1, desc.sender_id);
	UNSIGNED_LONGS_EQUAL(receiver_id + 1, acc_desc.receiver_id);
	UNSIGNED_LONGLONGS_EQUAL(tag, desc.tag);
}

TEST(sp_memory_management, sp_memory_retrieve_dynamic_descriptor_null)
{
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint32_t out_region_count = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_retrieve_dynamic(NULL, &acc_desc, &regions, 1,
					       &out_region_count, handle,
					       &tx_mem_transaction_buffer));
	UNSIGNED_LONGS_EQUAL(0, out_region_count);
}

TEST(sp_memory_management, sp_memory_retrieve_dynamic_acc_desc_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint32_t out_region_count = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_retrieve_dynamic(&desc, NULL, &regions, 1,
					       &out_region_count, handle,
					       &tx_mem_transaction_buffer));
	UNSIGNED_LONGS_EQUAL(0, out_region_count);
}

TEST(sp_memory_management, sp_memory_retrieve_dynamic_regions_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	uint32_t out_region_count = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_retrieve_dynamic(&desc, &acc_desc, NULL, 1,
					       &out_region_count, handle,
					       &tx_mem_transaction_buffer));
	UNSIGNED_LONGS_EQUAL(0, out_region_count);
}

TEST(sp_memory_management, sp_memory_retrieve_dynamic_out_region_count_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_retrieve_dynamic(&desc, &acc_desc, &regions, 1,
					       NULL, handle,
					       &tx_mem_transaction_buffer));
}

TEST(sp_memory_management, sp_memory_retrieve_dynamic_handle_zero)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint32_t out_region_count = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_retrieve_dynamic(&desc, &acc_desc, &regions, 1,
					       &out_region_count, 0,
					       &tx_mem_transaction_buffer));
}

TEST(sp_memory_management, sp_memory_retrieve_dynamic_buffer_null)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint32_t out_region_count = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_retrieve_dynamic(&desc, &acc_desc, &regions, 1,
					       &out_region_count, handle,
					       NULL));
}

TEST(sp_memory_management, sp_memory_retrieve_dynamic_ffa_error)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint32_t in_region_count = 1;
	uint32_t out_region_count = 1;
	ffa_result result = FFA_ABORTED;
	const uint32_t expected_size = get_expected_size(in_region_count);
	const uint32_t resp_total_length = 0;
	const uint32_t resp_fragment_length = 0;

	expect_ffa_mem_retrieve_req(expected_size, expected_size, tx_buffer,
				    buffer_size / FFA_MEM_TRANSACTION_PAGE_SIZE,
				    &resp_total_length, &resp_fragment_length,
				    result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_memory_retrieve_dynamic(&desc, &acc_desc, &regions,
					       in_region_count,
					       &out_region_count, handle,
					       &tx_mem_transaction_buffer));
	UNSIGNED_LONGS_EQUAL(0, out_region_count);
}

TEST(sp_memory_management, sp_memory_retrieve_dynamic_fragmented_response)
{
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_access_descriptor acc_desc = { 0 };
	struct sp_memory_region regions = { 0 };
	uint32_t in_region_count = 1;
	uint32_t out_region_count = 1;
	const uint32_t expected_size = get_expected_size(in_region_count);
	const uint32_t resp_total_length = 1;
	const uint32_t resp_fragment_length = 0;

	expect_ffa_mem_retrieve_req(expected_size, expected_size, tx_buffer,
				    buffer_size / FFA_MEM_TRANSACTION_PAGE_SIZE,
				    &resp_total_length, &resp_fragment_length,
				    FFA_OK);
	LONGS_EQUAL(SP_RESULT_INTERNAL_ERROR,
		    sp_memory_retrieve_dynamic(&desc, &acc_desc, &regions,
					       in_region_count,
					       &out_region_count, handle,
					       &tx_mem_transaction_buffer));
	UNSIGNED_LONGS_EQUAL(0, out_region_count);
}

TEST(sp_memory_management, sp_memory_retrieve_dynamic_in_out)
{
	struct sp_memory_descriptor desc = SP_MEM_DESC_C(sender_id, tag);
	struct sp_memory_access_descriptor acc_desc = { .receiver_id =
								receiver_id };
	struct sp_memory_region regions = { .address = ptr,
					    .page_count = page_count };
	uint32_t in_region_count = 1;
	uint32_t out_region_count = 1;
	const uint32_t expected_size = get_expected_size(in_region_count);
	const uint32_t resp_total_length = expected_size;
	const uint32_t resp_fragment_length = expected_size;

	/* Exercising */
	expect_ffa_mem_retrieve_req(expected_size, expected_size, tx_buffer,
				    buffer_size / FFA_MEM_TRANSACTION_PAGE_SIZE,
				    &resp_total_length, &resp_fragment_length,
				    FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK, sp_memory_retrieve_dynamic(
					  &desc, &acc_desc, &regions,
					  in_region_count, &out_region_count,
					  handle, &tx_mem_transaction_buffer));
	UNSIGNED_LONGS_EQUAL(1, out_region_count);

	/* Checking TX buffer */
	const struct ffa_mem_transaction_desc expected_transaction =
		FFA_MEM_DESC_C(sender_id, handle, tag);
	const struct ffa_mem_access_perm_desc expected_access_perm_desc = {
		.endpoint_id = receiver_id
	};
	const struct ffa_constituent_mem_region_desc expected_regions = {
		.address = (uint64_t)ptr, .page_count = page_count
	};
	check_descriptors(&expected_transaction, &expected_access_perm_desc,
			  &expected_regions, in_region_count);

	/* Checking output */
	UNSIGNED_LONGS_EQUAL(sender_id, desc.sender_id);
	UNSIGNED_LONGS_EQUAL(receiver_id, acc_desc.receiver_id);
	UNSIGNED_LONGLONGS_EQUAL(tag, desc.tag);
}

TEST(sp_memory_management, sp_memory_retrieve_dynamic_is_supported_null)
{
	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_retrieve_dynamic_is_supported(NULL));
}

TEST(sp_memory_management, sp_memory_retrieve_dynamic_is_supported_ffa_error)
{
	bool flag = true;
	struct ffa_interface_properties props = { 0 };
	expect_ffa_features(FFA_MEM_RETRIEVE_REQ_32, &props, result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_memory_retrieve_dynamic_is_supported(&flag));
	CHECK_FALSE(flag);
}

TEST(sp_memory_management, sp_memory_retrieve_dynamic_is_supported)
{
	bool flag = false;
	struct ffa_interface_properties props = { 0 };
	props.interface_properties[FFA_FEATURES_MEM_DYNAMIC_BUFFER_SUPPORT_INDEX] =
		FFA_FEATURES_MEM_DYNAMIC_BUFFER_SUPPORT;
	expect_ffa_features(FFA_MEM_RETRIEVE_REQ_32, &props, FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_memory_retrieve_dynamic_is_supported(&flag));
	CHECK_TRUE(flag);
}

TEST(sp_memory_management, sp_memory_relinquish_endpoints_null)
{
	struct sp_memory_transaction_flags flags = { 0 };

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_relinquish(handle, NULL, 1, &flags));
}

TEST(sp_memory_management, sp_memory_relinquish_endpoints_count_zero)
{
	uint16_t endpoints = 0;
	struct sp_memory_transaction_flags flags = { 0 };

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_relinquish(handle, &endpoints, 0, &flags));
}

TEST(sp_memory_management, sp_memory_relinquish_endpoints_flags_null)
{
	uint16_t endpoints = 0;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_relinquish(handle, &endpoints, 1, NULL));
}

TEST(sp_memory_management, sp_memory_relinquish_get_tx_error)
{
	uint16_t endpoints = 0;
	struct sp_memory_transaction_flags flags = { 0 };

	expect_sp_rxtx_buffer_tx_get(&tx_buffer, &buffer_size, result);
	LONGS_EQUAL(result,
		    sp_memory_relinquish(handle, &endpoints, 1, &flags));
}

TEST(sp_memory_management, sp_memory_relinquish_small_buffer)
{
	assert_environment_t env;
	uint16_t endpoints = 0;
	struct sp_memory_transaction_flags flags = { 0 };
	size_t small_buffer_size = 1;

	expect_sp_rxtx_buffer_tx_get(&tx_buffer, &small_buffer_size,
				     SP_RESULT_OK);
	if (SETUP_ASSERT_ENVIRONMENT(env)) {
		LONGS_EQUAL(result, sp_memory_relinquish(handle, &endpoints, 1,
							 &flags));
	}
}

TEST(sp_memory_management, sp_memory_relinquish_ffa_error)
{
	uint16_t endpoints = 0;
	struct sp_memory_transaction_flags flags = { 0 };
	const ffa_result result = FFA_ABORTED;

	expect_sp_rxtx_buffer_tx_get(&tx_buffer, &buffer_size, SP_RESULT_OK);
	expect_ffa_mem_relinquish(result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_memory_relinquish(handle, &endpoints, 1, &flags));
}

TEST(sp_memory_management, sp_memory_relinquish)
{
	uint16_t endpoints = 12345;
	struct sp_memory_transaction_flags flags = { 0 };
	struct ffa_mem_relinquish_desc *relinquish_desc = NULL;

	expect_sp_rxtx_buffer_tx_get(&tx_buffer, &buffer_size, SP_RESULT_OK);
	expect_ffa_mem_relinquish(FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_memory_relinquish(handle, &endpoints, 1, &flags));

	relinquish_desc = (struct ffa_mem_relinquish_desc *)tx_buffer;
	UNSIGNED_LONGLONGS_EQUAL(handle, relinquish_desc->handle);
	UNSIGNED_LONGS_EQUAL(0, relinquish_desc->flags);
	UNSIGNED_LONGS_EQUAL(1, relinquish_desc->endpoint_count);
	UNSIGNED_LONGS_EQUAL(endpoints, relinquish_desc->endpoints[0]);
}

TEST(sp_memory_management, sp_memory_relinquish_two_endpoints)
{
	uint16_t endpoints[] = { 12345, 5432 };
	struct sp_memory_transaction_flags flags = { 0 };
	struct ffa_mem_relinquish_desc *relinquish_desc = NULL;

	expect_sp_rxtx_buffer_tx_get(&tx_buffer, &buffer_size, SP_RESULT_OK);
	expect_ffa_mem_relinquish(FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_memory_relinquish(handle, endpoints, 2, &flags));

	relinquish_desc = (struct ffa_mem_relinquish_desc *)tx_buffer;
	UNSIGNED_LONGLONGS_EQUAL(handle, relinquish_desc->handle);
	UNSIGNED_LONGS_EQUAL(0, relinquish_desc->flags);
	UNSIGNED_LONGS_EQUAL(2, relinquish_desc->endpoint_count);
	UNSIGNED_LONGS_EQUAL(endpoints[0], relinquish_desc->endpoints[0]);
	UNSIGNED_LONGS_EQUAL(endpoints[1], relinquish_desc->endpoints[1]);
}

TEST(sp_memory_management, sp_memory_reclaim_ffa_error)
{
	ffa_result result = FFA_ABORTED;

	expect_ffa_mem_reclaim(handle, flags, result);
	LONGS_EQUAL(SP_RESULT_FFA(result), sp_memory_reclaim(handle, flags));
}

TEST(sp_memory_management, sp_memory_reclaim)
{
	expect_ffa_mem_reclaim(handle, flags, FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK, sp_memory_reclaim(handle, flags));
}

TEST_GROUP(sp_memory_permission) {
	TEST_SETUP()
	{
		memset(&mem_perm, 0x00, sizeof(mem_perm));
	}

	TEST_TEARDOWN()
	{
		mock().checkExpectations();
		mock().clear();
	}

	const void *valid_base_address = (const void *)0xfffff000;
	const void *invalid_base_address = (const void *)0xfffff800;
	size_t valid_size = 0x0000f000;
	size_t invalid_size = 0x0000f800;
	struct sp_mem_perm mem_perm;
};

TEST(sp_memory_permission, sp_memory_permission_get_base_null)
{
	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_permission_get(NULL, &mem_perm));
}

TEST(sp_memory_permission, sp_memory_permission_get_mem_perm_null)
{
	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_permission_get(valid_base_address, NULL));
}

TEST(sp_memory_permission, sp_memory_permission_get_invalid_address)
{
	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_permission_get(invalid_base_address, &mem_perm));
}

TEST(sp_memory_permission, sp_memory_permission_get_ffa_error)
{
	ffa_result result = FFA_ABORTED;
	const uint32_t raw_perm = 0x7;

	expect_ffa_mem_perm_get(valid_base_address, &raw_perm, result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_memory_permission_get(valid_base_address, &mem_perm));
}

TEST(sp_memory_permission, sp_memory_permission_get_reserved_data_perm)
{
	const uint32_t raw_perm = 0x02;

	expect_ffa_mem_perm_get(valid_base_address, &raw_perm, FFA_OK);
	LONGS_EQUAL(SP_RESULT_INTERNAL_ERROR,
		    sp_memory_permission_get(valid_base_address, &mem_perm));
}

TEST(sp_memory_permission, sp_memory_permission_get_no_data_access)
{
	const uint32_t raw_perm = 0x00;

	expect_ffa_mem_perm_get(valid_base_address, &raw_perm, FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_memory_permission_get(valid_base_address, &mem_perm));
	CHECK_EQUAL(sp_mem_perm_data_perm_no_access, mem_perm.data_access);
	CHECK_EQUAL(sp_mem_perm_instruction_perm_executable, mem_perm.instruction_access);
}

TEST(sp_memory_permission, sp_memory_permission_get_rw_data_access)
{
	const uint32_t raw_perm = 0x01;

	expect_ffa_mem_perm_get(valid_base_address, &raw_perm, FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_memory_permission_get(valid_base_address, &mem_perm));
	CHECK_EQUAL(sp_mem_perm_data_perm_read_write, mem_perm.data_access);
	CHECK_EQUAL(sp_mem_perm_instruction_perm_executable, mem_perm.instruction_access);
}

TEST(sp_memory_permission, sp_memory_permission_get_ro_data_access)
{
	const uint32_t raw_perm = 0x03;

	expect_ffa_mem_perm_get(valid_base_address, &raw_perm, FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_memory_permission_get(valid_base_address, &mem_perm));
	CHECK_EQUAL(sp_mem_perm_data_perm_read_only, mem_perm.data_access);
	CHECK_EQUAL(sp_mem_perm_instruction_perm_executable, mem_perm.instruction_access);
}

TEST(sp_memory_permission, sp_memory_permission_get_non_executable)
{
	const uint32_t raw_perm = 0x07;

	expect_ffa_mem_perm_get(valid_base_address, &raw_perm, FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_memory_permission_get(valid_base_address, &mem_perm));
	CHECK_EQUAL(sp_mem_perm_data_perm_read_only, mem_perm.data_access);
	CHECK_EQUAL(sp_mem_perm_instruction_perm_non_executable, mem_perm.instruction_access);
}

TEST(sp_memory_permission, sp_memory_permission_set_base_null)
{
	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_permission_set(NULL, valid_size, &mem_perm));
}

TEST(sp_memory_permission, sp_memory_permission_set_size_zero)
{
	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_permission_set(valid_base_address, 0, &mem_perm));
}

TEST(sp_memory_permission, sp_memory_permission_set_mem_perm_null)
{
	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_permission_set(valid_base_address, valid_size, NULL));
}

TEST(sp_memory_permission, sp_memory_permission_set_mem_perm_rwx)
{
	mem_perm.data_access = sp_mem_perm_data_perm_read_write;
	mem_perm.instruction_access = sp_mem_perm_instruction_perm_executable;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_permission_set(valid_base_address, valid_size, &mem_perm));
}

TEST(sp_memory_permission, sp_memory_permission_set_invalid_data_perm)
{
	mem_perm.data_access = sp_mem_perm_data_perm_reserved;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_permission_set(valid_base_address, valid_size, &mem_perm));
}

TEST(sp_memory_permission, sp_memory_permission_set_invalid_instruction_perm)
{
	mem_perm.instruction_access = (sp_mem_perm_instruction_access_permission)0xff;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_permission_set(valid_base_address, valid_size, &mem_perm));
}

TEST(sp_memory_permission, sp_memory_permission_set_invalid_base_addr)
{
	mem_perm.data_access = sp_mem_perm_data_perm_read_write;
	mem_perm.instruction_access = sp_mem_perm_instruction_perm_non_executable;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_permission_set(invalid_base_address, valid_size, &mem_perm));
}

TEST(sp_memory_permission, sp_memory_permission_set_invalid_region_size)
{
	mem_perm.data_access = sp_mem_perm_data_perm_read_write;
	mem_perm.instruction_access = sp_mem_perm_instruction_perm_non_executable;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_permission_set(valid_base_address, invalid_size, &mem_perm));
}

TEST(sp_memory_permission, sp_memory_permission_set_too_large_region_size)
{
	mem_perm.data_access = sp_mem_perm_data_perm_read_write;
	mem_perm.instruction_access = sp_mem_perm_instruction_perm_non_executable;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_memory_permission_set(valid_base_address, 0x100000000000UL, &mem_perm));
}

TEST(sp_memory_permission, sp_memory_permission_set_ronx)
{
	mem_perm.data_access = sp_mem_perm_data_perm_read_only;
	mem_perm.instruction_access = sp_mem_perm_instruction_perm_non_executable;

	expect_ffa_mem_perm_set(valid_base_address, valid_size / 4096, 0x07, FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_memory_permission_set(valid_base_address, valid_size, &mem_perm));
}

TEST(sp_memory_permission, sp_memory_permission_set_rwnx)
{
	mem_perm.data_access = sp_mem_perm_data_perm_read_write;
	mem_perm.instruction_access = sp_mem_perm_instruction_perm_non_executable;

	expect_ffa_mem_perm_set(valid_base_address, valid_size / 4096, 0x05, FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_memory_permission_set(valid_base_address, valid_size, &mem_perm));
}

TEST(sp_memory_permission, sp_memory_permission_set_rox)
{
	mem_perm.data_access = sp_mem_perm_data_perm_read_only;
	mem_perm.instruction_access = sp_mem_perm_instruction_perm_executable;

	expect_ffa_mem_perm_set(valid_base_address, valid_size / 4096, 0x03, FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_memory_permission_set(valid_base_address, valid_size, &mem_perm));
}
