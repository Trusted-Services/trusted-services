// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2020-2022, Arm Limited. All rights reserved.
 */

#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <stdint.h>
#include <string.h>
#include "mock_ffa_api.h"
#include "../include/sp_rxtx.h"

TEST_GROUP(sp_rxtx)
{
	struct ffa_interface_properties props;
	void *expected_tx_buffer = (void *)0x12350000;
	void *expected_rx_buffer = (void *)0x12360000;
	size_t expected_page_count = ((1 << 6) - 1);
	size_t expected_size = expected_page_count * 4096;

	TEST_SETUP()
	{
		memset(&props, 0x00, sizeof(props));
	}

	TEST_TEARDOWN()
	{
		mock().disable();
		sp_rxtx_buffer_unmap();
		mock().enable();

		mock().checkExpectations();
		mock().clear();
	}

	void expect_sp_rxtx_buffer_alignment_boundary_get(size_t granularity,
							  ffa_result result)
	{
		props.interface_properties
			[FFA_FEATURES_RXTX_MAP_GRANULARITY_INDEX] =
			(granularity & FFA_FEATURES_RXTX_MAP_GRANULARITY_MASK)
			<< FFA_FEATURES_RXTX_MAP_GRANULARITY_SHIFT;
		expect_ffa_features(FFA_RXTX_MAP_64, &props, result);
	}

	void do_successful_mapping()
	{
		expect_sp_rxtx_buffer_alignment_boundary_get(
			FFA_FEATURES_RXTX_MAP_GRANULARITY_4K, FFA_OK);
		expect_ffa_rxtx_map(expected_tx_buffer, expected_rx_buffer,
				    expected_page_count, FFA_OK);
		LONGS_EQUAL(SP_RESULT_OK, sp_rxtx_buffer_map(expected_tx_buffer,
							     expected_rx_buffer,
							     expected_size));
	}
};

TEST(sp_rxtx, sp_rxtx_buffer_map_null_tx_buffer)
{
	void *tx_buffer = NULL;
	void *rx_buffer = (void *)0x12340000;
	size_t size = 65536;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_rxtx_buffer_map(tx_buffer, rx_buffer, size));
}

TEST(sp_rxtx, sp_rxtx_buffer_map_null_rx_buffer)
{
	void *tx_buffer = (void *)0x12340000;
	void *rx_buffer = NULL;
	size_t size = 65536;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_rxtx_buffer_map(tx_buffer, rx_buffer, size));
}

TEST(sp_rxtx, sp_rxtx_buffer_map_zero_size)
{
	void *tx_buffer = (void *)0x12340000;
	void *rx_buffer = (void *)0x12350000;
	size_t size = 0;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_rxtx_buffer_map(tx_buffer, rx_buffer, size));
}

TEST(sp_rxtx, sp_rxtx_buffer_map_get_buffer_alignment_fail)
{
	void *tx_buffer = (void *)0x12340000;
	void *rx_buffer = (void *)0x12350000;
	size_t size = 65536;
	ffa_result result = FFA_ABORTED;

	expect_sp_rxtx_buffer_alignment_boundary_get(
		FFA_FEATURES_RXTX_MAP_GRANULARITY_4K, result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_rxtx_buffer_map(tx_buffer, rx_buffer, size));
}

TEST(sp_rxtx, sp_rxtx_buffer_map_tx_alignment_fail_4k)
{
	void *tx_buffer = (void *)2048;
	void *rx_buffer = (void *)0x12350000;
	size_t size = 65536;

	expect_sp_rxtx_buffer_alignment_boundary_get(
		FFA_FEATURES_RXTX_MAP_GRANULARITY_4K, FFA_OK);
	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_rxtx_buffer_map(tx_buffer, rx_buffer, size));
}

TEST(sp_rxtx, sp_rxtx_buffer_map_tx_alignment_fail_16k)
{
	void *tx_buffer = (void *)8192;
	void *rx_buffer = (void *)0x12350000;
	size_t size = 65536;

	expect_sp_rxtx_buffer_alignment_boundary_get(
		FFA_FEATURES_RXTX_MAP_GRANULARITY_16K, FFA_OK);
	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_rxtx_buffer_map(tx_buffer, rx_buffer, size));
}

