/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "../psa_fwu_m_update_agent.h"
#include "service/fwu/psa_fwu_m/interface/mock/mock_psa_fwu_m.h"
#include "protocols/service/fwu/fwu_proto.h"
#include <string.h>
#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

TEST_GROUP(psa_fwu_m_update_agent) {
	TEST_SETUP() {
		agent = psa_fwu_m_update_agent_init(mapping, 2, 4096);
		handle = 0;
		progress = 0;
		total_work = 0;
	}

	TEST_TEARDOWN() {
		psa_fwu_m_update_agent_deinit(agent);

		mock().checkExpectations();
		mock().clear();
	}

	void begin_staging() {
		expect_mock_psa_fwu_start(mapping[0].component, NULL, 0, PSA_SUCCESS);
		expect_mock_psa_fwu_start(mapping[1].component, NULL, 0, PSA_SUCCESS);
		LONGS_EQUAL(FWU_STATUS_SUCCESS, update_agent_begin_staging(agent, 0, 0, NULL));
	}

	void end_staging() {
		expect_mock_psa_fwu_finish(mapping[0].component, PSA_SUCCESS);
		expect_mock_psa_fwu_finish(mapping[1].component, PSA_SUCCESS);

		expect_mock_psa_fwu_install(PSA_SUCCESS);

		LONGS_EQUAL(FWU_STATUS_SUCCESS, update_agent_end_staging(agent));
	}

	void open() {
		LONGS_EQUAL(FWU_STATUS_SUCCESS,
			    update_agent_open(agent, &mapping[0].uuid, FWU_OP_TYPE_WRITE, &handle));
	}

	void write(const uint8_t *data, size_t data_len) {
		expect_mock_psa_fwu_write(mapping[0].component, 0, NULL, 0, PSA_SUCCESS);
		LONGS_EQUAL(FWU_STATUS_SUCCESS,
			    update_agent_write_stream(agent, handle, data, data_len));
	}

	struct update_agent *agent;
	uint32_t handle;
	uint32_t progress;
	uint32_t total_work;

	const psa_fwu_m_image_mapping mapping[2] = {
		{
			.uuid = {
				0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
				0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
			},
			.component = 3
		},
		{
			.uuid = {
				0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08,
				0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00
			},
			.component = 2
		},
	};

	const struct uuid_octets image_directory_uuid = {
		0xde, 0xee, 0x58, 0xd9, 0x51, 0x47, 0x4a, 0xd3,
		0xa2, 0x90, 0x77, 0x66, 0x6e, 0x23, 0x41, 0xa5
	};
};

TEST(psa_fwu_m_update_agent, discover)
{
	fwu_discovery_result result = { 0 };

	LONGS_EQUAL(FWU_STATUS_SUCCESS, update_agent_discover(agent, &result));

	UNSIGNED_LONGS_EQUAL(0, result.service_status);
	UNSIGNED_LONGS_EQUAL(1, result.version_major);
	UNSIGNED_LONGS_EQUAL(0, result.version_minor);
	UNSIGNED_LONGS_EQUAL(1, result.flags);
}

TEST(psa_fwu_m_update_agent, begin_staging_start_fail)
{
	expect_mock_psa_fwu_start(mapping[0].component, NULL, 0, PSA_ERROR_GENERIC_ERROR);
	expect_mock_psa_fwu_clean(mapping[0].component, PSA_SUCCESS);
	expect_mock_psa_fwu_clean(mapping[1].component, PSA_SUCCESS);
	LONGS_EQUAL(FWU_STATUS_UNKNOWN, update_agent_begin_staging(agent, 0, 0, NULL));
}

TEST(psa_fwu_m_update_agent, begin_staging_start_and_clean_fail)
{
	expect_mock_psa_fwu_start(mapping[0].component, NULL, 0, PSA_ERROR_GENERIC_ERROR);
	expect_mock_psa_fwu_clean(mapping[0].component, PSA_ERROR_GENERIC_ERROR);
	LONGS_EQUAL(FWU_STATUS_DENIED, update_agent_begin_staging(agent, 0, 0, NULL));
}

