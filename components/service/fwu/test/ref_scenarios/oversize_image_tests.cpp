/*
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
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
 * Tests that check behaviour when oversize images are installed.
 */
TEST_GROUP(FwuOversizeImageTests)
{
	void setup()
	{
		m_dut = NULL;
		m_fwu_client = NULL;
	}

	void teardown()
	{
		delete m_fwu_client;
		m_fwu_client = NULL;

		delete m_dut;
		m_dut = NULL;
	}

	size_t max_image_size(const struct uuid_octets *uuid)
	{
		image_directory_checker dir_checker;

		int status = dir_checker.fetch_image_directory(m_fwu_client);
		LONGS_EQUAL(0, status);

		const struct fwu_image_info_entry *img_entry = dir_checker.find_entry(uuid);
		CHECK_TRUE(img_entry);

		return static_cast<size_t>(img_entry->img_max_size);
	}

	fwu_dut *m_dut;
	fwu_client *m_fwu_client;
};

TEST(FwuOversizeImageTests, maxSizeInstall)
{
	int status = 0;
	struct uuid_octets uuid;
	uint32_t stream_handle = 0;

	/* Performs an update with an image of the maximum size advertised by
	 * the image directory.
	 */
	m_dut = fwu_dut_factory::create(1, false);
	m_fwu_client = m_dut->create_fwu_client();

	m_dut->boot();

	/* Generate image that should just fit */
	m_dut->whole_volume_image_type_uuid(0, &uuid);
	size_t img_size = max_image_size(&uuid);
	std::vector<uint8_t> image_data;
	m_dut->generate_image_data(&image_data, img_size);

	/* Install the image */
	status = m_fwu_client->begin_staging(0, 0, NULL);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	status = m_fwu_client->open(&uuid, fwu_client::op_type::WRITE, &stream_handle);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	status = m_fwu_client->write_stream(stream_handle,
					    reinterpret_cast<const uint8_t *>(image_data.data()),
					    image_data.size());
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	status = m_fwu_client->commit(stream_handle, false);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	status = m_fwu_client->end_staging();
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	/* Check for reboot with no errors */
	m_dut->shutdown();
	m_dut->boot();
}

TEST(FwuOversizeImageTests, oversizeInstallCancelStaging)
{
	int status = 0;
	struct uuid_octets uuid;
	uint32_t stream_handle = 0;

	/* Performs an update with an oversized image where the client
	 * cancels staging on seeing the error. This is the expected
	 * client behavior when an error occurs during installation.
	 */
	m_dut = fwu_dut_factory::create(1, false);
	m_fwu_client = m_dut->create_fwu_client();

	m_dut->boot();

	/* Generate image that's too big */
	m_dut->whole_volume_image_type_uuid(0, &uuid);
	size_t img_size = max_image_size(&uuid) + 1;
	std::vector<uint8_t> image_data;
	m_dut->generate_image_data(&image_data, img_size);

	/* Install the image */
	status = m_fwu_client->begin_staging(0, 0, NULL);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	status = m_fwu_client->open(&uuid, fwu_client::op_type::WRITE, &stream_handle);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	status = m_fwu_client->write_stream(stream_handle,
					    reinterpret_cast<const uint8_t *>(image_data.data()),
					    image_data.size());
	LONGS_EQUAL(FWU_STATUS_OUT_OF_BOUNDS, status);

	/* Client response to the error by cancelling staging */
	status = m_fwu_client->cancel_staging();
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	/* Check for reboot with no errors */
	m_dut->shutdown();
	m_dut->boot();
}

