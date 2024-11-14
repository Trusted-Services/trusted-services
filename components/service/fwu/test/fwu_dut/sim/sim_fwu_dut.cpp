/*
 * Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "sim_fwu_dut.h"

#include <CppUTest/TestHarness.h>
#include <cassert>
#include <cstring>
#include <sstream>

#include "common/endian/le.h"
#include "media/disk/guid.h"
#include "media/volume/index/volume_index.h"
#include "service/fwu/agent/update_agent.h"
#include "service/fwu/common/update_agent_interface.h"
#include "service/fwu/fw_store/banked/metadata_serializer/v1/metadata_serializer_v1.h"
#include "service/fwu/fw_store/banked/metadata_serializer/v2/metadata_serializer_v2.h"
#include "service/fwu/fw_store/banked/volume_id.h"
#include "service/fwu/inspector/direct/direct_fw_inspector.h"
#include "service/fwu/installer/installer_index.h"
#include "service/fwu/test/fwu_client/direct/direct_fwu_client.h"
#include "service/fwu/test/metadata_fetcher/volume/volume_metadata_fetcher.h"

sim_fwu_dut::sim_fwu_dut(unsigned int num_locations, unsigned int metadata_version,
			 bool allow_partial_updates)
	: fwu_dut(metadata_version)
	, m_is_booted(false)
	, m_is_first_boot(true)
	, m_boot_info()
	, m_metadata_checker(NULL)
	, m_num_locations(num_locations)
	, m_service_iface(NULL)
	, m_fw_flash()
	, m_partitioned_block_store()
	, m_block_store(NULL)
	, m_fw_volume_used_count(0)
	, m_fw_volume_pool()
	, m_raw_installer_used_count(0)
	, m_raw_installer_pool()
	, m_copy_installer_used_count(0)
	, m_copy_installer_pool()
	, m_update_agent()
	, m_fw_store()
	, m_fwu_provider()
{
	volume_index_init();
	installer_index_init();

	construct_storage(num_locations);
	construct_fw_volumes(num_locations);
	construct_installers(num_locations, allow_partial_updates);

	install_factory_images(num_locations);

	m_service_iface = fwu_provider_init(&m_fwu_provider, NULL);

	m_metadata_checker = create_metadata_checker();
}

sim_fwu_dut::~sim_fwu_dut()
{
	shutdown();

	delete m_metadata_checker;
	m_metadata_checker = NULL;

	fwu_provider_deinit(&m_fwu_provider);

	destroy_installers();
	destroy_fw_volumes();
	destroy_storage();

	installer_index_clear();
	volume_index_clear();
}

void sim_fwu_dut::boot(bool from_active_bank)
{
	if (m_is_booted)
		return;

	if (m_is_first_boot) {
		/* First boot where valid FWU metadata does not yet exist. */
		m_boot_info.boot_index = m_boot_info.active_index =
			m_boot_info.previous_active_index = FIRST_BOOT_BANK_INDEX;
		m_is_first_boot = false;

	} else {
		/* On subsequent boots, mimic the boot loader and derive boot
		 * info from the FWU metadata.
		 */
		m_metadata_checker->get_active_indices(&m_boot_info.active_index,
						       &m_boot_info.previous_active_index);

		m_boot_info.boot_index = (from_active_bank) ? m_boot_info.active_index :
							      m_boot_info.previous_active_index;
	}

	/* Now mimic boot loader image verification */
	verify_boot_images(m_boot_info.boot_index);

	/* Performs the generic update agent initialization that occurs on
	 * each system boot.
	 */
	int status = banked_fw_store_init(&m_fw_store, select_metadata_serializer());
	LONGS_EQUAL(0, status);

	m_update_agent = update_agent_init(m_boot_info.boot_index, direct_fw_inspector_inspect,
					   &m_fw_store);
	CHECK(m_update_agent != NULL);

	m_fwu_provider.update_agent = m_update_agent;

	m_is_booted = true;
}

void sim_fwu_dut::shutdown(void)
{
	if (!m_is_booted)
		return;

	/* Ensure all install streams are closed */
	update_agent_cancel_staging(m_update_agent);

	m_fwu_provider.update_agent = NULL;
	update_agent_deinit(m_update_agent);
	banked_fw_store_deinit(&m_fw_store);

	m_is_booted = false;
}