TEST(psa_fwu_m_update_agent, begin_staging_partial_invalid_uuid)
{
	const struct uuid_octets update_guid = { 0 };

	expect_mock_psa_fwu_clean(mapping[0].component, PSA_SUCCESS);
	expect_mock_psa_fwu_clean(mapping[1].component, PSA_SUCCESS);

	LONGS_EQUAL(FWU_STATUS_UNKNOWN, update_agent_begin_staging(agent, 0, 1, &update_guid));
}

TEST(psa_fwu_m_update_agent, begin_staging_partial_image_directory_uuid)
{
	expect_mock_psa_fwu_clean(mapping[0].component, PSA_SUCCESS);
	expect_mock_psa_fwu_clean(mapping[1].component, PSA_SUCCESS);

	LONGS_EQUAL(FWU_STATUS_UNKNOWN, update_agent_begin_staging(agent, 0, 1, &image_directory_uuid));
}

TEST(psa_fwu_m_update_agent, begin_staging_partial_start_fail)
{
	expect_mock_psa_fwu_start(mapping[0].component, NULL, 0, PSA_ERROR_GENERIC_ERROR);
	expect_mock_psa_fwu_clean(mapping[0].component, PSA_SUCCESS);
	expect_mock_psa_fwu_clean(mapping[1].component, PSA_SUCCESS);

	LONGS_EQUAL(FWU_STATUS_UNKNOWN, update_agent_begin_staging(agent, 0, 1, &mapping[0].uuid));
}

TEST(psa_fwu_m_update_agent, begin_staging_partial_start_and_clean_fail)
{
	expect_mock_psa_fwu_start(mapping[0].component, NULL, 0, PSA_ERROR_GENERIC_ERROR);
	expect_mock_psa_fwu_clean(mapping[0].component, PSA_ERROR_GENERIC_ERROR);

	LONGS_EQUAL(FWU_STATUS_DENIED, update_agent_begin_staging(agent, 0, 1, &mapping[0].uuid));
}

TEST(psa_fwu_m_update_agent, begin_staging_partial)
{
	expect_mock_psa_fwu_start(mapping[0].component, NULL, 0, PSA_SUCCESS);

	LONGS_EQUAL(FWU_STATUS_SUCCESS, update_agent_begin_staging(agent, 0, 1, &mapping[0].uuid));
}

TEST(psa_fwu_m_update_agent, begin_staging)
{
	begin_staging();
}

TEST(psa_fwu_m_update_agent, begin_staging_repeated_cancel_fail)
{
	begin_staging();

	expect_mock_psa_fwu_cancel(mapping[0].component, PSA_SUCCESS);
	expect_mock_psa_fwu_cancel(mapping[1].component, PSA_ERROR_GENERIC_ERROR);

	expect_mock_psa_fwu_clean(mapping[0].component, PSA_SUCCESS);
	expect_mock_psa_fwu_clean(mapping[1].component, PSA_SUCCESS);

	LONGS_EQUAL(FWU_STATUS_UNKNOWN, update_agent_begin_staging(agent, 0, 0, NULL));
}

TEST(psa_fwu_m_update_agent, begin_staging_repeated_clean_fail)
{
	begin_staging();

	expect_mock_psa_fwu_cancel(mapping[0].component, PSA_SUCCESS);
	expect_mock_psa_fwu_cancel(mapping[1].component, PSA_SUCCESS);

	expect_mock_psa_fwu_clean(mapping[0].component, PSA_ERROR_GENERIC_ERROR);

	LONGS_EQUAL(FWU_STATUS_DENIED, update_agent_begin_staging(agent, 0, 0, NULL));
}

TEST(psa_fwu_m_update_agent, begin_staging_repeated)
{
	begin_staging();

	expect_mock_psa_fwu_cancel(mapping[0].component, PSA_SUCCESS);
	expect_mock_psa_fwu_cancel(mapping[1].component, PSA_SUCCESS);

	expect_mock_psa_fwu_clean(mapping[0].component, PSA_SUCCESS);
	expect_mock_psa_fwu_clean(mapping[1].component, PSA_SUCCESS);

	expect_mock_psa_fwu_start(mapping[0].component, NULL, 0, PSA_SUCCESS);
	expect_mock_psa_fwu_start(mapping[1].component, NULL, 0, PSA_SUCCESS);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, update_agent_begin_staging(agent, 0, 0, NULL));
}

