/*
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <CppUTest/TestHarness.h>
#include <cassert>
#include <cstring>
#include <vector>

#include "common/uuid/uuid.h"
#include "protocols/service/fwu/fwu_proto.h"
#include "protocols/service/fwu/status.h"
#include "service/fwu/test/fwu_dut/fwu_dut.h"
#include "service/fwu/test/fwu_dut_factory/fwu_dut_factory.h"

#define EFI_BUFFER_TOO_SMALL ((int32_t)5)
#define EFI_SUCCESS	     ((int32_t)0)

//*************************************************************
// EFI_FIRMWARE_IMAGE_DESCRIPTOR
//*************************************************************
typedef struct {
	uint8_t ImageIndex;
	struct uuid_octets ImageTypeId;
	uint64_t ImageId;
	char16_t *ImageIdName;
	uint32_t Version;
	char16_t *VersionName;
	uint64_t Size;
	uint64_t AttributesSupported;
	uint64_t AttributesSetting;
	uint64_t Compatibilities;
	//Introduced with DescriptorVersion 2+
	uint32_t LowestSupportedImageVersion;
	//Introduced with DescriptorVersion 3+
	uint32_t LastAttemptVersion;
	uint32_t LastAttemptStatus;
	uint64_t HardwareInstance;
} EFI_FIRMWARE_IMAGE_DESCRIPTOR;

struct fmp {
	explicit fmp(fwu_client *m_fwu_client)
		: client(m_fwu_client)
		, is_staging(false)
		, payload_max_size(0)
		, img_info(NULL)
		, num_images(0)

	{
		parse_img_directory();
	}

	~fmp()
	{
		if (img_info)
			delete[] img_info;
	}

	int get_image_info(uint64_t *ImageInfoSize, EFI_FIRMWARE_IMAGE_DESCRIPTOR *ImageInfo,
			   uint32_t *DescriptorVersion, uint8_t *DescriptorCount,
			   uint64_t *DescriptorSize, uint32_t *PackageVersion,
			   char16_t **PackageVersionName)
	{
		const uint64_t img_info_size = num_images * sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR);
		if (img_info_size > *ImageInfoSize) {
			*ImageInfoSize = img_info_size;

			return -EFI_BUFFER_TOO_SMALL;
		}

		memcpy(ImageInfo, img_info, *ImageInfoSize);

		*DescriptorVersion = 3;
		*DescriptorCount = num_images;
		*DescriptorSize = sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR);

		// PackageVersion 0xFFFFFFFF means that package version
		// is not supported. See UEFI specification.
		*PackageVersion = 0xFFFFFFFF;

		return EFI_SUCCESS;
	};

	int set_image(uint8_t ImageIndex, const void *Image, uint64_t ImageSize,
		      const void *VendorCode, void *Progress, char16_t **AbortReason)
	{
		int status = 0;
		uint32_t stream_handle = 0;

		struct uuid_octets *uuid = &(img_info[ImageIndex].ImageTypeId);

		if (!is_staging) {
			status = client->begin_staging(0, 0, NULL);
			LONGS_EQUAL(FWU_STATUS_SUCCESS, status);
			is_staging = true;
		}

		status = client->open(uuid, fwu_client::op_type::WRITE, &stream_handle);
		LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

		status = client->write_stream(stream_handle, static_cast<const uint8_t *>(Image),
					      ImageSize);
		LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

		status = client->commit(stream_handle, false);
		LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

		return 0;
	};

private:
	int parse_img_directory()
	{
		int status = 0;
		uint32_t stream_handle = 0;
		size_t reported_total_len = 0;
		struct uuid_octets uuid;

		struct fwu_image_directory *img_dir = NULL;

		size_t data_len_read = 0;
		int num_img = 0;

		assert(img_info == NULL);

		uuid_guid_octets_from_canonical(&uuid, FWU_DIRECTORY_CANONICAL_UUID);

		status = client->open(&uuid, fwu_client::op_type::READ, &stream_handle);
		LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

		// Determine the size of the FW directory without reading any info.
		status = client->read_stream(stream_handle, NULL, 0, &data_len_read,
					     &reported_total_len);
		LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

		// Close and reopen the firmware directory stream
		// to reset the read seek.
		status = client->commit(stream_handle, false);
		LONGS_EQUAL(FWU_STATUS_SUCCESS, status);
		status = client->open(&uuid, fwu_client::op_type::READ, &stream_handle);
		LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

		img_dir = (fwu_image_directory *)new uint8_t[reported_total_len];

		// Read the firmware directory info into img_dir.
		status = client->read_stream(stream_handle, reinterpret_cast<uint8_t *>(img_dir),
					     reported_total_len, &data_len_read,
					     &reported_total_len);
		LONGS_EQUAL(FWU_STATUS_SUCCESS, status);
		LONGS_EQUAL(data_len_read, reported_total_len);

		status = client->commit(stream_handle, false);
		LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

		num_img = img_dir->num_images;

		// Translate the data from each entry in the img directory into the img
		// info array.
		img_info = (EFI_FIRMWARE_IMAGE_DESCRIPTOR
				    *)new uint8_t[num_img * sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR)];

		for (int idx = 0; idx < num_img; idx++) {
			img_info[idx].ImageIndex = idx;
			memcpy(img_info[idx].ImageTypeId.octets,
			       img_dir->img_info_entry[idx].img_type_uuid, UUID_OCTETS_LEN);
			img_info[idx].ImageId = idx;
			img_info[idx].ImageIdName = NULL;
			img_info[idx].Version = img_dir->img_info_entry[idx].img_version;
			img_info[idx].VersionName = NULL;
			img_info[idx].Size = img_dir->img_info_entry[idx].img_max_size;
			img_info[idx].AttributesSupported = 0;
			img_info[idx].AttributesSetting = 0;
			img_info[idx].Compatibilities = 0;
			img_info[idx].LowestSupportedImageVersion = 0;
			img_info[idx].LastAttemptVersion = 0;
			img_info[idx].LastAttemptStatus = 0;
			img_info[idx].HardwareInstance = 0;
		}

		num_images = num_img;

		delete[] img_dir;
		return status;
	}

	fwu_client *client;
	bool is_staging;

	int payload_max_size;
	EFI_FIRMWARE_IMAGE_DESCRIPTOR *img_info;
	uint32_t num_images;
};

/*
 * Tests that perform a range of normal update scenarios
 */
