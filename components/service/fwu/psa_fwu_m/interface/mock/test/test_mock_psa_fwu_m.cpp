/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "../mock_psa_fwu_m.h"
#include <string.h>
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(mock_psa_fwu_m) {
	TEST_TEARDOWN() {
		mock().checkExpectations();
		mock().clear();
	}

	const psa_fwu_component_t component = 0x5a;
	const psa_status_t result = PSA_ERROR_GENERIC_ERROR;
};

TEST(mock_psa_fwu_m, query)
{
	psa_fwu_component_info_t expected_info = { 0 };
	psa_fwu_component_info_t info = { 0 };

	memset(&expected_info, 0x5a, sizeof(expected_info));

	expect_mock_psa_fwu_query(component, &expected_info, result);
	LONGS_EQUAL(result, psa_fwu_query(component, &info));
	MEMCMP_EQUAL(&info, &expected_info, sizeof(info));
}

TEST(mock_psa_fwu_m, start)
{
	uint8_t manifest[8];

	memset(manifest, 0x5a, sizeof(manifest));

	expect_mock_psa_fwu_start(component, manifest, sizeof(manifest), result);
	LONGS_EQUAL(result, psa_fwu_start(component, manifest, sizeof(manifest)));
}

TEST(mock_psa_fwu_m, start_null_manifest)
{
	expect_mock_psa_fwu_start(component, NULL, 0, result);
	LONGS_EQUAL(result, psa_fwu_start(component, NULL, 0));
}

TEST(mock_psa_fwu_m, write)
{
	const size_t image_offset = 0x1000;
	uint8_t block[8];

	memset(block, 0x5a, sizeof(block));

	expect_mock_psa_fwu_write(component, image_offset, block, sizeof(block), result);
	LONGS_EQUAL(result, psa_fwu_write(component, image_offset, block, sizeof(block)));
}

TEST(mock_psa_fwu_m, finish)
{
	expect_mock_psa_fwu_finish(component, result);
	LONGS_EQUAL(result, psa_fwu_finish(component));
}

TEST(mock_psa_fwu_m, cancel)
{
	expect_mock_psa_fwu_cancel(component, result);
	LONGS_EQUAL(result, psa_fwu_cancel(component));
}

TEST(mock_psa_fwu_m, clean)
{
	expect_mock_psa_fwu_clean(component, result);
	LONGS_EQUAL(result, psa_fwu_clean(component));
}

TEST(mock_psa_fwu_m, install)
{
	expect_mock_psa_fwu_install(result);
	LONGS_EQUAL(result, psa_fwu_install());
}

TEST(mock_psa_fwu_m, request_reboot)
{
	expect_mock_psa_fwu_request_reboot(result);
	LONGS_EQUAL(result, psa_fwu_request_reboot());
}

TEST(mock_psa_fwu_m, reject)
{
	psa_status_t error = PSA_ERROR_STORAGE_FAILURE;

	expect_mock_psa_fwu_reject(error, result);
	LONGS_EQUAL(result, psa_fwu_reject(error));
}

TEST(mock_psa_fwu_m, accept)
{
	expect_mock_psa_fwu_accept(result);
	LONGS_EQUAL(result, psa_fwu_accept());
}