TEST(sp_rxtx, sp_rxtx_buffer_map_tx_alignment_fail_64k)
{
	void *tx_buffer = (void *)32768;
	void *rx_buffer = (void *)0x12350000;
	size_t size = 65536;

	expect_sp_rxtx_buffer_alignment_boundary_get(
		FFA_FEATURES_RXTX_MAP_GRANULARITY_64K, FFA_OK);
	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_rxtx_buffer_map(tx_buffer, rx_buffer, size));
}

TEST(sp_rxtx, sp_rxtx_buffer_map_rx_alignment_fail_4k)
{
	void *tx_buffer = (void *)0x12350000;
	void *rx_buffer = (void *)2048;
	size_t size = 65536;

	expect_sp_rxtx_buffer_alignment_boundary_get(
		FFA_FEATURES_RXTX_MAP_GRANULARITY_4K, FFA_OK);
	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_rxtx_buffer_map(tx_buffer, rx_buffer, size));
}

TEST(sp_rxtx, sp_rxtx_buffer_map_rx_alignment_fail_16k)
{
	void *tx_buffer = (void *)0x12350000;
	void *rx_buffer = (void *)8192;
	size_t size = 65536;

	expect_sp_rxtx_buffer_alignment_boundary_get(
		FFA_FEATURES_RXTX_MAP_GRANULARITY_16K, FFA_OK);
	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_rxtx_buffer_map(tx_buffer, rx_buffer, size));
}

TEST(sp_rxtx, sp_rxtx_buffer_map_rx_alignment_fail_64k)
{
	void *tx_buffer = (void *)0x12350000;
	void *rx_buffer = (void *)32768;
	size_t size = 65536;

	expect_sp_rxtx_buffer_alignment_boundary_get(
		FFA_FEATURES_RXTX_MAP_GRANULARITY_64K, FFA_OK);
	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_rxtx_buffer_map(tx_buffer, rx_buffer, size));
}

TEST(sp_rxtx, sp_rxtx_buffer_map_size_alignment_fail_4k)
{
	void *tx_buffer = (void *)0x12350000;
	void *rx_buffer = (void *)0x12360000;
	size_t size = 2048;

	expect_sp_rxtx_buffer_alignment_boundary_get(
		FFA_FEATURES_RXTX_MAP_GRANULARITY_4K, FFA_OK);
	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_rxtx_buffer_map(tx_buffer, rx_buffer, size));
}

TEST(sp_rxtx, sp_rxtx_buffer_map_size_alignment_fail_16k)
{
	void *tx_buffer = (void *)0x12350000;
	void *rx_buffer = (void *)0x12360000;
	size_t size = 8192;

	expect_sp_rxtx_buffer_alignment_boundary_get(
		FFA_FEATURES_RXTX_MAP_GRANULARITY_16K, FFA_OK);
	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_rxtx_buffer_map(tx_buffer, rx_buffer, size));
}

TEST(sp_rxtx, sp_rxtx_buffer_map_size_alignment_fail_64k)
{
	void *tx_buffer = (void *)0x12350000;
	void *rx_buffer = (void *)0x12360000;
	size_t size = 32768;

	expect_sp_rxtx_buffer_alignment_boundary_get(
		FFA_FEATURES_RXTX_MAP_GRANULARITY_64K, FFA_OK);
	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_rxtx_buffer_map(tx_buffer, rx_buffer, size));
}

TEST(sp_rxtx, sp_rxtx_buffer_map_size_max_fail)
{
	void *tx_buffer = (void *)0x12350000;
	void *rx_buffer = (void *)0x12360000;
	size_t page_count = (1 << 6);
	size_t size = page_count * 4096;

	expect_sp_rxtx_buffer_alignment_boundary_get(
		FFA_FEATURES_RXTX_MAP_GRANULARITY_4K, FFA_OK);
	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_rxtx_buffer_map(tx_buffer, rx_buffer, size));
}

TEST(sp_rxtx, sp_rxtx_buffer_map_ffa_rxtx_map_fail)
{
	void *tx_buffer = (void *)0x12350000;
	void *rx_buffer = (void *)0x12360000;
	size_t page_count = ((1 << 6) - 1);
	size_t size = page_count * 4096;
	ffa_result result = FFA_ABORTED;

	expect_sp_rxtx_buffer_alignment_boundary_get(
		FFA_FEATURES_RXTX_MAP_GRANULARITY_4K, FFA_OK);
	expect_ffa_rxtx_map(tx_buffer, rx_buffer, page_count, result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_rxtx_buffer_map(tx_buffer, rx_buffer, size));
}

