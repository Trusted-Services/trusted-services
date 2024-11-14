// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2020-2023, Arm Limited. All rights reserved.
 */

#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <stdint.h>
#include <string.h>
#include "mock_assert.h"
#include "mock_ffa_api.h"
#include "mock_sp_rxtx.h"
#include "../include/sp_discovery.h"

static uint8_t rx_buf[128];
static const void *rx_buffer = rx_buf;
static size_t rx_buffer_size = sizeof(rx_buf);

TEST_GROUP(sp_discovery)
{
	TEST_SETUP()
	{
		memset(rx_buf, 0x00, sizeof(rx_buf));
	}

	TEST_TEARDOWN()
	{
		mock().checkExpectations();
		mock().clear();
	}

	const sp_result result = -1;
};

TEST(sp_discovery, sp_discovery_ffa_version_get_ffa_error)
{
	uint16_t major = 1;
	uint16_t minor = 2;
	const uint32_t expected_version = 0x010000;
	ffa_result result = FFA_ABORTED;

	expect_ffa_version(&expected_version, result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_discovery_ffa_version_get(&major, &minor));
	UNSIGNED_LONGS_EQUAL(0, major);
	UNSIGNED_LONGS_EQUAL(0, minor);
}

TEST(sp_discovery, sp_discovery_ffa_version)
{
	uint16_t major = 1;
	uint16_t minor = 2;
	const uint32_t expected_version = 0x789a1234U;

	expect_ffa_version(&expected_version, FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK, sp_discovery_ffa_version_get(&major, &minor));
	UNSIGNED_LONGS_EQUAL(expected_version >> 16, major);
	UNSIGNED_LONGS_EQUAL(expected_version & 0xffffU, minor);
}

TEST(sp_discovery, sp_discovery_own_id_get_error)
{
	uint16_t id = 0;
	const uint16_t expected_id = 123;
	ffa_result result = FFA_ABORTED;

	expect_ffa_id_get(&id, result);
	LONGS_EQUAL(SP_RESULT_FFA(result), sp_discovery_own_id_get(&id));
	UNSIGNED_LONGS_EQUAL(0, id);
}

TEST(sp_discovery, sp_discovery_partition_id_get_null_uuid)
{
	uint16_t id = 0;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_discovery_partition_id_get(NULL, &id));
	UNSIGNED_LONGS_EQUAL(0xffff, id);
}

TEST(sp_discovery, sp_discovery_partition_id_get_null_id)
{
	struct sp_uuid uuid = { 1 };

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_discovery_partition_id_get(&uuid, NULL));
}

TEST(sp_discovery, sp_discovery_partition_id_get_nil_uuid)
{
	struct sp_uuid uuid = { 0 };
	uint16_t id = 0;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_discovery_partition_id_get(&uuid, &id));
	UNSIGNED_LONGS_EQUAL(0xffff, id);
}

TEST(sp_discovery, sp_discovery_partition_id_rx_buffer_get_fail)
{
	struct sp_uuid uuid = { 1 };
	uint16_t id = 0;

	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &rx_buffer_size, result);
	LONGS_EQUAL(result, sp_discovery_partition_id_get(&uuid, &id));
	UNSIGNED_LONGS_EQUAL(0xffff, id);
}

TEST(sp_discovery, sp_discovery_partition_id_ffa_fail)
{
	struct sp_uuid uuid = { 1 };
	uint16_t id = 0;
	struct ffa_uuid ffa_uuid = { 1 };
	uint32_t count = 0;

	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &rx_buffer_size, SP_RESULT_OK);
	expect_ffa_partition_info_get(&ffa_uuid, &count, result);
	LONGS_EQUAL(result, sp_discovery_partition_id_get(&uuid, &id));
	UNSIGNED_LONGS_EQUAL(0xffff, id);
}

