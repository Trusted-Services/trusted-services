/*
 * Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <CppUTest/TestHarness.h>
#include <cstdint>
#include <vector>

#include "service/fwu/test/fwu_dut/fwu_dut.h"
#include "service/fwu/test/fwu_dut_factory/fwu_dut_factory.h"
#include "service/fwu/test/image_directory_checker/image_directory_checker.h"

/*
 * Tests that focus on retrieving and checking the contents of the image
 * directory returned by the update agent. The image directory is intended
 * to provide an up-to-date view of updatable firmware components.
 */
TEST_GROUP(FwuImageDirectoryTests)
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

	fwu_dut *m_dut;
	fwu_client *m_fwu_client;
};

TEST(FwuImageDirectoryTests, streamedReads)
{
	int status = 0;

	/* Construct and boot a DUT with a couple of fw locations */
	m_dut = fwu_dut_factory::create(2);
	m_fwu_client = m_dut->create_fwu_client();
	m_dut->boot();

	/* As a reference, fetch the image directory with the default buffer
	 * used by an image_directory_checker.
	 */
	image_directory_checker checker_a;

	status = checker_a.fetch_image_directory(m_fwu_client);
	LONGS_EQUAL(0, status);

	/* Construct another image_directory_checker and repeatedly
	 * fetch and compare the results for consistency.
	 */
	image_directory_checker checker_b;

	for (unsigned int i = 0; i < 100; ++i) {
		/* Try lots of reads */
		status = checker_b.fetch_image_directory(m_fwu_client);
		LONGS_EQUAL(0, status);

		/* Always expect the read data to be consistent */
		CHECK_TRUE(checker_a.is_contents_equal(checker_b));
	}
}

TEST(FwuImageDirectoryTests, streamRecycling)
{
	std::vector<uint32_t> stream_handles;
	struct uuid_octets uuid;

	uuid_guid_octets_from_canonical(&uuid, FWU_DIRECTORY_CANONICAL_UUID);

	/* Construct and boot a DUT with a couple of fw locations */
	m_dut = fwu_dut_factory::create(2);
	m_fwu_client = m_dut->create_fwu_client();
	m_dut->boot();

	/* Expect to be able to keep opening streams, beyond the capacity of
	 * the update_agent. The update_agent implements a least recently used
	 * recycling strategy for streams to defend against a denial-of-service
	 * attack where streams are opened but never closed.
	 */
	for (unsigned int i = 0; i < 200; ++i) {
		int status = 0;
		uint32_t stream_handle = 0;

		status = m_fwu_client->open(&uuid, fwu_client::op_type::WRITE, &stream_handle);
		LONGS_EQUAL(0, status);

		stream_handles.push_back(stream_handle);
	}

	/* Only the most recently opened streams should still be opened. Expect
	 * older ones to have been cancelled. Test this by closing streams in
	 * reverse chronological order.
	 */
	unsigned int successfully_closed_count = 0;
	unsigned int cancelled_count = 0;
	unsigned int stream_index = stream_handles.size();

	CHECK_TRUE(stream_index > 0);

	do {
		int status = 0;

		--stream_index;

		status = m_fwu_client->commit(stream_handles[stream_index], false);

		if (!status) {
			/* Operation successful so expect this to be a recently opened stream */
			LONGS_EQUAL(0, cancelled_count);
			++successfully_closed_count;
		} else {
			/* Operation failed so expect this to be an older stream */
			CHECK_TRUE(successfully_closed_count > 0);
			++cancelled_count;
		}
	} while (stream_index > 0);

	CHECK_TRUE(successfully_closed_count > 0);
	CHECK_TRUE(cancelled_count > 0);
}

