/*
 * Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <CppUTest/TestHarness.h>
#include <vector>

#include "protocols/service/fwu/status.h"
#include "service/fwu/test/fwu_dut/fwu_dut.h"
#include "service/fwu/test/fwu_dut_factory/fwu_dut_factory.h"
/*
 * Tests to check that FWU metadata is never left in an invalid state
 * during the update process when unexpected power failures occur. Power
 * failures are simulated by shutting down the DUT after each write to
 * metadata and re-boot to check all conditions for a successful boot
 * are met.
 */
TEST_GROUP(FwuPowerFailureTests)
{
	void setup()
	{
		m_dut = NULL;
		m_fwu_client = NULL;
		m_metadata_checker = NULL;
	}

	void teardown()
	{
		delete m_metadata_checker;
		m_metadata_checker = NULL;

		delete m_fwu_client;
		m_fwu_client = NULL;

		delete m_dut;
		m_dut = NULL;
	}

	fwu_dut *m_dut;
	metadata_checker *m_metadata_checker;
	fwu_client *m_fwu_client;
};

TEST(FwuPowerFailureTests, powerFailureDuringStaging)
{
	int status = 0;
	struct uuid_octets uuid;
	uint32_t stream_handle = 0;
	std::vector<uint8_t> image_data;

	m_dut = fwu_dut_factory::create(3, true);
	m_fwu_client = m_dut->create_fwu_client();
	m_metadata_checker = m_dut->create_metadata_checker();

	m_dut->boot();

	/* Check assumptions about the first boot. */
	struct boot_info boot_info = m_dut->get_boot_info();
	UNSIGNED_LONGS_EQUAL(boot_info.boot_index, boot_info.active_index);
	m_metadata_checker->check_regular(boot_info.boot_index);

	/* Begin staging */
	status = m_fwu_client->begin_staging(0, 0, NULL);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);
	m_metadata_checker->check_ready_for_staging(boot_info.boot_index);

	/* Power cycle */
	m_dut->shutdown();
	m_dut->boot();

	/* Expect to reboot into the regular state without errors */
	boot_info = m_dut->get_boot_info();
	m_metadata_checker->check_regular(boot_info.boot_index);

	/* Begin staging again */
	status = m_fwu_client->begin_staging(0, 0, NULL);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);
	m_metadata_checker->check_ready_for_staging(boot_info.boot_index);

	/* Start installing an image but don't commit it */
	m_dut->whole_volume_image_type_uuid(0, &uuid);
	status = m_fwu_client->open(&uuid, fwu_client::op_type::WRITE, &stream_handle);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	m_dut->generate_image_data(&image_data);

	status = m_fwu_client->write_stream(stream_handle,
					    reinterpret_cast<const uint8_t *>(image_data.data()),
					    image_data.size());
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	/* Power cycle */
	m_dut->shutdown();
	m_dut->boot();

	/* Expect to reboot into the regular state without errors */
	boot_info = m_dut->get_boot_info();
	m_metadata_checker->check_regular(boot_info.boot_index);

	/* Begin staging again */
	status = m_fwu_client->begin_staging(0, 0, NULL);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);
	m_metadata_checker->check_ready_for_staging(boot_info.boot_index);

	/* Start installing an image but this time commit it without ending staging */
	m_dut->whole_volume_image_type_uuid(1, &uuid);
	status = m_fwu_client->open(&uuid, fwu_client::op_type::WRITE, &stream_handle);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	m_dut->generate_image_data(&image_data);

	status = m_fwu_client->write_stream(stream_handle,
					    reinterpret_cast<const uint8_t *>(image_data.data()),
					    image_data.size());
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	status = m_fwu_client->commit(stream_handle, false);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	/* Power cycle */
	m_dut->shutdown();
	m_dut->boot();

	/* Expect to reboot into the regular state without errors */
	boot_info = m_dut->get_boot_info();
	m_metadata_checker->check_regular(boot_info.boot_index);
}

TEST(FwuPowerFailureTests, powerFailureDuringTrial)
{
	int status = 0;
	struct uuid_octets uuid;
	uint32_t stream_handle = 0;
	std::vector<uint8_t> image_data;

	m_dut = fwu_dut_factory::create(3, true);
	m_fwu_client = m_dut->create_fwu_client();
	m_metadata_checker = m_dut->create_metadata_checker();

	m_dut->boot();

	/* Check assumptions about the first boot. */
	struct boot_info boot_info = m_dut->get_boot_info();
	UNSIGNED_LONGS_EQUAL(boot_info.boot_index, boot_info.active_index);
	m_metadata_checker->check_regular(boot_info.boot_index);

	/* Begin staging */
	status = m_fwu_client->begin_staging(0, 0, NULL);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);
	m_metadata_checker->check_ready_for_staging(boot_info.boot_index);

	/* Install a partial update */
	m_dut->whole_volume_image_type_uuid(2, &uuid);
	status = m_fwu_client->open(&uuid, fwu_client::op_type::WRITE, &stream_handle);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	m_dut->generate_image_data(&image_data);

	status = m_fwu_client->write_stream(stream_handle,
					    reinterpret_cast<const uint8_t *>(image_data.data()),
					    image_data.size());
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	status = m_fwu_client->commit(stream_handle, false);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	/* Transition to the trial state */
	status = m_fwu_client->end_staging();
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);
	m_metadata_checker->check_ready_to_activate(boot_info.boot_index);

	/* Power cycle */
	m_dut->shutdown();
	m_dut->boot();

	/* Check the metadata after the reboot to activate the update */
	boot_info = m_dut->get_boot_info();
	m_metadata_checker->check_trial(boot_info.boot_index);

	/* Power cycle again */
	m_dut->shutdown();
	m_dut->boot();

	/* Check trial is still active */
	boot_info = m_dut->get_boot_info();
	m_metadata_checker->check_trial(boot_info.boot_index);

	/* Power cycle again */
	m_dut->shutdown();
	m_dut->boot();

	/* Check trial is still active */
	boot_info = m_dut->get_boot_info();
	m_metadata_checker->check_trial(boot_info.boot_index);

	/* Only image 2 should need accepting as it was the only image updated */
	m_dut->whole_volume_image_type_uuid(2, &uuid);
	status = m_fwu_client->accept(&uuid);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	/* Power cycle again */
	m_dut->shutdown();
	m_dut->boot();

	/* Check transition to regular */
	boot_info = m_dut->get_boot_info();
	m_metadata_checker->check_regular(boot_info.boot_index);
}
