/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.
 */

#ifndef LIBSP_INCLUDE_FFA_API_TYPES_H_
#define LIBSP_INCLUDE_FFA_API_TYPES_H_

#include "compiler.h"
#include "ffa_api_defines.h"
#include <stddef.h>
#include <stdint.h>

/**
 * Boot info
 */

/**
 * @brief Boot protocol name-value pairs
 */
struct ffa_name_value_pair_v1_0 {
	uint32_t name[4]; /**< Name of the item */
	uintptr_t value; /**< Value of the item */
	size_t size; /**< Size of the referenced value */
};

/**
 * @brief Structure for passing boot protocol data (FF-A v1.0)
 */
struct ffa_boot_info_v1_0 {
	uint32_t magic; /**< FF-A */
	uint32_t count; /**< Count of name value size pairs */
	struct ffa_name_value_pair_v1_0 nvp[]; /**< Array of name value size pairs */
};

struct ffa_boot_info_desc_v1_1 {
	uint8_t name[16]; /**< Name of boot information passed to the consumer */
	uint8_t type; /**< Type of boot information passed to the consumer */
	uint8_t reserved_mbz; /**< Reserved (MBZ) */;
	uint16_t flags; /**< Flags to describe properties of boot information in this descriptor */
	uint32_t size; /**< Size (in bytes) of boot info identified by Name and Type fields */
	uint64_t contents; /**< Value or address of boot info identified by Name and Type fields. */
} __packed;

/**
 * @brief Structure for passing boot protocol data (FF-A v1.1)
 */
struct ffa_boot_info_header_v1_1 {
	uint32_t signature; /**< 0x0ffa */
	uint32_t version; /**< FF-A version: bit[31]: MBZ, bit[30:16] major version number,
				bit[15:0]  minor version number */
	uint32_t size; /**< Size of boot information blob spanning contiguous memory */
	uint32_t desc_size; /**< Size of each boot information descriptor in the array */
	uint32_t desc_cnt; /**< Count of boot information descriptors in the array */
	uint32_t desc_offs; /**< Offset to array of boot information descriptors */
	uint64_t reserved_mbz; /**< Reserved (MBZ) */
} __packed;

union ffa_boot_info {
	uint32_t signature;
	struct ffa_boot_info_v1_0 boot_info_v1_0;
	struct ffa_boot_info_header_v1_1 boot_info_v1_1;
};

/**
 * @brief FF-A error status code type
 */
typedef int32_t ffa_result;

/**
 * FF-A features types
 */

/**
 * @brief Used to encode any optional features implemented or any implementation
 *        details exported by the queried interface.
 */
struct ffa_interface_properties {
	uint32_t interface_properties[2];
};

/**
 * Partition information types
 */

/**
 * @brief UUID descriptor structure
 */
struct ffa_uuid {
	uint8_t uuid[16];
};

/**
 * @brief Table 8.25: Partition information descriptor (FF-A v1.0)
 *        Table 13.37: Partition information descriptor (FF-A v1.1)
 */
struct ffa_partition_information {
	uint16_t partition_id;
	uint16_t execution_context_count;
	uint32_t partition_properties;
#if CFG_FFA_VERSION >= FFA_VERSION_1_1
	struct ffa_uuid uuid;
#endif
} __packed;

/**
 * Direct message type
 */

/**
 * @brief Direct message type
 */
struct ffa_direct_msg {
	uint32_t function_id;
	uint16_t source_id;
	uint16_t destination_id;
	union {
		uint32_t args32[5];
		uint64_t args64[5];
	} args;
};

/**
 * Memory management transaction types
 */

/**
 * @brief Table 5.14: Constituent memory region descriptor
 */
struct ffa_constituent_mem_region_desc {
	uint64_t address;
	uint32_t page_count;
	uint32_t reserved_mbz;
} __packed;

/**
 * @brief Table 5.13: Composite memory region descriptor
 */
struct ffa_composite_mem_region_desc {
	uint32_t total_page_count;
	uint32_t address_range_count;
	uint64_t reserved_mbz;
	struct ffa_constituent_mem_region_desc constituent_mem_region_desc[];
} __packed;

/**
 * @brief Table 5.15: Memory access permissions descriptor
 */
struct ffa_mem_access_perm_desc {
	uint16_t endpoint_id;
	uint8_t mem_access_permissions;
	uint8_t flags;
} __packed;

/**
 * @brief Table 5.16: Endpoint memory access descriptor
 */
struct ffa_mem_access_desc {
	struct ffa_mem_access_perm_desc mem_access_perm_desc;
	uint32_t composite_mem_region_desc_offset;
	uint64_t reserved_mbz;
} __packed;

#if CFG_FFA_VERSION == FFA_VERSION_1_0
/**
 * @brief FF-A v1.0 Table 5.19: Lend, donate or share memory transaction descriptor
 */
struct ffa_mem_transaction_desc {
	uint16_t sender_id;
	uint8_t mem_region_attr;
	uint8_t reserved_mbz0;
	uint32_t flags;
	uint64_t handle;
	uint64_t tag;
	uint32_t reserved_mbz1;
	uint32_t mem_access_desc_count;
	struct ffa_mem_access_desc mem_access_desc[];
} __packed;
#elif CFG_FFA_VERSION >= FFA_VERSION_1_1
/**
 * @brief FF-A v1.1 Table 10.20: Memory transaction descriptor
 */
struct ffa_mem_transaction_desc {
	uint16_t sender_id;
	uint16_t mem_region_attr;
	uint32_t flags;
	uint64_t handle;
	uint64_t tag;
	uint32_t mem_access_desc_size;
	uint32_t mem_access_desc_count;
	uint32_t mem_access_desc_offset;
	uint8_t reserved_mbz0[12];
} __packed;
#endif /* CFG_FFA_VERSION */

/**
 * @brief Table 11.25: Descriptor to relinquish a memory region
 */
struct ffa_mem_relinquish_desc {
	uint64_t handle;
	uint32_t flags;
	uint32_t endpoint_count;
	uint16_t endpoints[];
} __packed;

#endif /* LIBSP_INCLUDE_FFA_API_TYPES_H_ */