TEST(sp_discovery, sp_discovery_partition_id_small_buffer)
{
	struct sp_uuid uuid = { 1 };
	uint16_t id = 0;
	struct ffa_uuid ffa_uuid = { 1 };
	uint32_t count =
		(rx_buffer_size / sizeof(struct ffa_partition_information)) + 1;

	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &rx_buffer_size, SP_RESULT_OK);
	expect_ffa_partition_info_get(&ffa_uuid, &count, SP_RESULT_OK);
	LONGS_EQUAL(SP_RESULT_INTERNAL_ERROR,
		    sp_discovery_partition_id_get(&uuid, &id));
	UNSIGNED_LONGS_EQUAL(0xffff, id);
}

TEST(sp_discovery, sp_discovery_partition_id_zero_count)
{
	struct sp_uuid uuid = { 1 };
	uint16_t id = 0;
	struct ffa_uuid ffa_uuid = { 1 };
	uint32_t count = 0;

	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &rx_buffer_size, SP_RESULT_OK);
	expect_ffa_partition_info_get(&ffa_uuid, &count, SP_RESULT_OK);
	LONGS_EQUAL(SP_RESULT_NOT_FOUND,
		    sp_discovery_partition_id_get(&uuid, &id));
	UNSIGNED_LONGS_EQUAL(0xffff, id);
}

TEST(sp_discovery, sp_discovery_partition_id)
{
	struct sp_uuid uuid = { 1 };
	uint16_t id = 0;
	struct ffa_uuid ffa_uuid = { 1 };
	uint32_t count =
		(rx_buffer_size / sizeof(struct ffa_partition_information));
	const uint16_t expected_id = 1234;

	((struct ffa_partition_information *)rx_buffer)->partition_id =
		expected_id;
	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &rx_buffer_size, SP_RESULT_OK);
	expect_ffa_partition_info_get(&ffa_uuid, &count, SP_RESULT_OK);
	LONGS_EQUAL(SP_RESULT_OK, sp_discovery_partition_id_get(&uuid, &id));
	UNSIGNED_LONGS_EQUAL(expected_id, id);
}

TEST(sp_discovery, sp_discovery_partition_info_get_null_uuid)
{
	struct sp_partition_info info = { 1 };
	const struct sp_partition_info expected_info = { 0 };
	uint32_t count = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_discovery_partition_info_get(NULL, &info, &count));
	MEMCMP_EQUAL(&expected_info, &info, sizeof(info));
}

TEST(sp_discovery, sp_discovery_partition_info_get_null_info)
{
	struct sp_uuid uuid = { 1 };
	uint32_t count = 0;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_discovery_partition_info_get(&uuid, NULL, &count));
}

TEST(sp_discovery, sp_discovery_partition_info_get_nil_uuid)
{
	struct sp_uuid uuid = { 0 };
	struct sp_partition_info info = { 1 };
	const struct sp_partition_info expected_info = { 0 };
	uint32_t count = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_discovery_partition_info_get(&uuid, &info, &count));
	MEMCMP_EQUAL(&expected_info, &info, sizeof(info));
}

TEST(sp_discovery, sp_discovery_partition_info_rx_buffer_get_fail)
{
	struct sp_uuid uuid = { 1 };
	struct sp_partition_info info = { 1 };
	const struct sp_partition_info expected_info = { 0 };
	uint32_t count = 1;

	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &rx_buffer_size, result);
	LONGS_EQUAL(result, sp_discovery_partition_info_get(&uuid, &info,
							    &count));
	MEMCMP_EQUAL(&expected_info, &info, sizeof(info));
}

TEST(sp_discovery, sp_discovery_partition_info_ffa_fail)
{
	struct sp_uuid uuid = { 1 };
	struct ffa_uuid ffa_uuid = { 1 };
	uint32_t count = 1;
	struct sp_partition_info info = { 1 };
	const struct sp_partition_info expected_info = { 0 };

	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &rx_buffer_size, SP_RESULT_OK);
	expect_ffa_partition_info_get(&ffa_uuid, &count, result);
	LONGS_EQUAL(result, sp_discovery_partition_info_get(&uuid, &info,
							    &count));
	MEMCMP_EQUAL(&expected_info, &info, sizeof(info));
}