TEST(FwuOversizeImageTests, oversizeInstallEndStaging)
{
	int status = 0;
	struct uuid_octets uuid;
	uint32_t stream_handle = 0;

	/* Performs an update with an oversized image where the client
	 * attempts to proceed with the update beyond the point where an error
	 * was reported due to an attempt to install an oversized image.
	 */
	m_dut = fwu_dut_factory::create(1, false);
	m_fwu_client = m_dut->create_fwu_client();

	m_dut->boot();

	/* Generate image that's too big */
	m_dut->whole_volume_image_type_uuid(0, &uuid);
	size_t img_size = max_image_size(&uuid) + 1;
	std::vector<uint8_t> image_data;
	m_dut->generate_image_data(&image_data, img_size);

	/* Install the image */
	status = m_fwu_client->begin_staging(0, 0, NULL);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	status = m_fwu_client->open(&uuid, fwu_client::op_type::WRITE, &stream_handle);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	status = m_fwu_client->write_stream(stream_handle,
					    reinterpret_cast<const uint8_t *>(image_data.data()),
					    image_data.size());
	LONGS_EQUAL(FWU_STATUS_OUT_OF_BOUNDS, status);

	status = m_fwu_client->commit(stream_handle, false);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	/* Client has ignored the error and proceeded to end staging. Because
	 * the DUT was configured to not support partial updates, expect end_staging
	 * to fail.
	 */
	status = m_fwu_client->end_staging();
	LONGS_EQUAL(FWU_STATUS_NOT_AVAILABLE, status);

	/* Check for reboot with no errors */
	m_dut->shutdown();
	m_dut->boot();
}

TEST(FwuOversizeImageTests, oversizeInstallMultiLocationEndStaging)
{
	int status = 0;
	struct uuid_octets uuid;
	uint32_t stream_handle = 0;

	/* Performs an update with one oversized image when the DUT has
	 * multiple locations. This checks handling when some installs are
	 * successful but one fails because the image is too big. Because
	 * the DUT allows partial updates, if the client proceeds to
	 * finalize the update, any images handled by the errored installer
	 * will be ignored and treated as if they weren't updated.
	 */
	m_dut = fwu_dut_factory::create(3, true);
	m_fwu_client = m_dut->create_fwu_client();

	m_dut->boot();

	/* Perform multi-image update transaction */
	status = m_fwu_client->begin_staging(0, 0, NULL);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	/* Install good image for location 0 */
	std::vector<uint8_t> image_data;
	m_dut->whole_volume_image_type_uuid(0, &uuid);
	size_t img_size = max_image_size(&uuid);
	m_dut->generate_image_data(&image_data, img_size);

	status = m_fwu_client->open(&uuid, fwu_client::op_type::WRITE, &stream_handle);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	status = m_fwu_client->write_stream(stream_handle,
					    reinterpret_cast<const uint8_t *>(image_data.data()),
					    image_data.size());
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	status = m_fwu_client->commit(stream_handle, false);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	/* Install oversized image for location 1 */
	m_dut->whole_volume_image_type_uuid(1, &uuid);
	img_size = max_image_size(&uuid) + 1;
	m_dut->generate_image_data(&image_data, img_size);

	status = m_fwu_client->open(&uuid, fwu_client::op_type::WRITE, &stream_handle);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	status = m_fwu_client->write_stream(stream_handle,
					    reinterpret_cast<const uint8_t *>(image_data.data()),
					    image_data.size());
	LONGS_EQUAL(FWU_STATUS_OUT_OF_BOUNDS, status);

	status = m_fwu_client->commit(stream_handle, false);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	/* Install good image for location 2 */
	m_dut->whole_volume_image_type_uuid(2, &uuid);
	img_size = max_image_size(&uuid);
	m_dut->generate_image_data(&image_data, img_size);

	status = m_fwu_client->open(&uuid, fwu_client::op_type::WRITE, &stream_handle);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	status = m_fwu_client->write_stream(stream_handle,
					    reinterpret_cast<const uint8_t *>(image_data.data()),
					    image_data.size());
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	status = m_fwu_client->commit(stream_handle, false);
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	/* Expect end staging to be successful and that the errored image is
	 * excluded from the update. This should be confirmed by the following
	 * boot being error-free.
	 */
	status = m_fwu_client->end_staging();
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	/* Check for reboot with no errors */
	m_dut->shutdown();
	m_dut->boot();
}
