// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 */

#include <CppUTestExt/MockSupport.h>
#include <CppUTest/TestHarness.h>
#include "mock_sp_messaging.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const struct sp_msg expected_req = {
	.source_id = 0x0123,
	.destination_id = 0x4567,
	.is_64bit_message = false,
	.args = {0x89abcdef, 0xfedcba98, 0x76543210, 0xabcdef01}
};
static const struct sp_msg expected_resp = {
	.source_id = 0x1234,
	.destination_id = 0x5678,
	.is_64bit_message = false,
	.args = {0x9abcdef8, 0xedcba98f, 0x65432107, 0xbcdef01a}
};

TEST_GROUP(mock_sp_messaging)
{
	TEST_SETUP()
	{
		memset(&req, 0x00, sizeof(req));
		memset(&resp, 0x00, sizeof(resp));
	}

	TEST_TEARDOWN()
	{
		mock().checkExpectations();
		mock().clear();
	}

	struct sp_msg req;
	struct sp_msg resp;
	static const sp_result result = -1;
};

TEST(mock_sp_messaging, sp_msg_wait)
{
	expect_sp_msg_wait(&expected_req, result);
	LONGS_EQUAL(result, sp_msg_wait(&req));
	MEMCMP_EQUAL(&expected_req, &req, sizeof(expected_req));
}

TEST(mock_sp_messaging, sp_yield)
{
	expect_sp_yield(result);
	LONGS_EQUAL(result, sp_yield());
}

TEST(mock_sp_messaging, sp_msg_send_direct_req)
{
	req = expected_req;

	expect_sp_msg_send_direct_req(&expected_req, &expected_resp, result);
	LONGS_EQUAL(result, sp_msg_send_direct_req(&req, &resp));
	MEMCMP_EQUAL(&expected_resp, &resp, sizeof(expected_resp));
}

TEST(mock_sp_messaging, sp_msg_send_direct_resp)
{
	resp = expected_resp;

	expect_sp_msg_send_direct_resp(&expected_resp, &expected_req, result);
	LONGS_EQUAL(result, sp_msg_send_direct_resp(&resp, &req));
	MEMCMP_EQUAL(&expected_req, &req, sizeof(expected_req));
}

#if FFA_DIRECT_MSG_ROUTING_EXTENSION
TEST(mock_sp_messaging, sp_msg_send_rc_req)
{
	req = expected_req;

	expect_sp_msg_send_rc_req(&expected_req, &expected_resp, result);
	LONGS_EQUAL(result, sp_msg_send_rc_req(&req, &resp));
	MEMCMP_EQUAL(&expected_resp, &resp, sizeof(expected_resp));
}
#endif /* FFA_DIRECT_MSG_ROUTING_EXTENSION */