#if CFG_FFA_VERSION >= FFA_VERSION_1_1
TEST(sp_discovery, sp_discovery_partition_info_invalid_desc_size)
{
	struct sp_uuid uuid = { 1 };
	struct ffa_uuid ffa_uuid = { 1 };
	uint32_t count = 1;
	uint32_t size = 0;
	struct sp_partition_info info = { 1 };
	const struct sp_partition_info expected_info = { 0 };

	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &rx_buffer_size, SP_RESULT_OK);
	expect_ffa_partition_info_get(&ffa_uuid, 0, &count, &size, FFA_OK);
	LONGS_EQUAL(SP_RESULT_INTERNAL_ERROR,
		    sp_discovery_partition_info_get(&uuid, &info, &count));
	MEMCMP_EQUAL(&expected_info, &info, sizeof(info));
}
#endif

TEST(sp_discovery, sp_discovery_partition_info_small_buffer)
{
	struct sp_uuid uuid = { 1 };
	struct ffa_uuid ffa_uuid = { 1 };
	uint32_t expected_count =
		(rx_buffer_size / sizeof(struct ffa_partition_information)) + 1;
	uint32_t count = 1;
	struct sp_partition_info info = { 1 };
	const struct sp_partition_info expected_info = { 0 };

	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &rx_buffer_size, SP_RESULT_OK);
	expect_ffa_partition_info_get(&ffa_uuid, &expected_count, SP_RESULT_OK);
	LONGS_EQUAL(SP_RESULT_INTERNAL_ERROR,
		    sp_discovery_partition_info_get(&uuid, &info, &count));
	MEMCMP_EQUAL(&expected_info, &info, sizeof(info));
}

TEST(sp_discovery, sp_discovery_partition_info_zero_count)
{
	struct sp_uuid uuid = { 1 };
	struct ffa_uuid ffa_uuid = { 1 };
	uint32_t expected_count = 0;
	uint32_t count = 1;
	struct sp_partition_info info = { 1 };
	const struct sp_partition_info expected_info = { 0 };

	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &rx_buffer_size, SP_RESULT_OK);
	expect_ffa_partition_info_get(&ffa_uuid, &expected_count, SP_RESULT_OK);
	LONGS_EQUAL(SP_RESULT_NOT_FOUND,
		    sp_discovery_partition_info_get(&uuid, &info, &count));
	MEMCMP_EQUAL(&expected_info, &info, sizeof(info));
	UNSIGNED_LONGS_EQUAL(expected_count, count);
}

TEST(sp_discovery, sp_discovery_partition_info)
{
	struct sp_uuid uuid = { 1 };
	struct ffa_uuid ffa_uuid = { 1 };
	uint32_t count =
		(rx_buffer_size / sizeof(struct ffa_partition_information));
	const uint16_t expected_id = 1234;
	const uint16_t expected_context_count = 23456;
	struct sp_partition_info *info =
		(struct sp_partition_info *)calloc(count, sizeof(struct sp_partition_info));

	for (uint32_t i = 0; i < count; i++) {
		((struct ffa_partition_information *)rx_buffer)[i].partition_id = expected_id + i;
		((struct ffa_partition_information *)rx_buffer)[i].execution_context_count = expected_context_count + i;
		((struct ffa_partition_information *)rx_buffer)[i].partition_properties = 0;
	}
	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &rx_buffer_size, SP_RESULT_OK);
	expect_ffa_partition_info_get(&ffa_uuid, &count, SP_RESULT_OK);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_discovery_partition_info_get(&uuid, info, &count));

	for (uint32_t i = 0; i < count; i++) {
		UNSIGNED_LONGS_EQUAL(expected_id + i, info[i].partition_id);
		UNSIGNED_LONGS_EQUAL(expected_context_count + i,
				info[i].execution_context_count);
		CHECK_FALSE(info[i].supports_direct_requests);
		CHECK_FALSE(info[i].can_send_direct_requests);
		CHECK_FALSE(info[i].supports_indirect_requests);
	}

	free(info);
}

