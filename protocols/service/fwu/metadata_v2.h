/*
 * Copyright (c) 2023-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * FWU metadata information as per the specification section 4.1:
 * https://developer.arm.com/documentation/den0118/a/
 *
 */

#ifndef FWU_PROTO_METADATA_V2_H
#define FWU_PROTO_METADATA_V2_H

#include <stdint.h>

#include "metadata.h"
#include "protocols/common/osf/uuid.h"

/* Bank state definitions */
#define FWU_METADATA_V2_NUM_BANK_STATES	    (4)
#define FWU_METADATA_V2_BANK_STATE_INVALID  (0xff)
#define FWU_METADATA_V2_BANK_STATE_VALID    (0xfe)
#define FWU_METADATA_V2_BANK_STATE_ACCEPTED (0xfc)

/*
 * Version 2 FWU metadata data structure (mandatory)
 *
 * The metadata structure is variable length. The actual length is determined
 * from the metadata_size member.
 */
struct __attribute__((__packed__)) fwu_metadata {
	/* Metadata CRC value */
	uint32_t crc_32;

	/* Metadata version */
	uint32_t version;

	/* Active bank index as directed by update agent [0..n] */
	uint32_t active_index;

	/* Previous active bank index [0..n] */
	uint32_t previous_active_index;

	/* The overall metadata size */
	uint32_t metadata_size;

	/* The size in bytes of the fixed size header */
	uint16_t descriptor_offset;

	uint16_t reserved_16;

	/* Bank state bitmaps */
	uint8_t bank_state[FWU_METADATA_V2_NUM_BANK_STATES];

	uint32_t reserved_1c;
};

/* Properties of image in a bank */
struct __attribute__((__packed__)) fwu_img_bank_info {
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

	/* UUID of the storage volume where the image is located (e.g. a disk UUID) */
	uint8_t location_uuid[OSF_UUID_OCTET_LEN];

	/* Per-bank info related to the image */
	struct fwu_img_bank_info img_bank_info[FWU_METADATA_NUM_BANKS];
};

/*
 * Firmware store descriptor
 *
 * FWU metadata may optionally include a description of the firmware store
 * to direct the bootloader during boot. If a bootloader uses an alternative
 * method to determine where to boot from, the fw_store_desc structure is
 * not required. The fw_store_desc is assumed to be present if metadata_size
 * > header_size.
 */
struct __attribute__((__packed__)) fwu_fw_store_desc {
	/* Number of banks */
	uint8_t num_banks;

	uint8_t reserved_01;

	/* Number of images listed in the img_entry array */
	uint16_t num_images;

	/* The size of the img_entry data structure */
	uint16_t img_entry_size;

	/* The size of bytes of the bank_entry data structure */
	uint16_t bank_info_entry_size;

	/* Array of image_entry structures */
	struct fwu_image_entry img_entry[];
};

#endif /* FWU_PROTO_METADATA_V2_H */