TEST(sp_rxtx, sp_rxtx_buffer_map_success)
{
	void *tx_buffer = (void *)0x12350000;
	void *rx_buffer = (void *)0x12360000;
	size_t page_count = ((1 << 6) - 1);
	size_t size = page_count * 4096;
	ffa_result result = FFA_OK;

	expect_sp_rxtx_buffer_alignment_boundary_get(
		FFA_FEATURES_RXTX_MAP_GRANULARITY_4K, FFA_OK);
	expect_ffa_rxtx_map(tx_buffer, rx_buffer, page_count, result);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_rxtx_buffer_map(tx_buffer, rx_buffer, size));
}

TEST(sp_rxtx, sp_rxtx_buffer_map_double_map_fail)
{
	void *tx_buffer = (void *)0x12350000;
	void *rx_buffer = (void *)0x12360000;
	size_t page_count = ((1 << 6) - 1);
	size_t size = page_count * 4096;
	ffa_result result = FFA_OK;

	expect_sp_rxtx_buffer_alignment_boundary_get(
		FFA_FEATURES_RXTX_MAP_GRANULARITY_4K, FFA_OK);
	expect_ffa_rxtx_map(tx_buffer, rx_buffer, page_count, result);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_rxtx_buffer_map(tx_buffer, rx_buffer, size));
	LONGS_EQUAL(SP_RESULT_INVALID_STATE,
		    sp_rxtx_buffer_map(tx_buffer, rx_buffer, size));
}

TEST(sp_rxtx, sp_rxtx_buffer_unmap_not_mapped)
{
	LONGS_EQUAL(SP_RESULT_INVALID_STATE, sp_rxtx_buffer_unmap());
}

TEST(sp_rxtx, sp_rxtx_buffer_unmap_id_get_fail)
{
	const uint16_t id = 0x1234;
	ffa_result result = FFA_ABORTED;

	do_successful_mapping();
	expect_ffa_id_get(&id, result);
	LONGS_EQUAL(SP_RESULT_FFA(result), sp_rxtx_buffer_unmap());
}

TEST(sp_rxtx, sp_rxtx_buffer_unmap_rxtx_unmap_fail)
{
	const uint16_t id = 0x1234;
	ffa_result result = FFA_ABORTED;

	do_successful_mapping();
	expect_ffa_id_get(&id, FFA_OK);
	expect_ffa_rxtx_unmap(id, result);

	sp_result sp_res = sp_rxtx_buffer_unmap();
	LONGS_EQUAL(SP_RESULT_FFA(result), sp_res);
}

TEST(sp_rxtx, sp_rxtx_buffer_unmap_success)
{
	const uint16_t id = 0x1234;

	do_successful_mapping();
	expect_ffa_id_get(&id, FFA_OK);
	expect_ffa_rxtx_unmap(id, FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK, sp_rxtx_buffer_unmap());
}

TEST(sp_rxtx, sp_rxtx_buffer_alignment_boundary_get_null_ptr)
{
	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_rxtx_buffer_alignment_boundary_get(NULL));
}

TEST(sp_rxtx, sp_rxtx_buffer_alignment_boundary_get_ffa_error)
{
	uintptr_t alignment = 0xfedcba98U;
	struct ffa_interface_properties props = { 0 };
	ffa_result result = FFA_DENIED;

	props.interface_properties[FFA_FEATURES_RXTX_MAP_GRANULARITY_INDEX] =
		0 << FFA_FEATURES_RXTX_MAP_GRANULARITY_SHIFT;
	expect_ffa_features(FFA_RXTX_MAP_64, &props, result);

	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_rxtx_buffer_alignment_boundary_get(&alignment));
	UNSIGNED_LONGS_EQUAL(0, alignment);
}

TEST(sp_rxtx, sp_rxtx_buffer_alignment_boundary_get_4k)
{
	uintptr_t alignment = 0xfedcba98U;
	struct ffa_interface_properties props = { 0 };

	props.interface_properties[FFA_FEATURES_RXTX_MAP_GRANULARITY_INDEX] =
		0 << FFA_FEATURES_RXTX_MAP_GRANULARITY_SHIFT;
	expect_ffa_features(FFA_RXTX_MAP_64, &props, FFA_OK);

	LONGS_EQUAL(SP_RESULT_OK,
		    sp_rxtx_buffer_alignment_boundary_get(&alignment));
	UNSIGNED_LONGS_EQUAL(4 * 1024, alignment);
}

