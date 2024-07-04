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
#include "service/fwu/test/image_directory_checker/image_directory_checker.h"

/*
 * Tests that check defenses against invalid behaviour from a client.
 */
TEST_GROUP(FwuInvalidBehaviourTests)
{
	void setup()
	{
		m_dut = NULL;
		m_fwu_client = NULL;
		m_dir_checker = new image_directory_checker;
	}

	void teardown()
	{
		delete m_dir_checker;
		m_dir_checker = NULL;

		delete m_fwu_client;
		m_fwu_client = NULL;

		delete m_dut;
		m_dut = NULL;
	}

	fwu_dut *m_dut;
	image_directory_checker *m_dir_checker;
	fwu_client *m_fwu_client;
};

TEST(FwuInvalidBehaviourTests, invalidOperationsInRegular)
{
	int status = 0;
	uint32_t stream_handle = 0;
	struct uuid_octets uuid;

	/* Construct and boot a DUT with a single fw location. Assume
	 * an initial transition to the REGULAR FWU state.
	 */
	m_dut = fwu_dut_factory::create(1);
	m_fwu_client = m_dut->create_fwu_client();
	m_dut->boot();

	/* End staging without having entered STAGING */
	status = m_fwu_client->end_staging();
	LONGS_EQUAL(FWU_STATUS_DENIED, status);

	/* Cancel staging without having entered STAGING */
	status = m_fwu_client->cancel_staging();
	LONGS_EQUAL(FWU_STATUS_DENIED, status);

	/* Open a fw image when not STAGING */
	m_dut->whole_volume_image_type_uuid(0, &uuid);
	status = m_fwu_client->open(&uuid, fwu_client::op_type::WRITE, &stream_handle);
	LONGS_EQUAL(FWU_STATUS_DENIED, status);

	/* Write to a stream when not STAGING. Note also that it's not possible
	 * to obtain a stream handle outside of STAGING so use an arbitrary value.
	 * There are a few possible failure cases, depending on whether the stream
	 * handle qualifies an open stream or not. For this case, it shouldn't.
	 */
	std::string image_data("some image data...");

	stream_handle = 67;
	status = m_fwu_client->write_stream(stream_handle,
					    reinterpret_cast<const uint8_t *>(image_data.data()),
					    image_data.size());
	LONGS_EQUAL(FWU_STATUS_UNKNOWN, status);

	/* An attempt to commit with an invalid handle should fail in a similar way */
	stream_handle = 771;
	status = m_fwu_client->commit(stream_handle, false);
	LONGS_EQUAL(FWU_STATUS_UNKNOWN, status);
}