struct rpc_service_interface *sim_fwu_dut::get_service_interface(void)
{
	return m_service_iface;
}

struct boot_info sim_fwu_dut::get_boot_info(void) const
{
	return m_boot_info;
}

metadata_checker *sim_fwu_dut::create_metadata_checker(bool is_primary) const
{
	struct uuid_octets partition_guid;

	fwu_metadata_partition_guid(is_primary, &partition_guid);

	metadata_fetcher *metadata_fetcher =
		new volume_metadata_fetcher(&partition_guid, m_block_store);

	return fwu_dut::create_metadata_checker(metadata_fetcher, m_num_locations);
}

fwu_client *sim_fwu_dut::create_fwu_client(void)
{
	return new direct_fwu_client(&m_update_agent);
}

void sim_fwu_dut::fw_partition_guid(unsigned int location_index, unsigned int bank_index,
				    struct uuid_octets *uuid) const
{
	static const char *partition_guid[MAX_LOCATIONS][BANK_SCHEME_NUM_BANKS] = {
		{ "318757ec-82a0-48ce-a0d9-dfdeee6847a9", "3455a361-4074-42a3-ac0f-0e217c58494a" },
		{ "5feb0dff-3d4b-42ab-9635-c112cf641f2b", "d75c1efc-6c8c-458a-aa72-41810d1d8a99" },
		{ "0558ce63-db89-40ad-8039-1fafeb057fc8", "f07f1be5-077d-487f-9bd9-1ae33ed580e9" },
		{ "3b00979c-e776-4e79-b675-f78681b4cce3", "3de167ca-5e8c-4f05-9f87-e0c6a57538a5" }
	};

	CHECK_TRUE(location_index < MAX_LOCATIONS);
	CHECK_TRUE(bank_index < BANK_SCHEME_NUM_BANKS);

	uuid_guid_octets_from_canonical(uuid, partition_guid[location_index][bank_index]);
}

void sim_fwu_dut::fwu_metadata_partition_guid(bool is_primary, struct uuid_octets *uuid) const
{
	if (is_primary)
		uuid_guid_octets_from_canonical(uuid,
						DISK_GUID_UNIQUE_PARTITION_PRIMARY_FWU_METADATA);
	else
		uuid_guid_octets_from_canonical(uuid,
						DISK_GUID_UNIQUE_PARTITION_BACKUP_FWU_METADATA);
}

void sim_fwu_dut::disk_guid(struct uuid_octets *uuid) const
{
	uuid_guid_octets_from_canonical(uuid, "da92a93d-91d3-4b74-9102-7b45c21fe7db");
}

void sim_fwu_dut::construct_storage(unsigned int num_locations)
{
	size_t required_storage_blocks =
		METADATA_VOLUME_NUM_BLOCKS * 2 +
		FW_VOLUME_NUM_BLOCKS * BANK_SCHEME_NUM_BANKS * num_locations;

	struct uuid_octets flash_store_guid;

	disk_guid(&flash_store_guid);

	/* Construct the 'flash' */
	struct block_store *flash_store = ram_block_store_init(
		&m_fw_flash, &flash_store_guid, required_storage_blocks, FLASH_BLOCK_SIZE);

	/* Stack a partitioned_block_store over the flash */
	m_block_store = partitioned_block_store_init(&m_partitioned_block_store, 0,
						     &flash_store_guid, flash_store, NULL);

	/* Add all disk partitions */
	unsigned int lba = 0;
	struct uuid_octets partition_guid;

	/* First the primary fwu metadata partition */
	fwu_metadata_partition_guid(true, &partition_guid);
	bool is_added = partitioned_block_store_add_partition(&m_partitioned_block_store,
							      &partition_guid, lba,
							      lba + METADATA_VOLUME_NUM_BLOCKS - 1,
							      0, NULL);

	CHECK_TRUE(is_added);
	lba += METADATA_VOLUME_NUM_BLOCKS;

	/* Add partitions for each fw location */
	for (unsigned int location = 0; location < num_locations; location++) {
		for (unsigned int bank = 0; bank < BANK_SCHEME_NUM_BANKS; bank++) {
			fw_partition_guid(location, bank, &partition_guid);
			is_added = partitioned_block_store_add_partition(
				&m_partitioned_block_store, &partition_guid, lba,
				lba + FW_VOLUME_NUM_BLOCKS - 1, 0, NULL);

			CHECK_TRUE(is_added);
			lba += FW_VOLUME_NUM_BLOCKS;
		}
	}

	/* Finally, add the backup fwu metadata partition */
	fwu_metadata_partition_guid(false, &partition_guid);
	is_added = partitioned_block_store_add_partition(&m_partitioned_block_store,
							 &partition_guid, lba,
							 lba + METADATA_VOLUME_NUM_BLOCKS - 1, 0,
							 NULL);

	CHECK_TRUE(is_added);
}

