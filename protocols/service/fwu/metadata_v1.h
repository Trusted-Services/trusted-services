/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * FWU metadata information as per the specification section 4.1:
 * https://developer.arm.com/documentation/den0118/a/
 *
 */

#ifndef FWU_PROTO_METADATA_V1_H
#define FWU_PROTO_METADATA_V1_H

#include <stdint.h>

#include "metadata.h"
#include "protocols/common/osf/uuid.h"

/**
 * FWU metadata version corresponding to these structure definitions.
 */
#define FWU_METADATA_VERSION (1)

/**
 * The number of image entries in the metadata structure.
 */
#ifndef FWU_METADATA_NUM_IMAGE_ENTRIES
#define FWU_METADATA_NUM_IMAGE_ENTRIES (1)
#endif

/* Properties of image in a bank */
struct __attribute__((__packed__)) fwu_image_properties {
	/* UUID of the image in this bank */
	uint8_t img_uuid[OSF_UUID_OCTET_LEN];

	/* [0]: bit describing the image acceptance status –
	 *      1 means the image is accepted
	 * [31:1]: MBZ
	 */
	uint32_t accepted;

	/* reserved (MBZ) */
	uint32_t reserved;
};

/* Image entry information */
struct __attribute__((__packed__)) fwu_image_entry {
	/* UUID identifying the image type */
	uint8_t img_type_uuid[OSF_UUID_OCTET_LEN];

	/* UUID of the storage volume where the image is located */
	uint8_t location_uuid[OSF_UUID_OCTET_LEN];

	/* Properties of images with img_type_uuid in the different FW banks */
	struct fwu_image_properties img_props[FWU_METADATA_NUM_BANKS];
};

/*
 * FWU metadata filled by the updater and consumed by TF-A for
 * various purposes as below:
 * 1. Get active FW bank.
 * 2. Rollback to previous working FW bank.
 * 3. Get properties of all images present in all banks.
 */
struct __attribute__((__packed__)) fwu_metadata {
	/* Metadata CRC value */
	uint32_t crc_32;

	/* Metadata version */
	uint32_t version;

	/* Bank index with which device boots */
	uint32_t active_index;

	/* Previous bank index with which device booted successfully */
	uint32_t previous_active_index;

	/* Image entry information */
	struct fwu_image_entry img_entry[FWU_METADATA_NUM_IMAGE_ENTRIES];
};

#endif /* FWU_PROTO_METADATA_V1_H */
