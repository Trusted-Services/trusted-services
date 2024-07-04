/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
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
 * FWU service level tests that can be run with other service level tests.
 * Because these tests may be run on the device-under-test, update activation
 * scenarios that rely on a shutdown and restart are not included in tests.
 * These tests are mainly concerned with checking that the service provider
 * is reachable and is responding to requests made by a client.
 */
TEST_GROUP(FwuServiceTests)
{
	void setup()
	{
		m_dut = NULL;
		m_fwu_client = NULL;
		m_metadata_checker = NULL;

		m_dut = fwu_dut_factory::create(1, false);
		CHECK_TRUE(m_dut);
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

TEST(FwuServiceTests, checkImgDirAccess)
{
	m_fwu_client = m_dut->create_fwu_client();
	CHECK_TRUE(m_fwu_client);

	m_dut->boot();

	image_directory_checker img_dir_checker;

	int status = img_dir_checker.fetch_image_directory(m_fwu_client);
	LONGS_EQUAL(0, status);

	CHECK_TRUE(img_dir_checker.num_images() > 0);
}

TEST(FwuServiceTests, checkMetadataAccess)
{
	m_metadata_checker = m_dut->create_metadata_checker();
	CHECK_TRUE(m_metadata_checker);

	m_dut->boot();

	uint32_t active_index;
	uint32_t previous_active_index;

	m_metadata_checker->get_active_indices(&active_index, &previous_active_index);
	m_metadata_checker->check_regular(active_index);
}