TEST(psa_fwu_m_update_agent, begin_staging_in_trial_state)
{
	begin_staging();
	end_staging();
	LONGS_EQUAL(FWU_STATUS_DENIED, update_agent_begin_staging(agent, 0, 0, NULL));
}

TEST(psa_fwu_m_update_agent, end_staging_not_in_staging)
{
	LONGS_EQUAL(FWU_STATUS_DENIED, update_agent_end_staging(agent));
}

TEST(psa_fwu_m_update_agent, end_staging_finish_fail)
{
	begin_staging();

	expect_mock_psa_fwu_finish(mapping[0].component, PSA_SUCCESS);
	expect_mock_psa_fwu_finish(mapping[1].component, PSA_ERROR_GENERIC_ERROR);

	LONGS_EQUAL(FWU_STATUS_DENIED, update_agent_end_staging(agent));
}

TEST(psa_fwu_m_update_agent, end_staging_install_fail)
{
	begin_staging();

	expect_mock_psa_fwu_finish(mapping[0].component, PSA_SUCCESS);
	expect_mock_psa_fwu_finish(mapping[1].component, PSA_SUCCESS);

	expect_mock_psa_fwu_install(PSA_ERROR_GENERIC_ERROR);

	LONGS_EQUAL(FWU_STATUS_DENIED, update_agent_end_staging(agent));
}

TEST(psa_fwu_m_update_agent, end_staging)
{
	begin_staging();
	end_staging();

	// In trial state, so a repeated begin_staging should not succeed
	LONGS_EQUAL(FWU_STATUS_DENIED, update_agent_begin_staging(agent, 0, 0, NULL));
}

TEST(psa_fwu_m_update_agent, end_staging_with_opened_handles)
{
	begin_staging();
	open();

	LONGS_EQUAL(FWU_STATUS_BUSY, update_agent_end_staging(agent));
}

TEST(psa_fwu_m_update_agent, end_staging_all_accepted_accept_fail)
{
	begin_staging();

	LONGS_EQUAL(FWU_STATUS_SUCCESS,
		    update_agent_open(agent, &mapping[0].uuid, FWU_OP_TYPE_WRITE, &handle));
	LONGS_EQUAL(FWU_STATUS_SUCCESS,
		    update_agent_commit(agent, handle, true, 0, &progress, &total_work));

	LONGS_EQUAL(FWU_STATUS_SUCCESS,
		    update_agent_open(agent, &mapping[1].uuid, FWU_OP_TYPE_WRITE, &handle));
	LONGS_EQUAL(FWU_STATUS_SUCCESS,
		    update_agent_commit(agent, handle, true, 0, &progress, &total_work));

	expect_mock_psa_fwu_finish(mapping[0].component, PSA_SUCCESS);
	expect_mock_psa_fwu_finish(mapping[1].component, PSA_SUCCESS);

	expect_mock_psa_fwu_install(PSA_SUCCESS);

	expect_mock_psa_fwu_accept(PSA_ERROR_GENERIC_ERROR);

	LONGS_EQUAL(FWU_STATUS_DENIED, update_agent_end_staging(agent));
}

TEST(psa_fwu_m_update_agent, end_staging_all_accepted)
{
	begin_staging();

	LONGS_EQUAL(FWU_STATUS_SUCCESS,
		    update_agent_open(agent, &mapping[0].uuid, FWU_OP_TYPE_WRITE, &handle));
	LONGS_EQUAL(FWU_STATUS_SUCCESS,
		    update_agent_commit(agent, handle, true, 0, &progress, &total_work));

	LONGS_EQUAL(FWU_STATUS_SUCCESS,
		    update_agent_open(agent, &mapping[1].uuid, FWU_OP_TYPE_WRITE, &handle));
	LONGS_EQUAL(FWU_STATUS_SUCCESS,
		    update_agent_commit(agent, handle, true, 0, &progress, &total_work));

	expect_mock_psa_fwu_accept(PSA_SUCCESS);
	end_staging();

	// In regular state, repeated begin_staging should succeed
	begin_staging();
}