TEST(FwuImageDirectoryTests, singleFwLocation)
{
	int status = 0;

	/* Construct and boot a DUT with a single fw location. This configuration
	 * is typical of a TF-A based device where all firmware is loaded from a
	 * FIP image. A/B banks are stored in separate disk partitions.
	 */
	m_dut = fwu_dut_factory::create(1);
	m_fwu_client = m_dut->create_fwu_client();
	m_dut->boot();

	image_directory_checker checker;

	status = checker.fetch_image_directory(m_fwu_client);
	LONGS_EQUAL(0, status);

	const struct fwu_image_directory *dir_header = checker.get_header();

	/* Expect directory header to reflect correct values */
	CHECK_TRUE(dir_header);
	UNSIGNED_LONGS_EQUAL(offsetof(struct fwu_image_directory, img_info_entry),
			     dir_header->img_info_offset);
	UNSIGNED_LONGS_EQUAL(sizeof(struct fwu_image_info_entry), dir_header->img_info_size);
	UNSIGNED_LONGS_EQUAL(2, dir_header->directory_version);
	UNSIGNED_LONGS_EQUAL(1, dir_header->correct_boot);
	UNSIGNED_LONGS_EQUAL(0, dir_header->reserved);
	CHECK_TRUE(dir_header->num_images >= 1);

	/* Expect an image entry for whole volume updates for location id zero */
	struct uuid_octets expected_img_type_uuid;

	m_dut->whole_volume_image_type_uuid(0, &expected_img_type_uuid);

	const struct fwu_image_info_entry *image_entry =
		checker.find_entry(&expected_img_type_uuid);

	CHECK_TRUE(image_entry);
}

TEST(FwuImageDirectoryTests, multipleFwLocations)
{
	int status = 0;
	unsigned int num_locations = 3;

	/* Construct and boot a DUT with multiple fw locations. This configuration
	 * will be typical of devices where firmware components distributed
	 * across multiple disk partitions.
	 */
	m_dut = fwu_dut_factory::create(num_locations);
	m_fwu_client = m_dut->create_fwu_client();
	m_dut->boot();

	image_directory_checker checker;

	status = checker.fetch_image_directory(m_fwu_client);
	LONGS_EQUAL(0, status);

	const struct fwu_image_directory *dir_header = checker.get_header();

	/* Expect directory header to reflect correct values */
	CHECK_TRUE(dir_header);
	UNSIGNED_LONGS_EQUAL(2, dir_header->directory_version);
	UNSIGNED_LONGS_EQUAL(1, dir_header->correct_boot);
	CHECK_TRUE(dir_header->num_images >= num_locations);

	for (unsigned int location_id = 0; location_id < num_locations; location_id++) {
		/* Expect an image entry for whole volume updates for each location */
		struct uuid_octets expected_img_type_uuid;

		m_dut->whole_volume_image_type_uuid(0, &expected_img_type_uuid);

		const struct fwu_image_info_entry *image_entry =
			checker.find_entry(&expected_img_type_uuid);

		CHECK_TRUE(image_entry);
	}
}

TEST(FwuImageDirectoryTests, zeroFwLocations)
{
	int status = 0;

	/* Construct and boot a DUT with no fw locations. This configuration
	 * will be typical of a device with no Swd accessible flash. With
	 * this sort of configuration, update images will be installed from
	 * Nwd.
	 */
	m_dut = fwu_dut_factory::create(0);
	m_fwu_client = m_dut->create_fwu_client();
	m_dut->boot();

	image_directory_checker checker;

	status = checker.fetch_image_directory(m_fwu_client);
	LONGS_EQUAL(0, status);

	const struct fwu_image_directory *dir_header = checker.get_header();

	/* Expect directory header to reflect correct values */
	CHECK_TRUE(dir_header);
	UNSIGNED_LONGS_EQUAL(2, dir_header->directory_version);
	UNSIGNED_LONGS_EQUAL(1, dir_header->correct_boot);

	/* The DUT uses a direct_fw_inspector to populate the fw directory.
	 * This relies on direct access to fw storage to determine what components
	 * should be reflected in the image directory. In practice, for a single
	 * flash system, an alternative fw inspector will be used.
	 */
	UNSIGNED_LONGS_EQUAL(0, dir_header->num_images);
}