TEST(FwuInvalidBehaviourTests, invalidOperationsInStaging)
{
	int status = 0;
	struct uuid_octets uuid;

	/* Construct and boot a DUT with a 3 fw locations and with
	 * the policy not to allow partial updates. This means that
	 * an incoming update must include image updates for all
	 * locations.
	 *
	 * Assume an initial transition to the REGULAR FWU state.
	 */
	m_dut = fwu_dut_factory::create(3, false);
	m_fwu_client = m_dut->create_fwu_client();
	m_dut->boot();

	/* Expect to be able to transition to STAGING */
	status = m_fwu_client->begin_staging(0, 0, NULL);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	/* And re-enter STAGING (implicit cancel) */
	status = m_fwu_client->begin_staging(0, 0, NULL);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	/* Opening a couple of streams for installing images associated
	 * with two of the three updatable locations.
	 */
	uint32_t stream_handle1 = 0;
	uint32_t stream_handle2 = 0;

	m_dut->whole_volume_image_type_uuid(0, &uuid);
	status = m_fwu_client->open(&uuid, fwu_client::op_type::WRITE, &stream_handle1);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	m_dut->whole_volume_image_type_uuid(1, &uuid);
	status = m_fwu_client->open(&uuid, fwu_client::op_type::WRITE, &stream_handle2);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	/* Attempting to end staging with open install streams should fail */
	status = m_fwu_client->end_staging();
	LONGS_EQUAL(FWU_STATUS_BUSY, status);

	/* Commit without actually writing any image data. This would occur
	 * if an incoming update package contained zero length images. This
	 * could be interpreted as an image delete. As a delete operation
	 * is not supported by the raw_installer used by the DUT, expect
	 * the commit operations to fail.
	 */
	status = m_fwu_client->commit(stream_handle1, false);
	LONGS_EQUAL(FWU_STATUS_NOT_AVAILABLE, status);
	status = m_fwu_client->commit(stream_handle2, false);
	LONGS_EQUAL(FWU_STATUS_NOT_AVAILABLE, status);

	/* Expect accepting images to be denied while STAGING */
	m_dut->whole_volume_image_type_uuid(0, &uuid);
	status = m_fwu_client->accept(&uuid);
	LONGS_EQUAL(FWU_STATUS_DENIED, status);

	m_dut->whole_volume_image_type_uuid(1, &uuid);
	status = m_fwu_client->accept(&uuid);
	LONGS_EQUAL(FWU_STATUS_DENIED, status);

	/* Expect rolling back to previous version to also be denied while STAGING */
	status = m_fwu_client->select_previous();
	LONGS_EQUAL(FWU_STATUS_DENIED, status);

	/* Attempting to end staging when errors occurred during the installation
	 * process should fail.
	 */
	status = m_fwu_client->end_staging();
	LONGS_EQUAL(FWU_STATUS_NOT_AVAILABLE, status);

	/* Because the end_staging failed, expect to be still in STAGING.
	 * A call to cancel_staging is required to force a transition back to
	 * REGULAR.
	 */
	status = m_fwu_client->cancel_staging();
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	/* Cancelling again though should fail. */
	status = m_fwu_client->cancel_staging();
	LONGS_EQUAL(FWU_STATUS_DENIED, status);
}

TEST(FwuInvalidBehaviourTests, invalidOperationsInTrial)
{
	int status = 0;
	struct uuid_octets uuid;

	/* Construct and boot a DUT with a 3 fw locations and with
	 * the policy to allow partial updates. With this policy
	 * not all locations need to be updated for a viable update.
	 */
	m_dut = fwu_dut_factory::create(3, true);
	m_fwu_client = m_dut->create_fwu_client();
	m_dut->boot();

	/* Expect to be able to transition to STAGING */
	status = m_fwu_client->begin_staging(0, 0, NULL);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	/* Install an image into location 0 */
	uint32_t stream_handle = 0;

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

	/* Although only one location was updated, the DUT accepts partial
	 * updates so ending staging should be happy.
	 */
	status = m_fwu_client->end_staging();
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	/* If we've transitioned to TRAIL, attempting to begin staging
	 * again should be denied.
	 */
	status = m_fwu_client->begin_staging(0, 0, NULL);
	LONGS_EQUAL(FWU_STATUS_DENIED, status);

	/* Activate the update. We'd expect the update to have been installed
	 * in a different bank from the boot bank.
	 */
	m_dut->shutdown();
	m_dut->boot();

	/* If all's well, the DUT should have rebooted to TRIAL. Confirm this by
	 * trying to begin staging - this should be denied.
	 */
	status = m_fwu_client->begin_staging(0, 0, NULL);
	LONGS_EQUAL(FWU_STATUS_DENIED, status);

	/* All other staging related operations should also be denied */
	status = m_fwu_client->end_staging();
	LONGS_EQUAL(FWU_STATUS_DENIED, status);

	status = m_fwu_client->cancel_staging();
	LONGS_EQUAL(FWU_STATUS_DENIED, status);

	/* Attempting to install images should also be denied */
	m_dut->whole_volume_image_type_uuid(0, &uuid);
	status = m_fwu_client->open(&uuid, fwu_client::op_type::WRITE, &stream_handle);
	LONGS_EQUAL(FWU_STATUS_DENIED, status);

	/* Reading the image directory should be ok though */
	status = m_dir_checker->fetch_image_directory(m_fwu_client);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	/* Reverting to the previous version should be ok */
	status = m_fwu_client->select_previous();
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	/* Should have transitioned back to REGULAR */
	status = m_fwu_client->begin_staging(0, 0, NULL);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	status = m_fwu_client->begin_staging(0, 0, NULL);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	status = m_fwu_client->cancel_staging();
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	m_dut->shutdown();
}
