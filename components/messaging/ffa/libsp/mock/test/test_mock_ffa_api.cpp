// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2020-2022, Arm Limited. All rights reserved.
 */

#include <assert.h>
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include "mock_ffa_api.h"

const struct ffa_direct_msg expected_msg = { 0xffeeddcc, 0xaabb,     0x8899,
					     0x12345678, 0x23456789, 0x3456789a,
					     0x456789ab, 0x56789abc };

TEST_GROUP(mock_ffa_api)
{
	TEST_TEARDOWN()
	{
		mock().checkExpectations();
		mock().clear();
	}

	static const ffa_result result = -1;
};

TEST(mock_ffa_api, ffa_version)
{
	const uint32_t expected_version = 0x12345678U;
	uint32_t version = 0;

	expect_ffa_version(&expected_version, result);
	LONGS_EQUAL(result, ffa_version(&version));
	UNSIGNED_LONGS_EQUAL(expected_version, version);
}

TEST(mock_ffa_api, ffa_features)
{
	const uint32_t ffa_function_id = 0xfedcba98U;
	const struct ffa_interface_properties expected_interface_properties = {
		0xaabbccdd, 0x44556677
	};
	struct ffa_interface_properties interface_properties = { 0 };

	expect_ffa_features(ffa_function_id, &expected_interface_properties,
			    result);
	LONGS_EQUAL(result,
		    ffa_features(ffa_function_id, &interface_properties));
	MEMCMP_EQUAL(&expected_interface_properties, &interface_properties,
		     sizeof(expected_interface_properties));
}

TEST(mock_ffa_api, ffa_rx_release)
{
	expect_ffa_rx_release(result);
	LONGS_EQUAL(result, ffa_rx_release());
}

TEST(mock_ffa_api, ffa_rxtx_map)
{
	const char tx_buffer = 0;
	const char rx_buffer = 0;
	const uint32_t page_count = 0x89abcdefU;

	expect_ffa_rxtx_map(&tx_buffer, &rx_buffer, page_count, result);
	LONGS_EQUAL(result, ffa_rxtx_map(&tx_buffer, &rx_buffer, page_count));
}

TEST(mock_ffa_api, ffa_rxtx_unmap)
{
	const uint16_t id = 0xffee;

	expect_ffa_rxtx_unmap(id, result);
	LONGS_EQUAL(result, ffa_rxtx_unmap(id));
}

#if CFG_FFA_VERSION == FFA_VERSION_1_0
TEST(mock_ffa_api, ffa_partition_info_get_v1_0)
{
	const struct ffa_uuid uuid = { 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54,
				       0x32, 0x10, 0x01, 0x23, 0x45, 0x67,
				       0x89, 0xab, 0xcd, 0xef };
	const uint32_t expected_count = 0xff00ee11U;
	uint32_t count = 0;

	expect_ffa_partition_info_get(&uuid, &expected_count, result);
	LONGS_EQUAL(result, ffa_partition_info_get(&uuid, &count));
	UNSIGNED_LONGS_EQUAL(expected_count, count);
}
#elif CFG_FFA_VERSION >= FFA_VERSION_1_1
TEST(mock_ffa_api, ffa_partition_info_get_v1_1)
{
	const struct ffa_uuid uuid = { 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54,
				       0x32, 0x10, 0x01, 0x23, 0x45, 0x67,
				       0x89, 0xab, 0xcd, 0xef };
	const uint32_t flags = 0xabcdef01;
	const uint32_t expected_count = 0xff00ee11U;
	const uint32_t expected_size = 0xff00ee11U;
	uint32_t count = 0;
	uint32_t size = 0;

	expect_ffa_partition_info_get(&uuid, flags, &expected_count, &expected_size, result);
	LONGS_EQUAL(result, ffa_partition_info_get(&uuid, flags, &count, &size));
	UNSIGNED_LONGS_EQUAL(expected_count, count);
	UNSIGNED_LONGS_EQUAL(expected_size, size);
}
#endif /* CFG_FFA_VERSION */