TEST(psa_fwu_m_update_agent, cancel_staging_not_in_staging)
{
	LONGS_EQUAL(FWU_STATUS_DENIED, update_agent_cancel_staging(agent));
}

TEST(psa_fwu_m_update_agent, cancel_staging_cancel_fail)
{
	begin_staging();

	expect_mock_psa_fwu_cancel(mapping[0].component, PSA_ERROR_GENERIC_ERROR);

	LONGS_EQUAL(FWU_STATUS_DENIED, update_agent_cancel_staging(agent));
}

TEST(psa_fwu_m_update_agent, cancel_staging)
{
	begin_staging();

	expect_mock_psa_fwu_cancel(mapping[0].component, PSA_SUCCESS);
	expect_mock_psa_fwu_cancel(mapping[1].component, PSA_SUCCESS);

	LONGS_EQUAL(FWU_STATUS_SUCCESS, update_agent_cancel_staging(agent));
}

TEST(psa_fwu_m_update_agent, open_for_write_not_staging)
{
	LONGS_EQUAL(FWU_STATUS_DENIED, update_agent_open(agent, &mapping[0].uuid,
		    FWU_OP_TYPE_WRITE, &handle));
}


TEST(psa_fwu_m_update_agent, open_for_write_uuid_not_exists)
{
	const struct uuid_octets uuid = { 0 };

	begin_staging();

	LONGS_EQUAL(FWU_STATUS_UNKNOWN, update_agent_open(agent, &uuid, FWU_OP_TYPE_WRITE,
							  &handle));
}

TEST(psa_fwu_m_update_agent, open_for_write_image_directory)
{
	begin_staging();

	LONGS_EQUAL(FWU_STATUS_DENIED,
		    update_agent_open(agent, &image_directory_uuid, FWU_OP_TYPE_WRITE, &handle));
}

TEST(psa_fwu_m_update_agent, open_for_write_partial_not_staging)
{
	expect_mock_psa_fwu_start(mapping[0].component, NULL, 0, PSA_SUCCESS);

	LONGS_EQUAL(FWU_STATUS_SUCCESS, update_agent_begin_staging(agent, 0, 1, &mapping[0].uuid));

	LONGS_EQUAL(FWU_STATUS_DENIED,
		    update_agent_open(agent, &mapping[1].uuid, FWU_OP_TYPE_WRITE, &handle));
}

TEST(psa_fwu_m_update_agent, open_for_read)
{
	begin_staging();

	LONGS_EQUAL(FWU_STATUS_NOT_AVAILABLE,
		    update_agent_open(agent, &mapping[0].uuid, FWU_OP_TYPE_READ, &handle));
}

TEST(psa_fwu_m_update_agent, open)
{
	begin_staging();
	open();
}

TEST(psa_fwu_m_update_agent, open_too_many)
{
	begin_staging();

	while (1) {
		int result = FWU_STATUS_DENIED;

		result = update_agent_open(agent, &mapping[0].uuid, FWU_OP_TYPE_WRITE, &handle);
		if (result == FWU_STATUS_NOT_AVAILABLE) {
			break;
		} else {
			LONGS_EQUAL(FWU_STATUS_SUCCESS, result);
		}
	}
}

TEST(psa_fwu_m_update_agent, write_stream_not_in_staging)
{
	LONGS_EQUAL(FWU_STATUS_DENIED, update_agent_write_stream(agent, 0, NULL, 0));
}

TEST(psa_fwu_m_update_agent, write_stream_invalid_handle)
{
	begin_staging();

	LONGS_EQUAL(FWU_STATUS_UNKNOWN, update_agent_write_stream(agent, 0, NULL, 0));
	LONGS_EQUAL(FWU_STATUS_UNKNOWN, update_agent_write_stream(agent, 0xffffffff, NULL, 0));
}

TEST(psa_fwu_m_update_agent, write_stream_opened_for_read)
{
	begin_staging();

	LONGS_EQUAL(FWU_STATUS_SUCCESS,
		    update_agent_open(agent, &image_directory_uuid, FWU_OP_TYPE_READ, &handle));
	LONGS_EQUAL(FWU_STATUS_NO_PERMISSION, update_agent_write_stream(agent, handle, NULL, 0));
}