TEST(sp_discovery, sp_discovery_partition_info_support_direct_req)
{
	struct sp_uuid uuid = { 1 };
	struct ffa_uuid ffa_uuid = { 1 };
	uint32_t count = 1;
	struct sp_partition_info info = { 0 };

	((struct ffa_partition_information *)rx_buffer)->partition_properties =
		FFA_PARTITION_SUPPORTS_DIRECT_REQUESTS;
	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &rx_buffer_size, SP_RESULT_OK);
	expect_ffa_partition_info_get(&ffa_uuid, &count, SP_RESULT_OK);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_discovery_partition_info_get(&uuid, &info, &count));
	CHECK_TRUE(info.supports_direct_requests);
	CHECK_FALSE(info.can_send_direct_requests);
	CHECK_FALSE(info.supports_indirect_requests);
}

TEST(sp_discovery, sp_discovery_partition_info_can_send_direct_req)
{
	struct sp_uuid uuid = { 1 };
	struct ffa_uuid ffa_uuid = { 1 };
	uint32_t count = 1;
	struct sp_partition_info info = { 0 };

	((struct ffa_partition_information *)rx_buffer)->partition_properties =
		FFA_PARTITION_CAN_SEND_DIRECT_REQUESTS;
	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &rx_buffer_size, SP_RESULT_OK);
	expect_ffa_partition_info_get(&ffa_uuid, &count, SP_RESULT_OK);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_discovery_partition_info_get(&uuid, &info, &count));
	CHECK_FALSE(info.supports_direct_requests);
	CHECK_TRUE(info.can_send_direct_requests);
	CHECK_FALSE(info.supports_indirect_requests);
}

TEST(sp_discovery, sp_discovery_partition_info_support_indirect_req)
{
	struct sp_uuid uuid = { 1 };
	struct ffa_uuid ffa_uuid = { 1 };
	uint32_t count = 1;
	struct sp_partition_info info = { 0 };

	((struct ffa_partition_information *)rx_buffer)->partition_properties =
		FFA_PARTITION_SUPPORTS_INDIRECT_REQUESTS;
	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &rx_buffer_size, SP_RESULT_OK);
	expect_ffa_partition_info_get(&ffa_uuid, &count, SP_RESULT_OK);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_discovery_partition_info_get(&uuid, &info, &count));
	CHECK_FALSE(info.supports_direct_requests);
	CHECK_FALSE(info.can_send_direct_requests);
	CHECK_TRUE(info.supports_indirect_requests);
}

TEST(sp_discovery, sp_discovery_partition_info_get_all_null_info)
{
	uint32_t count = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_discovery_partition_info_get_all(NULL, &count));
	UNSIGNED_LONGS_EQUAL(0, count);
}

TEST(sp_discovery, sp_discovery_partition_info_get_all_null_count)
{
	struct sp_partition_info info;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_discovery_partition_info_get_all(&info, NULL));
}

TEST(sp_discovery, sp_discovery_partition_info_get_all_rx_buffer_get_fail)
{
	struct sp_partition_info info;
	uint32_t count = 1;

	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &rx_buffer_size, result);
	LONGS_EQUAL(result, sp_discovery_partition_info_get_all(&info, &count));
	UNSIGNED_LONGS_EQUAL(0, count);
}

TEST(sp_discovery, sp_discovery_partition_info_get_all_ffa_fail)
{
	struct ffa_uuid ffa_uuid = { 0 };
	struct sp_partition_info info;
	uint32_t count = 2;
	const uint32_t expected_count = 1;

	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &rx_buffer_size, SP_RESULT_OK);
	expect_ffa_partition_info_get(&ffa_uuid, &expected_count, result);
	LONGS_EQUAL(result, sp_discovery_partition_info_get_all(&info, &count));
	UNSIGNED_LONGS_EQUAL(0, count);
}

TEST(sp_discovery, sp_discovery_partition_info_get_all_small_buffer)
{
	struct ffa_uuid ffa_uuid = { 0 };
	struct sp_partition_info info;
	uint32_t count = 2;
	const uint32_t expected_count =
		(rx_buffer_size / sizeof(struct ffa_partition_information)) + 1;

	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &rx_buffer_size, SP_RESULT_OK);
	expect_ffa_partition_info_get(&ffa_uuid, &expected_count, SP_RESULT_OK);
	LONGS_EQUAL(SP_RESULT_INTERNAL_ERROR,
		    sp_discovery_partition_info_get_all(&info, &count));
	UNSIGNED_LONGS_EQUAL(0, count);
}

