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
 * Tests that check update rollback during the trial state where an
 * update is abandoned and the previous active image becomes the active
 * image.
 */
TEST_GROUP(FwuRollbackTests)
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

/*
 * A client requests select_previous after staging an update but before the
 * restart that activates the update.
 */
TEST(FwuRollbackTests, selectPreviousPriorToActivation)
{
	int status = 0;
	struct uuid_octets uuid;
	uint32_t stream_handle = 0;

	/* Create DUT */
	m_dut = fwu_dut_factory::create(1, false);
	m_fwu_client = m_dut->create_fwu_client();
	m_metadata_checker = m_dut->create_metadata_checker();

	m_dut->boot();

	struct boot_info boot_info = m_dut->get_boot_info();
	UNSIGNED_LONGS_EQUAL(boot_info.boot_index, boot_info.active_index);
	m_metadata_checker->check_regular(boot_info.boot_index);

	/* Note the pre-update bank index */
	unsigned int pre_update_bank_index = boot_info.boot_index;

	/* Install the update */
	status = m_fwu_client->begin_staging(0, 0, NULL);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);
	m_metadata_checker->check_ready_for_staging(boot_info.boot_index);

	m_dut->whole_volume_image_type_uuid(0, &uuid);
	status = m_fwu_client->open(&uuid, fwu_client::op_type::WRITE, &stream_handle);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	std::vector<uint8_t> image_data;
	m_dut->generate_image_data(&image_data);

	status = m_fwu_client->write_stream(stream_handle,
					    reinterpret_cast<const uint8_t *>(image_data.data()),
					    image_data.size());
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	status = m_fwu_client->commit(stream_handle, false);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	status = m_fwu_client->end_staging();
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);
	m_metadata_checker->check_ready_to_activate(boot_info.boot_index);

	/* Rollback to pre-update state - note that the update has not yet
	 * been activated.
	 */
	status = m_fwu_client->select_previous();
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);
	m_metadata_checker->check_regular(boot_info.boot_index);

	/* Reboot */
	m_dut->shutdown();
	m_dut->boot();

	/* Check that the pre-update state is restored */
	boot_info = m_dut->get_boot_info();
	UNSIGNED_LONGS_EQUAL(boot_info.boot_index, boot_info.active_index);
	UNSIGNED_LONGS_EQUAL(pre_update_bank_index, boot_info.boot_index);
	m_metadata_checker->check_regular(boot_info.boot_index);
}

/*
 * A client requests select_previous after activating the update.
 */
TEST(FwuRollbackTests, selectPreviousAfterActivation)
{
	int status = 0;
	struct uuid_octets uuid;
	uint32_t stream_handle = 0;

	/* Create DUT */
	m_dut = fwu_dut_factory::create(1, false);
	m_fwu_client = m_dut->create_fwu_client();
	m_metadata_checker = m_dut->create_metadata_checker();

	m_dut->boot();

	struct boot_info boot_info = m_dut->get_boot_info();
	UNSIGNED_LONGS_EQUAL(boot_info.boot_index, boot_info.active_index);
	m_metadata_checker->check_regular(boot_info.boot_index);

	/* Note the pre-update bank index */
	unsigned int pre_update_bank_index = boot_info.boot_index;

	/* Install the update */
	status = m_fwu_client->begin_staging(0, 0, NULL);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);
	m_metadata_checker->check_ready_for_staging(boot_info.boot_index);

	m_dut->whole_volume_image_type_uuid(0, &uuid);
	status = m_fwu_client->open(&uuid, fwu_client::op_type::WRITE, &stream_handle);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	std::vector<uint8_t> image_data;
	m_dut->generate_image_data(&image_data);

	status = m_fwu_client->write_stream(stream_handle,
					    reinterpret_cast<const uint8_t *>(image_data.data()),
					    image_data.size());
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	status = m_fwu_client->commit(stream_handle, false);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	status = m_fwu_client->end_staging();
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);
	m_metadata_checker->check_ready_to_activate(boot_info.boot_index);

	/* Reboot to activate update*/
	m_dut->shutdown();
	m_dut->boot();

	/* Expect to be in trial state */
	boot_info = m_dut->get_boot_info();
	UNSIGNED_LONGS_EQUAL(boot_info.boot_index, boot_info.active_index);
	m_metadata_checker->check_trial(boot_info.boot_index);

	/* Rollback to pre-update state */
	status = m_fwu_client->select_previous();
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	/* Reboot */
	m_dut->shutdown();
	m_dut->boot();

	/* Check that the pre-update state is restored */
	boot_info = m_dut->get_boot_info();
	UNSIGNED_LONGS_EQUAL(boot_info.boot_index, boot_info.active_index);
	UNSIGNED_LONGS_EQUAL(pre_update_bank_index, boot_info.boot_index);
	m_metadata_checker->check_regular(boot_info.boot_index);
}

/*
 * The bootloader fails to boot from the active bank so it falls back
 * to booting from the previous active bank/
 */
TEST(FwuRollbackTests, bootloaderFallback)
{
	int status = 0;
	struct uuid_octets uuid;
	uint32_t stream_handle = 0;

	/* Create DUT */
	m_dut = fwu_dut_factory::create(1, false);
	m_fwu_client = m_dut->create_fwu_client();
	m_metadata_checker = m_dut->create_metadata_checker();

	m_dut->boot();

	struct boot_info boot_info = m_dut->get_boot_info();
	UNSIGNED_LONGS_EQUAL(boot_info.boot_index, boot_info.active_index);
	m_metadata_checker->check_regular(boot_info.boot_index);

	/* Note the pre-update bank index */
	unsigned int pre_update_bank_index = boot_info.boot_index;

	/* Install the update */
	status = m_fwu_client->begin_staging(0, 0, NULL);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);
	m_metadata_checker->check_ready_for_staging(boot_info.boot_index);

	m_dut->whole_volume_image_type_uuid(0, &uuid);
	status = m_fwu_client->open(&uuid, fwu_client::op_type::WRITE, &stream_handle);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	std::vector<uint8_t> image_data;
	m_dut->generate_image_data(&image_data);

	status = m_fwu_client->write_stream(stream_handle,
					    reinterpret_cast<const uint8_t *>(image_data.data()),
					    image_data.size());
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	status = m_fwu_client->commit(stream_handle, false);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	status = m_fwu_client->end_staging();
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);
	m_metadata_checker->check_ready_to_activate(boot_info.boot_index);

	/* Reboot to activate update but simulate a failure to boot from the
	 * new active bank. The bootloader instead boots from the previous
	 * active bank.
	 */
	m_dut->shutdown();
	m_dut->boot(false);

	/* Expect the bootloader to have booted from the previous active bank. */
	boot_info = m_dut->get_boot_info();
	UNSIGNED_LONGS_EQUAL(pre_update_bank_index, boot_info.boot_index);
	UNSIGNED_LONGS_EQUAL(boot_info.boot_index, boot_info.previous_active_index);
	CHECK_FALSE(boot_info.previous_active_index == boot_info.active_index);

	/* Expect the update agent to have spotted the bootloader decision and
	 * modified the metadata to prevent a repeat of the boot failure.
	 */
	uint32_t active_index;
	uint32_t previous_active_index;

	m_metadata_checker->get_active_indices(&active_index, &previous_active_index);
	UNSIGNED_LONGS_EQUAL(boot_info.boot_index, active_index);
	UNSIGNED_LONGS_EQUAL(boot_info.boot_index, previous_active_index);
	m_metadata_checker->check_regular(boot_info.boot_index);
}