TEST_GROUP(FwuUpdateFmpTests)
{
	void setup()
	{
		m_dut = NULL;
		m_fwu_client = NULL;
		m_fmp_protocol = NULL;
	}

	void teardown()
	{
		delete m_fwu_client;
		m_fwu_client = NULL;

		delete m_dut;
		m_dut = NULL;

		delete m_fmp_protocol;
		m_fmp_protocol = NULL;
	}

	fwu_dut *m_dut;
	fwu_client *m_fwu_client;
	fmp *m_fmp_protocol;
};

TEST(FwuUpdateFmpTests, wholeFmpFlow)
{
	struct uuid_octets uuid;

	uint64_t ImageInfoSize;
	uint32_t DescriptorVersion;
	uint8_t DescriptorCount;
	uint64_t DescriptorSize;
	uint32_t PackageVersion;
	int status;

	EFI_FIRMWARE_IMAGE_DESCRIPTOR *img_info = NULL;

	// Create a FW Store with two firmware images.
	m_dut = fwu_dut_factory::create(1, false);
	m_fwu_client = m_dut->create_fwu_client();

	assert(m_fwu_client != NULL);

	// Generate the images to be installed.
	std::vector<uint8_t> image_data;
	m_dut->generate_image_data(&image_data);

	// System start
	m_dut->boot();

	// Obtain the image_type_guid -- emulate the guid from the payload in an FMP capsule.
	m_dut->whole_volume_image_type_uuid(0, &uuid);

	m_fmp_protocol = new fmp(m_fwu_client);

	ImageInfoSize = 0;

	// First call to fmp.get_image_info obtains the required size.
	m_fmp_protocol->get_image_info(&ImageInfoSize, img_info, &DescriptorVersion,
				       &DescriptorCount, &DescriptorSize, &PackageVersion, NULL);

	img_info = (EFI_FIRMWARE_IMAGE_DESCRIPTOR *)new uint8_t[ImageInfoSize];

	m_fmp_protocol->get_image_info(&ImageInfoSize, img_info, &DescriptorVersion,
				       &DescriptorCount, &DescriptorSize, &PackageVersion, NULL);

	// Iterate over all the image descriptors returned by get_img_info
	// to obtain the ImageId.
	for (int idx = 0; idx < DescriptorCount; idx++) {
		if (uuid_is_equal(img_info[idx].ImageTypeId.octets, uuid.octets)) {
			uint8_t img_idx = img_info[idx].ImageIndex;

			m_fmp_protocol->set_image(img_idx, image_data.data(), image_data.size(),
						  NULL, NULL, NULL);
			break;
		}
	}

	status = m_fwu_client->end_staging();
	LONGS_EQUAL(FWU_STATUS_SUCCESS, status);

	delete[] img_info;
}