TEST(sp_discovery, sp_discovery_partition_info_get_all_zero_count)
{
	struct ffa_uuid ffa_uuid = { 0 };
	struct sp_partition_info info;
	uint32_t count = 2;
	const uint32_t expected_count = 0;

	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &rx_buffer_size, SP_RESULT_OK);
	expect_ffa_partition_info_get(&ffa_uuid, &expected_count, SP_RESULT_OK);
	LONGS_EQUAL(SP_RESULT_NOT_FOUND,
		    sp_discovery_partition_info_get_all(&info, &count));
	UNSIGNED_LONGS_EQUAL(0, count);
}

TEST(sp_discovery, sp_discovery_partition_info_get_all_one)
{
	struct ffa_uuid ffa_uuid = { 0 };
	struct sp_partition_info info;
	uint32_t count = 2;
	const uint32_t expected_count = 1;
	const uint16_t expected_id = 1234;
	const uint16_t expected_context_count = 23456;

	((struct ffa_partition_information *)rx_buffer)->partition_id =
		expected_id;
	((struct ffa_partition_information *)rx_buffer)
		->execution_context_count = expected_context_count;
	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &rx_buffer_size, SP_RESULT_OK);
	expect_ffa_partition_info_get(&ffa_uuid, &expected_count, SP_RESULT_OK);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_discovery_partition_info_get_all(&info, &count));
	UNSIGNED_LONGS_EQUAL(expected_count, count);

	UNSIGNED_LONGS_EQUAL(expected_id, info.partition_id);
	UNSIGNED_LONGS_EQUAL(expected_context_count,
			     info.execution_context_count);
	CHECK_FALSE(info.supports_direct_requests);
	CHECK_FALSE(info.can_send_direct_requests);
	CHECK_FALSE(info.supports_indirect_requests);
}

TEST(sp_discovery, sp_discovery_partition_info_get_all_two)
{
	struct ffa_uuid ffa_uuid = { 0 };
	struct sp_partition_info info[2];
	uint32_t count = 2;
	const uint32_t expected_count = 2;
	const uint16_t expected_id0 = 1234;
	const uint16_t expected_context_count0 = 23456;
	const uint16_t expected_id1 = 7234;
	const uint16_t expected_context_count1 = 43456;

	((struct ffa_partition_information *)rx_buffer)[0].partition_id =
		expected_id0;
	((struct ffa_partition_information *)rx_buffer)[0]
		.execution_context_count = expected_context_count0;
	((struct ffa_partition_information *)rx_buffer)[1].partition_id =
		expected_id1;
	((struct ffa_partition_information *)rx_buffer)[1]
		.execution_context_count = expected_context_count1;
	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &rx_buffer_size, SP_RESULT_OK);
	expect_ffa_partition_info_get(&ffa_uuid, &expected_count, SP_RESULT_OK);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_discovery_partition_info_get_all(info, &count));
	UNSIGNED_LONGS_EQUAL(expected_count, count);

	UNSIGNED_LONGS_EQUAL(expected_id0, info[0].partition_id);
	UNSIGNED_LONGS_EQUAL(expected_context_count0,
			     info[0].execution_context_count);
	CHECK_FALSE(info[0].supports_direct_requests);
	CHECK_FALSE(info[0].can_send_direct_requests);
	CHECK_FALSE(info[0].supports_indirect_requests);

	UNSIGNED_LONGS_EQUAL(expected_id1, info[1].partition_id);
	UNSIGNED_LONGS_EQUAL(expected_context_count1,
			     info[1].execution_context_count);
	CHECK_FALSE(info[1].supports_direct_requests);
	CHECK_FALSE(info[1].can_send_direct_requests);
	CHECK_FALSE(info[1].supports_indirect_requests);
}