void sim_fwu_dut::destroy_storage(void)
{
	partitioned_block_store_deinit(&m_partitioned_block_store);
	ram_block_store_deinit(&m_fw_flash);
}

void sim_fwu_dut::construct_fw_volumes(unsigned int num_locations)
{
	int status = 0;
	struct volume *volume = NULL;
	struct uuid_octets partition_guid;

	/* Construct volume for primary fwu metadata access */
	fwu_metadata_partition_guid(true, &partition_guid);

	status = block_volume_init(&m_fw_volume_pool[m_fw_volume_used_count], m_block_store,
				   &partition_guid, &volume);
	LONGS_EQUAL(0, status);
	CHECK_TRUE(volume);

	status = volume_index_add(BANKED_VOLUME_ID_PRIMARY_METADATA, volume);
	LONGS_EQUAL(0, status);
	++m_fw_volume_used_count;

	/* Construct volume for backup fwu metadata access */
	fwu_metadata_partition_guid(false, &partition_guid);

	status = block_volume_init(&m_fw_volume_pool[m_fw_volume_used_count], m_block_store,
				   &partition_guid, &volume);
	LONGS_EQUAL(0, status);
	CHECK_TRUE(volume);

	status = volume_index_add(BANKED_VOLUME_ID_BACKUP_METADATA, volume);
	LONGS_EQUAL(0, status);
	++m_fw_volume_used_count;

	/* Construct volumes for each fw storage partition */
	for (unsigned int location = 0; location < num_locations; location++) {
		for (unsigned int bank = 0; bank < BANK_SCHEME_NUM_BANKS; bank++) {
			fw_partition_guid(location, bank, &partition_guid);

			status = block_volume_init(&m_fw_volume_pool[m_fw_volume_used_count],
						   m_block_store, &partition_guid, &volume);
			LONGS_EQUAL(0, status);
			CHECK_TRUE(volume);

			status = volume_index_add(banked_volume_id(location, banked_usage_id(bank)),
						  volume);
			LONGS_EQUAL(0, status);
			++m_fw_volume_used_count;
		}
	}
}

void sim_fwu_dut::destroy_fw_volumes(void)
{
	for (unsigned int i = 0; i < m_fw_volume_used_count; i++)
		block_volume_deinit(&m_fw_volume_pool[i]);

	m_fw_volume_used_count = 0;
}

void sim_fwu_dut::construct_installers(unsigned int num_locations, bool allow_partial_updates)
{
	for (unsigned int location = 0; location < num_locations; location++) {
		/* Provides a raw and optional copy installer per location. The raw_installer
		 * is used for installing whole volume images using an externally streamed
		 * image while the copy installer is used to copy the previously good whole
		 * volume image to the update bank for cases where an incoming update
		 * did not include images for all locations. Use of a copy_installer to
		 * support this case is optional. By not registering a copy_installer for
		 * a location, an update attempt will fail if and image for the location
		 * was not included in an incoming update package.
		 */
		struct uuid_octets img_type_uuid;

		whole_volume_image_type_uuid(location, &img_type_uuid);

		struct raw_installer *raw_installer =
			&m_raw_installer_pool[m_raw_installer_used_count];

		raw_installer_init(raw_installer, &img_type_uuid, location);
		installer_index_register(&raw_installer->base_installer);
		++m_raw_installer_used_count;

		if (allow_partial_updates) {
			struct copy_installer *copy_installer =
				&m_copy_installer_pool[m_copy_installer_used_count];

			copy_installer_init(copy_installer, &img_type_uuid, location);
			installer_index_register(&copy_installer->base_installer);
			++m_copy_installer_used_count;
		}
	}
}

