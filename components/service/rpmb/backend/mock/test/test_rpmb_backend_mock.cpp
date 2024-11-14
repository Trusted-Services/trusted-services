/*
 * Copyright (c) 2023-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include "../rpmb_backend_mock.h"
#include <string.h>

TEST_GROUP(rpmb_backend_mock) {
	TEST_SETUP()
	{
		backend = rpmb_backend_mock_init(&mock_backend);
	}

	TEST_TEARDOWN()
	{
		rpmb_backend_mock_deinit(&mock_backend);
		mock().checkExpectations();
		mock().clear();
	}

	struct rpmb_backend *backend;
	struct rpmb_backend_mock mock_backend;
	const uint32_t dev_id = 1;
};

TEST(rpmb_backend_mock, get_dev_info)
{
	const struct rpmb_dev_info expected_dev_info = {
		.cid = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f},
		.rpmb_size_mult = 100
	};
	struct rpmb_dev_info dev_info = { 0 };
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;

	rpmb_backend_mock_expect_get_dev_info(backend, dev_id, &expected_dev_info, PSA_SUCCESS);

	status = rpmb_backend_get_dev_info(backend, dev_id, &dev_info);
	LONGS_EQUAL(PSA_SUCCESS, status);
	MEMCMP_EQUAL(&expected_dev_info, &dev_info, sizeof(expected_dev_info));
}

TEST(rpmb_backend_mock, data_request)
{
	struct rpmb_data_frame request_frames[2];
	struct rpmb_data_frame expected_response_frames[3];
	struct rpmb_data_frame response_frames[3] = { 0 };
	size_t expected_response_frame_count = 3;
	size_t response_frame_count = 4;
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;

	memset(request_frames, 0x11, sizeof(request_frames));
	memset(expected_response_frames, 0x22, sizeof(response_frames));

	rpmb_backend_mock_expect_data_request(backend, dev_id,
					      request_frames, 2,
					      expected_response_frames, 4, &expected_response_frame_count,
					      PSA_SUCCESS);

	status = rpmb_backend_data_request(backend, dev_id, request_frames, 2,
					   response_frames, &response_frame_count);
	LONGS_EQUAL(PSA_SUCCESS, status);
	UNSIGNED_LONGS_EQUAL(expected_response_frame_count, response_frame_count);
	MEMCMP_EQUAL(&expected_response_frames, &response_frames, sizeof(expected_response_frames));
}