TEST(psa_fwu_m_update_agent, write_stream_write_fail)
{
	begin_staging();
	open();

	expect_mock_psa_fwu_write(mapping[0].component, 0, NULL, 0, PSA_ERROR_GENERIC_ERROR);
	LONGS_EQUAL(FWU_STATUS_DENIED, update_agent_write_stream(agent, handle, NULL, 0));
}

TEST(psa_fwu_m_update_agent, write_stream_overflow)
{
	uint8_t data[8];

	memset(data, 0x5a, sizeof(data));

	begin_staging();
	open();

	expect_mock_psa_fwu_write(mapping[0].component, 0, data, sizeof(data), PSA_SUCCESS);
	LONGS_EQUAL(FWU_STATUS_SUCCESS,
		    update_agent_write_stream(agent, handle, data, sizeof(data)));

	LONGS_EQUAL(FWU_STATUS_OUT_OF_BOUNDS,
		    update_agent_write_stream(agent, handle, NULL, 0xffffffffffffffff));
}

TEST(psa_fwu_m_update_agent, write_stream)
{
	uint8_t data[8];

	memset(data, 0x5a, sizeof(data));

	begin_staging();
	open();

	expect_mock_psa_fwu_write(mapping[0].component, 0, data, sizeof(data), PSA_SUCCESS);
	LONGS_EQUAL(FWU_STATUS_SUCCESS,
		    update_agent_write_stream(agent, handle, data, sizeof(data)));

	expect_mock_psa_fwu_write(mapping[0].component, sizeof(data), data, sizeof(data), PSA_SUCCESS);
	LONGS_EQUAL(FWU_STATUS_SUCCESS,
		    update_agent_write_stream(agent, handle, data, sizeof(data)));
}

TEST(psa_fwu_m_update_agent, read_stream_invalid_handle)
{
	LONGS_EQUAL(FWU_STATUS_UNKNOWN, update_agent_read_stream(agent, 0, NULL, 0, NULL, NULL));
	LONGS_EQUAL(FWU_STATUS_UNKNOWN,
		    update_agent_read_stream(agent, 0xffffffff, NULL, 0, NULL, NULL));
}

TEST(psa_fwu_m_update_agent, read_stream_opened_for_write)
{
	begin_staging();
	open();

	LONGS_EQUAL(FWU_STATUS_NO_PERMISSION,
		    update_agent_read_stream(agent, handle, NULL, 0, NULL, NULL));
}

TEST(psa_fwu_m_update_agent, read_image_directory_too_short)
{
	const size_t expected_len = sizeof(fwu_image_directory) + 2 * sizeof(fwu_image_info_entry);
	size_t read_len = 0;
	size_t total_len = 0;

	LONGS_EQUAL(FWU_STATUS_SUCCESS,
		    update_agent_open(agent, &image_directory_uuid, FWU_OP_TYPE_READ, &handle));

	LONGS_EQUAL(FWU_STATUS_SUCCESS,
		    update_agent_read_stream(agent, handle, NULL, 0, &read_len, &total_len));

	UNSIGNED_LONGS_EQUAL(0, read_len);
	UNSIGNED_LONGS_EQUAL(expected_len, total_len);
}

TEST(psa_fwu_m_update_agent, read_image_directory_query_fail)
{
	const size_t expected_len = sizeof(fwu_image_directory) + 2 * sizeof(fwu_image_info_entry);
	uint8_t buffer[expected_len] = { 0 };
	psa_fwu_component_info_t info = { 0 };
	size_t read_len = 0;
	size_t total_len = 0;

	LONGS_EQUAL(FWU_STATUS_SUCCESS,
		    update_agent_open(agent, &image_directory_uuid, FWU_OP_TYPE_READ, &handle));

	expect_mock_psa_fwu_query(mapping[0].component, &info, PSA_ERROR_GENERIC_ERROR);
	LONGS_EQUAL(FWU_STATUS_DENIED,
		    update_agent_read_stream(agent, handle, buffer, sizeof(buffer), &read_len,
		    			     &total_len));

	UNSIGNED_LONGS_EQUAL(0, read_len);
	UNSIGNED_LONGS_EQUAL(expected_len, total_len);
}