TEST(mock_ffa_api, ffa_id_get)
{
	const uint16_t expected_id = 0xabcd;
	uint16_t id = 0;

	expect_ffa_id_get(&expected_id, result);
	LONGS_EQUAL(result, ffa_id_get(&id));
	UNSIGNED_LONGS_EQUAL(expected_id, id);
}

TEST(mock_ffa_api, ffa_msg_wait)
{
	struct ffa_direct_msg msg = { 0 };

	expect_ffa_msg_wait(&expected_msg, result);
	LONGS_EQUAL(result, ffa_msg_wait(&msg));
	MEMCMP_EQUAL(&expected_msg, &msg, sizeof(expected_msg));
}

TEST(mock_ffa_api, ffa_msg_send_direct_req_32)
{
	const uint16_t source = 0x1122;
	const uint16_t dest = 0x2233;
	const uint32_t a0 = 0x45678912;
	const uint32_t a1 = 0x56789124;
	const uint32_t a2 = 0x67891245;
	const uint32_t a3 = 0x78912456;
	const uint32_t a4 = 0x89124567;
	struct ffa_direct_msg msg = { 0 };

	expect_ffa_msg_send_direct_req_32(source, dest, a0, a1, a2, a3, a4,
				          &expected_msg, result);
	LONGS_EQUAL(result, ffa_msg_send_direct_req_32(source, dest, a0, a1, a2,
						       a3, a4, &msg));
}

TEST(mock_ffa_api, ffa_msg_send_direct_req_64)
{
	const uint16_t source = 0x1122;
	const uint16_t dest = 0x2233;
	const uint64_t a0 = 0x4567891221987654;
	const uint64_t a1 = 0x5678912442198765;
	const uint64_t a2 = 0x6789124554219876;
	const uint64_t a3 = 0x7891245665421987;
	const uint64_t a4 = 0x8912456776542198;
	struct ffa_direct_msg msg = { 0 };

	expect_ffa_msg_send_direct_req_64(source, dest, a0, a1, a2, a3, a4,
				       &expected_msg, result);
	LONGS_EQUAL(result, ffa_msg_send_direct_req_64(source, dest, a0, a1, a2,
						       a3, a4, &msg));
}

TEST(mock_ffa_api, ffa_msg_send_direct_resp_32)
{
	const uint16_t source = 0x1122;
	const uint16_t dest = 0x2233;
	const uint32_t a0 = 0x45678912;
	const uint32_t a1 = 0x56789124;
	const uint32_t a2 = 0x67891245;
	const uint32_t a3 = 0x78912456;
	const uint32_t a4 = 0x89124567;
	struct ffa_direct_msg msg = { 0 };

	expect_ffa_msg_send_direct_resp_32(source, dest, a0, a1, a2, a3, a4,
					&expected_msg, result);
	LONGS_EQUAL(result, ffa_msg_send_direct_resp_32(source, dest, a0, a1,
							a2, a3, a4, &msg));
}

TEST(mock_ffa_api, ffa_msg_send_direct_resp_64)
{
	const uint16_t source = 0x1122;
	const uint16_t dest = 0x2233;
	const uint64_t a0 = 0x4567891221987654;
	const uint64_t a1 = 0x5678912442198765;
	const uint64_t a2 = 0x6789124554219876;
	const uint64_t a3 = 0x7891245665421987;
	const uint64_t a4 = 0x8912456776542198;
	struct ffa_direct_msg msg = { 0 };

	expect_ffa_msg_send_direct_resp_64(source, dest, a0, a1, a2, a3, a4,
					  &expected_msg, result);
	LONGS_EQUAL(result, ffa_msg_send_direct_resp_64(source, dest, a0, a1,
							a2, a3, a4, &msg));
}