void sim_fwu_dut::destroy_installers(void)
{
	for (unsigned int i = 0; i < m_raw_installer_used_count; i++)
		raw_installer_deinit(&m_raw_installer_pool[i]);

	m_raw_installer_used_count = 0;

	for (unsigned int i = 0; i < m_copy_installer_used_count; i++)
		copy_installer_deinit(&m_copy_installer_pool[i]);

	m_copy_installer_used_count = 0;
}

void sim_fwu_dut::install_factory_images(unsigned int num_locations)
{
	/* Install valid images into bank 0 to mimic the state of
	 * a device with factory programmed flash.
	 */
	for (unsigned int location = 0; location < num_locations; location++) {
		struct volume *volume = NULL;
		size_t len_written = 0;

		int status = volume_index_find(
			banked_volume_id(location, banked_usage_id(FIRST_BOOT_BANK_INDEX)),
			&volume);
		LONGS_EQUAL(0, status);
		CHECK_TRUE(volume);

		std::vector<uint8_t> image_data;

		generate_image_data(&image_data);

		status = volume_open(volume);
		LONGS_EQUAL(0, status);

		status = volume_write(volume, (uintptr_t)image_data.data(), image_data.size(),
				      &len_written);
		LONGS_EQUAL(0, status);
		UNSIGNED_LONGS_EQUAL(image_data.size(), len_written);

		status = volume_close(volume);
		LONGS_EQUAL(0, status);
	}
}

void sim_fwu_dut::verify_boot_images(unsigned int boot_index)
{
	for (unsigned int location = 0; location < m_num_locations; location++) {
		struct volume *volume = NULL;

		int status = volume_index_find(
			banked_volume_id(location, banked_usage_id(boot_index)), &volume);
		LONGS_EQUAL(0, status);
		CHECK_TRUE(volume);

		status = volume_open(volume);
		LONGS_EQUAL(0, status);

		sim_fwu_dut::verify_image(volume);

		status = volume_close(volume);
		LONGS_EQUAL(0, status);
	}
}

const struct metadata_serializer *sim_fwu_dut::select_metadata_serializer(void) const
{
	unsigned int version = metadata_version();

	if (version == 1)
		return metadata_serializer_v1();

	if (version == 2)
		return metadata_serializer_v2();

	/* Metadata version not supported */
	assert(false);

	return NULL;
}

void sim_fwu_dut::verify_image(struct volume *volume)
{
	std::string fixed_header(VALID_IMAGE_HEADER);
	size_t header_len = fixed_header.size() + sizeof(uint32_t) + sizeof(uint32_t);

	/* Read image header */
	uint8_t header_buf[header_len];
	size_t total_bytes_read = 0;

	int status = volume_read(volume, (uintptr_t)header_buf, header_len, &total_bytes_read);
	LONGS_EQUAL(0, status);
	CHECK_TRUE(total_bytes_read == header_len);

	/* Verify header and extract values */
	MEMCMP_EQUAL(fixed_header.data(), header_buf, fixed_header.size());

	size_t image_size = load_u32_le(header_buf, fixed_header.size());
	uint32_t seq_num = load_u32_le(header_buf, fixed_header.size() + sizeof(uint32_t));

	CHECK_TRUE(image_size >= header_len);

	/* Read the remainder of the image and check data is as expected */
	uint8_t expected_fill_val = static_cast<uint8_t>(seq_num);

	while (total_bytes_read < image_size) {
		uint8_t read_buf[1024];
		size_t bytes_read = 0;
		size_t bytes_remaining = image_size - total_bytes_read;
		size_t bytes_to_read = (bytes_remaining > sizeof(read_buf)) ? sizeof(read_buf) :
									      bytes_remaining;

		status = volume_read(volume, (uintptr_t)read_buf, bytes_to_read, &bytes_read);
		LONGS_EQUAL(0, status);
		UNSIGNED_LONGS_EQUAL(bytes_to_read, bytes_read);

		for (size_t i = 0; i < bytes_read; i++)
			BYTES_EQUAL(expected_fill_val, read_buf[i]);

		total_bytes_read += bytes_read;
	}

	UNSIGNED_LONGS_EQUAL(image_size, total_bytes_read);
}