TEST(psa_fwu_m_update_agent, read_image_directory)
{
	const size_t expected_len = sizeof(fwu_image_directory) + 2 * sizeof(fwu_image_info_entry);
	uint8_t buffer[expected_len] = { 0 };
	psa_fwu_component_info_t info0 = {
		.state = PSA_FWU_REJECTED,
		.version = {.major = 1, .minor = 2, .patch = 3},
		.max_size = 123,

	};
	psa_fwu_component_info_t info1 = {
		.state = PSA_FWU_UPDATED,
		.version = {.major = 6, .minor = 7, .patch = 8},
		.max_size = 456,
	};
	size_t read_len = 0;
	size_t total_len = 0;

	LONGS_EQUAL(FWU_STATUS_SUCCESS,
		    update_agent_open(agent, &image_directory_uuid, FWU_OP_TYPE_READ, &handle));

	expect_mock_psa_fwu_query(mapping[0].component, &info0, PSA_SUCCESS);
	expect_mock_psa_fwu_query(mapping[1].component, &info1, PSA_SUCCESS);
	LONGS_EQUAL(FWU_STATUS_SUCCESS,
		    update_agent_read_stream(agent, handle, buffer, sizeof(buffer), &read_len,
		    			     &total_len));

	UNSIGNED_LONGS_EQUAL(expected_len, read_len);
	UNSIGNED_LONGS_EQUAL(expected_len, total_len);

	struct fwu_image_directory *directory = (struct fwu_image_directory *)buffer;
	UNSIGNED_LONGS_EQUAL(2, directory->directory_version);
	UNSIGNED_LONGS_EQUAL(0x18, directory->img_info_offset);
	UNSIGNED_LONGS_EQUAL(2, directory->num_images);
	UNSIGNED_LONGS_EQUAL(0, directory->correct_boot);
	UNSIGNED_LONGS_EQUAL(0x28, directory->img_info_size);

	MEMCMP_EQUAL(&mapping[0].uuid, directory->img_info_entry[0].img_type_uuid,
		     sizeof(mapping[0].uuid));
	UNSIGNED_LONGS_EQUAL(1, directory->img_info_entry[0].client_permissions);
	UNSIGNED_LONGS_EQUAL(info0.max_size, directory->img_info_entry[0].img_max_size);
	UNSIGNED_LONGS_EQUAL(0, directory->img_info_entry[0].lowest_accepted_version);
	UNSIGNED_LONGLONGS_EQUAL(0x01020003, directory->img_info_entry[0].img_version);
	UNSIGNED_LONGS_EQUAL(0, directory->img_info_entry[0].accepted);
	UNSIGNED_LONGS_EQUAL(0, directory->img_info_entry[0].reserved);

	MEMCMP_EQUAL(&mapping[1].uuid, directory->img_info_entry[1].img_type_uuid,
		     sizeof(mapping[1].uuid));
	UNSIGNED_LONGS_EQUAL(1, directory->img_info_entry[1].client_permissions);
	UNSIGNED_LONGS_EQUAL(info1.max_size, directory->img_info_entry[1].img_max_size);
	UNSIGNED_LONGS_EQUAL(0, directory->img_info_entry[1].lowest_accepted_version);
	UNSIGNED_LONGLONGS_EQUAL(0x06070008, directory->img_info_entry[1].img_version);
	UNSIGNED_LONGS_EQUAL(1, directory->img_info_entry[1].accepted);
	UNSIGNED_LONGS_EQUAL(0, directory->img_info_entry[1].reserved);
}

TEST(psa_fwu_m_update_agent, commit_invalid_handle)
{
	LONGS_EQUAL(FWU_STATUS_UNKNOWN, update_agent_commit(agent, 0, false, 0, &progress,
							    &total_work));
	LONGS_EQUAL(FWU_STATUS_UNKNOWN, update_agent_commit(agent, 0xffffffff, false, 0, &progress,
							    &total_work));
}