TEST(mock_ffa_api, ffa_mem_donate)
{
	const uint32_t total_length = 0x12345678U;
	const uint32_t fragment_length = 0x23456789U;
	const char buffer = 0;
	const uint32_t page_count = 0x3456789au;
	const uint64_t expected_handle = 0xfedcba9876543210ULL;
	uint64_t handle = 0;

	expect_ffa_mem_donate(total_length, fragment_length, (void *)&buffer,
			      page_count, &expected_handle, result);
	LONGS_EQUAL(result,
		    ffa_mem_donate(total_length, fragment_length,
				   (void *)&buffer, page_count, &handle));
	UNSIGNED_LONGLONGS_EQUAL(expected_handle, handle);
}

TEST(mock_ffa_api, ffa_mem_donate_rxtx)
{
	const uint32_t total_length = 0x12345678U;
	const uint32_t fragment_length = 0x23456789U;
	const uint64_t expected_handle = 0xfedcba9876543210ULL;
	uint64_t handle = 0;

	expect_ffa_mem_donate_rxtx(total_length, fragment_length,
				   &expected_handle, result);
	LONGS_EQUAL(result, ffa_mem_donate_rxtx(total_length, fragment_length,
						&handle));
	UNSIGNED_LONGLONGS_EQUAL(expected_handle, handle);
}

TEST(mock_ffa_api, ffa_mem_lend)
{
	const uint32_t total_length = 0x12345678U;
	const uint32_t fragment_length = 0x23456789U;
	const char buffer = 0;
	const uint32_t page_count = 0x3456789au;
	const uint64_t expected_handle = 0xfedcba9876543210ULL;
	uint64_t handle = 0;

	expect_ffa_mem_lend(total_length, fragment_length, (void *)&buffer,
			    page_count, &expected_handle, result);
	LONGS_EQUAL(result, ffa_mem_lend(total_length, fragment_length,
					 (void *)&buffer, page_count, &handle));
	UNSIGNED_LONGLONGS_EQUAL(expected_handle, handle);
}

TEST(mock_ffa_api, ffa_mem_lend_rxtx)
{
	const uint32_t total_length = 0x12345678U;
	const uint32_t fragment_length = 0x23456789U;
	const uint64_t expected_handle = 0xfedcba9876543210ULL;
	uint64_t handle = 0;

	expect_ffa_mem_lend_rxtx(total_length, fragment_length,
				 &expected_handle, result);
	LONGS_EQUAL(result,
		    ffa_mem_lend_rxtx(total_length, fragment_length, &handle));
	UNSIGNED_LONGLONGS_EQUAL(expected_handle, handle);
}

TEST(mock_ffa_api, ffa_mem_share)
{
	const uint32_t total_length = 0x12345678U;
	const uint32_t fragment_length = 0x23456789U;
	const char buffer = 0;
	const uint32_t page_count = 0x3456789au;
	const uint64_t expected_handle = 0xfedcba9876543210ULL;
	uint64_t handle = 0;

	expect_ffa_mem_share(total_length, fragment_length, (void *)&buffer,
			     page_count, &expected_handle, result);
	LONGS_EQUAL(result,
		    ffa_mem_share(total_length, fragment_length,
				  (void *)&buffer, page_count, &handle));
	UNSIGNED_LONGLONGS_EQUAL(expected_handle, handle);
}

TEST(mock_ffa_api, ffa_mem_share_rxtx)
{
	const uint32_t total_length = 0x12345678U;
	const uint32_t fragment_length = 0x23456789U;
	const uint64_t expected_handle = 0xfedcba9876543210ULL;
	uint64_t handle = 0;

	expect_ffa_mem_share_rxtx(total_length, fragment_length,
				  &expected_handle, result);
	LONGS_EQUAL(result,
		    ffa_mem_share_rxtx(total_length, fragment_length, &handle));
	UNSIGNED_LONGLONGS_EQUAL(expected_handle, handle);
}

