/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <CppUTest/TestHarness.h>
#include <mock_assert.h>
#include <protocols/rpc/common/packed-c/encoding.h>
#include <service_locator.h>
#include <string.h>

struct service_context *mock_query(const char *sn, int *status)
{
	return NULL;
}

rpc_caller_session *mock_service_context_open(void *contex)
{
	return NULL;
}

TEST_GROUP(ServiceLocatorTests){

};

TEST(ServiceLocatorTests, locatorQueryInvalidServiceName)
{
	struct service_context *ctx = service_locator_query("sn:trustedfirmware.org");

	POINTERS_EQUAL(NULL, ctx);
}

TEST(ServiceLocatorTests, locatorQueryNoResultForValidServiceName)
{
	struct service_context *ctx =
		service_locator_query("sn:trustedfirmware.org:secure-storage.1.0:0");

	POINTERS_EQUAL(NULL, ctx);
}

TEST(ServiceLocatorTests, serviceContextOpenFails)
{
	struct service_context ctx {
		.open = mock_service_context_open
	};

	struct rpc_caller_session *session = service_context_open(&ctx);

	POINTERS_EQUAL(NULL, session);
}