TEST(sp_discovery, sp_discovery_partition_info_get_all_two_small_buffer)
{
	struct ffa_uuid ffa_uuid = { 0 };
	struct sp_partition_info info[2] = { 0 };
	uint32_t count = 1;
	const uint32_t expected_count = 2;
	const uint16_t expected_id0 = 1234;
	const uint16_t expected_context_count0 = 23456;
	const uint16_t expected_id1 = 7234;
	const uint16_t expected_context_count1 = 43456;

	((struct ffa_partition_information *)rx_buffer)[0].partition_id =
		expected_id0;
	((struct ffa_partition_information *)rx_buffer)[0]
		.execution_context_count = expected_context_count0;
	((struct ffa_partition_information *)rx_buffer)[1].partition_id =
		expected_id1;
	((struct ffa_partition_information *)rx_buffer)[1]
		.execution_context_count = expected_context_count1;
	expect_sp_rxtx_buffer_rx_get(&rx_buffer, &rx_buffer_size, SP_RESULT_OK);
	expect_ffa_partition_info_get(&ffa_uuid, &expected_count, SP_RESULT_OK);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_discovery_partition_info_get_all(info, &count));
	UNSIGNED_LONGS_EQUAL(1, count);

	UNSIGNED_LONGS_EQUAL(expected_id0, info[0].partition_id);
	UNSIGNED_LONGS_EQUAL(expected_context_count0,
			     info[0].execution_context_count);
	CHECK_FALSE(info[0].supports_direct_requests);
	CHECK_FALSE(info[0].can_send_direct_requests);
	CHECK_FALSE(info[0].supports_indirect_requests);

	UNSIGNED_LONGS_EQUAL(0, info[1].partition_id);
	UNSIGNED_LONGS_EQUAL(0, info[1].execution_context_count);
	CHECK_FALSE(info[1].supports_direct_requests);
	CHECK_FALSE(info[1].can_send_direct_requests);
	CHECK_FALSE(info[1].supports_indirect_requests);
}

#if CFG_FFA_VERSION >= FFA_VERSION_1_1
TEST(sp_discovery, sp_discovery_partition_info_get_count_null)
{
	struct sp_uuid uuid = { 1 };

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_discovery_partition_info_get_count(&uuid, NULL));
}

TEST(sp_discovery, sp_discovery_partition_info_get_count_uuid_null)
{
	uint32_t count = 1;

	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_discovery_partition_info_get_count(NULL, &count));
	UNSIGNED_LONGS_EQUAL(0, count);
}

TEST(sp_discovery, sp_discovery_partition_info_get_count_ffa_error)
{
	struct ffa_uuid ffa_uuid = { 1 };
	struct sp_uuid sp_uuid = { 1 };
	const uint32_t expected_count = 1;
	const uint32_t expected_size = 0;
	uint32_t count = 0;

	expect_ffa_partition_info_get(&ffa_uuid, 0x01, &expected_count, &expected_size,
					   result);
	LONGS_EQUAL(result,
		    sp_discovery_partition_info_get_count(&sp_uuid, &count));
}

TEST(sp_discovery, sp_discovery_partition_info_get_count_invalid_size)
{
	struct ffa_uuid ffa_uuid = { 1 };
	struct sp_uuid sp_uuid = { 1 };
	const uint32_t expected_count = 1;
	const uint32_t expected_size = 11;
	uint32_t count = 0;

	expect_ffa_partition_info_get(&ffa_uuid, 0x01, &expected_count, &expected_size,
					   FFA_OK);
	LONGS_EQUAL(SP_RESULT_INTERNAL_ERROR,
		    sp_discovery_partition_info_get_count(&sp_uuid, &count));
}

TEST(sp_discovery, sp_discovery_partition_info_get_count)
{
	struct ffa_uuid ffa_uuid = { 1 };
	struct sp_uuid sp_uuid = { 1 };
	const uint32_t expected_count = 1;
	const uint32_t expected_size = 0;
	uint32_t count = 0;

	expect_ffa_partition_info_get(&ffa_uuid, 0x01, &expected_count, &expected_size,
					   FFA_OK);
	LONGS_EQUAL(SP_RESULT_OK,
		    sp_discovery_partition_info_get_count(&sp_uuid, &count));
	UNSIGNED_LONGS_EQUAL(expected_count, count);
}
#endif /* CFG_FFA_VERSION */