TEST(mock_ffa_api, ffa_mem_retrieve_req)
{
	const uint32_t total_length = 0x12345678U;
	const uint32_t fragment_length = 0x23456789U;
	const char buffer = 0;
	const uint32_t page_count = 0x3456789aU;
	const uint32_t expected_resp_total_length = 0xfedcba98U;
	const uint32_t expected_resp_fragment_length = 0xedcba987U;
	uint32_t resp_total_length = 0;
	uint32_t resp_fragment_length = 0;

	expect_ffa_mem_retrieve_req(total_length, fragment_length,
				    (void *)&buffer, page_count,
				    &expected_resp_total_length,
				    &expected_resp_fragment_length, result);
	LONGS_EQUAL(result, ffa_mem_retrieve_req(total_length, fragment_length,
						 (void *)&buffer, page_count,
						 &resp_total_length,
						 &resp_fragment_length));
	UNSIGNED_LONGS_EQUAL(expected_resp_total_length, resp_total_length);
	UNSIGNED_LONGS_EQUAL(expected_resp_fragment_length,
			     resp_fragment_length);
}

TEST(mock_ffa_api, ffa_mem_retrieve_req_rxtx)
{
	const uint32_t total_length = 0x12345678U;
	const uint32_t fragment_length = 0x23456789U;
	const uint32_t expected_resp_total_length = 0xfedcba98U;
	const uint32_t expected_resp_fragment_length = 0xedcba987U;
	uint32_t resp_total_length = 0;
	uint32_t resp_fragment_length = 0;

	expect_ffa_mem_retrieve_req_rxtx(total_length, fragment_length,
					 &expected_resp_total_length,
					 &expected_resp_fragment_length,
					 result);
	LONGS_EQUAL(result, ffa_mem_retrieve_req_rxtx(
				    total_length, fragment_length,
				    &resp_total_length, &resp_fragment_length));
	UNSIGNED_LONGS_EQUAL(expected_resp_total_length, resp_total_length);
	UNSIGNED_LONGS_EQUAL(expected_resp_fragment_length,
			     resp_fragment_length);
}

TEST(mock_ffa_api, ffa_mem_relinquish)
{
	expect_ffa_mem_relinquish(result);
	LONGS_EQUAL(result, ffa_mem_relinquish());
}

TEST(mock_ffa_api, ffa_mem_reclaim)
{
	const uint64_t handle = 0xfedcba9876543210ULL;
	const uint32_t flags = 0xaaccbbddU;

	expect_ffa_mem_reclaim(handle, flags, result);
	LONGS_EQUAL(result, ffa_mem_reclaim(handle, flags));
}

TEST(mock_ffa_api, ffa_mem_perm_get)
{
	const void *base_address = (const void *)0x01234567;
	uint32_t expected_mem_perm = 0x89abcdef;
	uint32_t mem_perm = 0;

	expect_ffa_mem_perm_get(base_address, &expected_mem_perm, result);
	LONGS_EQUAL(result, ffa_mem_perm_get(base_address, &mem_perm));
	UNSIGNED_LONGS_EQUAL(expected_mem_perm, mem_perm);
}

TEST(mock_ffa_api, ffa_mem_perm_set)
{
	const void *base_address = (const void *)0x01234567;
	const uint32_t page_count = 0x76543210;
	const uint32_t mem_perm = 0x89abcdef;

	expect_ffa_mem_perm_set(base_address, page_count, mem_perm, result);
	LONGS_EQUAL(result, ffa_mem_perm_set(base_address, page_count, mem_perm));
}

TEST(mock_ffa_api, ffa_console_log_32)
{
	const char *message = "log message";
	const size_t length = 11;

	expect_ffa_console_log_32(message, length, result);
	LONGS_EQUAL(result, ffa_console_log_32(message, length));
}

TEST(mock_ffa_api, ffa_console_log_64)
{
	const char *message = "log message";
	const size_t length = 11;

	expect_ffa_console_log_64(message, length, result);
	LONGS_EQUAL(result, ffa_console_log_64(message, length));
}