TEST(psa_fwu_m_update_agent, commit_read)
{
	LONGS_EQUAL(FWU_STATUS_SUCCESS,
		    update_agent_open(agent, &image_directory_uuid, FWU_OP_TYPE_READ, &handle));

	LONGS_EQUAL(FWU_STATUS_SUCCESS, update_agent_commit(agent, handle, false, 0, &progress,
							    &total_work));
}

TEST(psa_fwu_m_update_agent, commit_write)
{
	begin_staging();
	open();

	LONGS_EQUAL(FWU_STATUS_SUCCESS, update_agent_commit(agent, handle, false, 0, &progress,
							    &total_work));
}

TEST(psa_fwu_m_update_agent, accept_image_not_in_trial)
{
	LONGS_EQUAL(FWU_STATUS_DENIED, update_agent_accept_image(agent, NULL));
}

TEST(psa_fwu_m_update_agent, accept_image_invalid_uuid)
{
	struct uuid_octets uuid = { 0 };

	begin_staging();
	end_staging();
	LONGS_EQUAL(FWU_STATUS_UNKNOWN, update_agent_accept_image(agent, &uuid));
}

TEST(psa_fwu_m_update_agent, accept_image_not_selected)
{
	begin_staging();
	end_staging();
	LONGS_EQUAL(FWU_STATUS_DENIED, update_agent_accept_image(agent, &image_directory_uuid));
}

TEST(psa_fwu_m_update_agent, accept_image_one)
{
	begin_staging();
	end_staging();
	LONGS_EQUAL(FWU_STATUS_SUCCESS, update_agent_accept_image(agent, &mapping[0].uuid));
}

TEST(psa_fwu_m_update_agent, accept_image_accept_fail)
{
	begin_staging();
	end_staging();

	expect_mock_psa_fwu_accept(PSA_ERROR_GENERIC_ERROR);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, update_agent_accept_image(agent, &mapping[0].uuid));
	LONGS_EQUAL(FWU_STATUS_DENIED, update_agent_accept_image(agent, &mapping[1].uuid));
}

TEST(psa_fwu_m_update_agent, accept_image)
{
	begin_staging();
	end_staging();

	expect_mock_psa_fwu_accept(PSA_SUCCESS);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, update_agent_accept_image(agent, &mapping[0].uuid));
	LONGS_EQUAL(FWU_STATUS_SUCCESS, update_agent_accept_image(agent, &mapping[1].uuid));
}


TEST(psa_fwu_m_update_agent, select_previous_not_in_trial)
{
	LONGS_EQUAL(FWU_STATUS_DENIED, update_agent_select_previous(agent));
}

TEST(psa_fwu_m_update_agent, select_previous_reject_fail)
{
	/* This test also covers error code conversion */
	begin_staging();
	end_staging();

	expect_mock_psa_fwu_reject(0, PSA_ERROR_DOES_NOT_EXIST);
	LONGS_EQUAL(FWU_STATUS_UNKNOWN, update_agent_select_previous(agent));

	expect_mock_psa_fwu_reject(0, PSA_ERROR_INVALID_ARGUMENT);
	LONGS_EQUAL(FWU_STATUS_OUT_OF_BOUNDS, update_agent_select_previous(agent));


	expect_mock_psa_fwu_reject(0, PSA_ERROR_INVALID_SIGNATURE);
	LONGS_EQUAL(FWU_STATUS_AUTH_FAIL, update_agent_select_previous(agent));


	expect_mock_psa_fwu_reject(0, PSA_ERROR_NOT_PERMITTED);
	LONGS_EQUAL(FWU_STATUS_NO_PERMISSION, update_agent_select_previous(agent));


	expect_mock_psa_fwu_reject(0, PSA_ERROR_BAD_STATE);
	LONGS_EQUAL(FWU_STATUS_DENIED, update_agent_select_previous(agent));
}

TEST(psa_fwu_m_update_agent, select_previous)
{
	/* This test also covers error code conversion */
	begin_staging();
	end_staging();

	expect_mock_psa_fwu_reject(0, PSA_SUCCESS);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, update_agent_select_previous(agent));
}