TEST(sp_rxtx, sp_rxtx_buffer_alignment_boundary_get_64k)
{
	uintptr_t alignment = 0xfedcba98U;
	struct ffa_interface_properties props = { 0 };

	props.interface_properties[FFA_FEATURES_RXTX_MAP_GRANULARITY_INDEX] =
		1 << FFA_FEATURES_RXTX_MAP_GRANULARITY_SHIFT;
	expect_ffa_features(FFA_RXTX_MAP_64, &props, FFA_OK);

	LONGS_EQUAL(SP_RESULT_OK,
		    sp_rxtx_buffer_alignment_boundary_get(&alignment));
	UNSIGNED_LONGS_EQUAL(64 * 1024, alignment);
}

TEST(sp_rxtx, sp_rxtx_buffer_alignment_boundary_get_16k)
{
	uintptr_t alignment = 0xfedcba98U;
	struct ffa_interface_properties props = { 0 };

	props.interface_properties[FFA_FEATURES_RXTX_MAP_GRANULARITY_INDEX] =
		2 << FFA_FEATURES_RXTX_MAP_GRANULARITY_SHIFT;
	expect_ffa_features(FFA_RXTX_MAP_64, &props, FFA_OK);

	LONGS_EQUAL(SP_RESULT_OK,
		    sp_rxtx_buffer_alignment_boundary_get(&alignment));
	UNSIGNED_LONGS_EQUAL(16 * 1024, alignment);
}

TEST(sp_rxtx, sp_rxtx_buffer_alignment_boundary_get_reserved)
{
	uintptr_t alignment = 0xfedcba98U;
	struct ffa_interface_properties props = { 0 };

	props.interface_properties[FFA_FEATURES_RXTX_MAP_GRANULARITY_INDEX] =
		3 << FFA_FEATURES_RXTX_MAP_GRANULARITY_SHIFT;
	expect_ffa_features(FFA_RXTX_MAP_64, &props, FFA_OK);

	LONGS_EQUAL(SP_RESULT_INTERNAL_ERROR,
		    sp_rxtx_buffer_alignment_boundary_get(&alignment));
	UNSIGNED_LONGS_EQUAL(0, alignment);
}

TEST(sp_rxtx, sp_rxtx_buffer_rx_get_null_buffer)
{
	size_t size = 0;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_rxtx_buffer_rx_get(NULL, &size));
}

TEST(sp_rxtx, sp_rxtx_buffer_rx_get_null_size)
{
	const void *buffer = NULL;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_rxtx_buffer_rx_get(&buffer, NULL));
}

TEST(sp_rxtx, sp_rxtx_buffer_rx_get_not_mapped)
{
	const void *buffer = NULL;
	size_t size = 0;

	LONGS_EQUAL(SP_RESULT_INVALID_STATE,
		    sp_rxtx_buffer_rx_get(&buffer, &size));
}

TEST(sp_rxtx, sp_rxtx_buffer_rx_get_success)
{
	const void *buffer = NULL;
	size_t size = 0;

	do_successful_mapping();
	LONGS_EQUAL(SP_RESULT_OK, sp_rxtx_buffer_rx_get(&buffer, &size));
	POINTERS_EQUAL(expected_rx_buffer, buffer);
	UNSIGNED_LONGS_EQUAL(expected_size, size);
}

TEST(sp_rxtx, sp_rxtx_buffer_tx_get_null_buffer)
{
	size_t size = 0;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_rxtx_buffer_tx_get(NULL, &size));
}

TEST(sp_rxtx, sp_rxtx_buffer_tx_get_null_size)
{
	void *buffer = NULL;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_rxtx_buffer_tx_get(&buffer, NULL));
}

TEST(sp_rxtx, sp_rxtx_buffer_tx_get_not_mapped)
{
	void *buffer = NULL;
	size_t size = 0;

	LONGS_EQUAL(SP_RESULT_INVALID_STATE,
		    sp_rxtx_buffer_tx_get(&buffer, &size));
}

TEST(sp_rxtx, sp_rxtx_buffer_tx_get_success)
{
	void *buffer = NULL;
	size_t size = 0;

	do_successful_mapping();
	LONGS_EQUAL(SP_RESULT_OK, sp_rxtx_buffer_tx_get(&buffer, &size));
	POINTERS_EQUAL(expected_tx_buffer, buffer);
	UNSIGNED_LONGS_EQUAL(expected_size, size);
}
