/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2020-2025, Arm Limited and Contributors. All rights reserved.
 */

#ifndef LIBSP_INCLUDE_FFA_API_DEFINES_H_
#define LIBSP_INCLUDE_FFA_API_DEFINES_H_

#include <stdint.h>
#include "util.h"

#define FFA_VERSION_1_0	(0x00010000)
#define FFA_VERSION_1_1	(0x00010001)

#ifndef CFG_FFA_VERSION
#error CFG_FFA_VERSION must be defined
#elif CFG_FFA_VERSION != FFA_VERSION_1_0 && CFG_FFA_VERSION != FFA_VERSION_1_1
#error Only FF-A versions 1.0 and 1.1 are supported
#endif

/* Status codes */
#define FFA_OK				(0)
#define FFA_NOT_SUPPORTED		(-1)
#define FFA_INVALID_PARAMETERS		(-2)
#define FFA_NO_MEMORY			(-3)
#define FFA_BUSY			(-4)
#define FFA_INTERRUPTED			(-5)
#define FFA_DENIED			(-6)
#define FFA_RETRY			(-7)
#define FFA_ABORTED			(-8)

/* Function IDs */
#define FFA_ERROR			UINT32_C(0x84000060)
#define FFA_SUCCESS_32			UINT32_C(0x84000061)
#define FFA_SUCCESS_64			UINT32_C(0xC4000061)
#define FFA_INTERRUPT			UINT32_C(0x84000062)
#define FFA_VERSION			UINT32_C(0x84000063)
#define FFA_FEATURES			UINT32_C(0x84000064)
#define FFA_RX_RELEASE			UINT32_C(0x84000065)
#define FFA_RXTX_MAP_32			UINT32_C(0x84000066)
#define FFA_RXTX_MAP_64			UINT32_C(0xC4000066)
#define FFA_RXTX_UNMAP			UINT32_C(0x84000067)
#define FFA_PARTITION_INFO_GET		UINT32_C(0x84000068)
#define FFA_ID_GET			UINT32_C(0x84000069)
#define FFA_MSG_WAIT			UINT32_C(0x8400006B)
#define FFA_YIELD			UINT32_C(0x8400006C)
#define FFA_RUN				UINT32_C(0x8400006D)
#define FFA_MSG_SEND			UINT32_C(0x8400006E)
#define FFA_MSG_SEND_DIRECT_REQ_32	UINT32_C(0x8400006F)
#define FFA_MSG_SEND_DIRECT_REQ_64	UINT32_C(0xC400006F)
#define FFA_MSG_SEND_DIRECT_RESP_32	UINT32_C(0x84000070)
#define FFA_MSG_SEND_DIRECT_RESP_64	UINT32_C(0xC4000070)
#define FFA_MSG_POLL			UINT32_C(0x8400006A)
#define FFA_MEM_DONATE_32		UINT32_C(0x84000071)
#define FFA_MEM_DONATE_64		UINT32_C(0xC4000071)
#define FFA_MEM_LEND_32			UINT32_C(0x84000072)
#define FFA_MEM_LEND_64			UINT32_C(0xC4000072)
#define FFA_MEM_SHARE_32		UINT32_C(0x84000073)
#define FFA_MEM_SHARE_64		UINT32_C(0xC4000073)
#define FFA_MEM_RETRIEVE_REQ_32		UINT32_C(0x84000074)
#define FFA_MEM_RETRIEVE_REQ_64		UINT32_C(0xC4000074)
#define FFA_MEM_RETRIEVE_RESP		UINT32_C(0x84000075)
#define FFA_MEM_RELINQUISH		UINT32_C(0x84000076)
#define FFA_MEM_RECLAIM			UINT32_C(0x84000077)
#define FFA_MEM_FRAG_RX			UINT32_C(0x8400007A)
#define FFA_MEM_FRAG_TX			UINT32_C(0x8400007B)
#define FFA_MEM_PERM_GET		UINT32_C(0x84000088)
#define FFA_MEM_PERM_SET		UINT32_C(0x84000089)
#define FFA_CONSOLE_LOG_32		UINT32_C(0x8400008A)
#define FFA_CONSOLE_LOG_64		UINT32_C(0xC400008A)

/* Utility macros */
#define FFA_TO_32_BIT_FUNC(x)		((x) & (~UINT32_C(0x40000000)))
#define FFA_TO_64_BIT_FUNC(x)		((x) | UINT32_C(0x40000000))
#define FFA_IS_32_BIT_FUNC(x)		(((x) & UINT32_C(0x40000000)) == 0)
#define FFA_IS_64_BIT_FUNC(x)		(((x) & UINT32_C(0x40000000)) != 0)

/* Special value for MBZ parameters */
#define FFA_PARAM_MBZ			UINT32_C(0x0)

/* Boot information signature */
#define FFA_BOOT_INFO_SIGNATURE_V1_0	UINT32_C(0x412D4646) /* FF-A ASCII */
#define FFA_BOOT_INFO_SIGNATURE_V1_1	UINT32_C(0x0FFA)

/* Boot information type */
#define FFA_BOOT_INFO_TYPE_SHIFT	UINT32_C(7)
#define FFA_BOOT_INFO_TYPE_MASK		UINT32_C(1)

#define FFA_BOOT_INFO_TYPE_STD		UINT32_C(0)
#define FFA_BOOT_INFO_TYPE_IMPDEF	UINT32_C(1)

/* Boot information ID */
#define FFA_BOOT_INFO_ID_SHIFT		UINT32_C(0)
#define FFA_BOOT_INFO_ID_MASK		GENMASK_32(6, 0)

#define FFA_BOOT_INFO_ID_STD_FDT	UINT32_C(0)
#define FFA_BOOT_INFO_ID_STD_HOB	UINT32_C(1)

/* Boot information flags */
#define FFA_BOOT_INFO_CONTENT_FMT_SHIFT	UINT32_C(2)
#define FFA_BOOT_INFO_CONTENT_FMT_MASK	GENMASK_32(1, 0)

#define FFA_BOOT_INFO_CONTENT_FMT_ADDR	UINT32_C(0)
#define FFA_BOOT_INFO_CONTENT_FMT_VAL	UINT32_C(1)

#define FFA_BOOT_INFO_NAME_FMT_SHIFT	UINT32_C(0)
#define FFA_BOOT_INFO_NAME_FMT_MASK	GENMASK_32(1, 0)

#define FFA_BOOT_INFO_NAME_FMT_STR	UINT32_C(0)
#define FFA_BOOT_INFO_NAME_FMT_UUID	UINT32_C(1)

/* FFA_VERSION */
#if CFG_FFA_VERSION == FFA_VERSION_1_0
#define FFA_VERSION_MAJOR		UINT32_C(1)
#define FFA_VERSION_MINOR		UINT32_C(0)
#elif CFG_FFA_VERSION == FFA_VERSION_1_1
#define FFA_VERSION_MAJOR		UINT32_C(1)
#define FFA_VERSION_MINOR		UINT32_C(1)
#endif /* CFG_FFA_VERSION */

#define FFA_VERSION_MAJOR_SHIFT		UINT32_C(16)
#define FFA_VERSION_MAJOR_MASK		GENMASK_32(14, 0)
#define FFA_VERSION_MINOR_SHIFT		UINT32_C(0)
#define FFA_VERSION_MINOR_MASK		GENMASK_32(15, 0)

/* FFA_FEATURES */

/* Features of FFA_RXTX_MAP */
#define FFA_FEATURES_RXTX_MAP_GRANULARITY_INDEX		UINT32_C(0)
#define FFA_FEATURES_RXTX_MAP_GRANULARITY_SHIFT		UINT32_C(0)
#define FFA_FEATURES_RXTX_MAP_GRANULARITY_MASK		GENMASK_32(1, 0)

#define FFA_FEATURES_RXTX_MAP_GRANULARITY_4K		UINT32_C(0x00)
#define FFA_FEATURES_RXTX_MAP_GRANULARITY_64K		UINT32_C(0x01)
#define FFA_FEATURES_RXTX_MAP_GRANULARITY_16K		UINT32_C(0x02)

/* Features of FFA_MEM_DONATE, FFA_MEM_LEND, FFA_MEM_SHARE */
#define FFA_FEATURES_MEM_DYNAMIC_BUFFER_SUPPORT_INDEX	UINT32_C(0)
#define FFA_FEATURES_MEM_DYNAMIC_BUFFER_SUPPORT		BIT32(0)

/* Features of FFA_MEM_RETRIEVE_REQ */
#define FFA_FEATURES_MEM_RETRIEVE_REQ_RETRIEVALS_INDEX	UINT32_C(0)
#define FFA_FEATURES_MEM_RETRIEVE_REQ_RETRIEVALS_SHIFT	UINT32_C(0)
#define FFA_FEATURES_MEM_RETRIEVE_REQ_RETRIEVALS_MASK	GENMASK_32(7, 0)

/* FFA_RXTX_MAP */
#define FFA_RXTX_MAP_PAGE_COUNT_SHIFT		UINT32_C(0)
#define FFA_RXTX_MAP_PAGE_COUNT_MASK		GENMASK_32(5, 0)
#define FFA_RXTX_MAP_PAGE_COUNT_MAX		FFA_RXTX_MAP_PAGE_COUNT_MASK
#define FFA_RXTX_MAP_PAGE_SIZE			UINT32_C(4096)

/* FFA_RXTX_UNMAP */
#define FFA_RXTX_UNMAP_ID_SHIFT				UINT32_C(16)
#define FFA_RXTX_UNMAP_ID_MASK				GENMASK_32(15, 0)

/* FFA_PARTITION_INFO_GET */
#if CFG_FFA_VERSION >= FFA_VERSION_1_1
#define FFA_PARTITION_INFO_GET_FLAG_COUNT_ONLY		BIT32(0)
#endif /* CFG_FFA_VERSION */

#define FFA_PARTITION_SUPPORTS_DIRECT_REQUESTS		BIT32(0)
#define FFA_PARTITION_CAN_SEND_DIRECT_REQUESTS		BIT32(1)
#define FFA_PARTITION_SUPPORTS_INDIRECT_REQUESTS	BIT32(2)
#if CFG_FFA_VERSION >= FFA_VERSION_1_1
#define FFA_PARTITION_SUPPORTS_NOTIFICATIONS		BIT32(3)

#define FFA_PARTITION_PART_ID_SHIFT			UINT32_C(4)
#define FFA_PARTITION_PART_ID_MASK			GENMASK_32(1, 0)
#define FFA_PARTITION_PART_ID_PE_ENDPOINT_ID		UINT16_C(0)
#define FFA_PARTITION_PART_ID_SEPID_INDEPENDENT		UINT16_C(1)
#define FFA_PARTITION_PART_ID_SEPID_DEPENDENT		UINT16_C(2)
#define FFA_PARTITION_PART_ID_PE_AUX_ID			UINT16_C(3)

#define FFA_PARTITION_INFORM_VM_CREATE			BIT32(6)
#define FFA_PARTITION_INFORM_VM_DESTROY			BIT32(7)

#define FFA_PARTITION_AARCH64_EXECUTION_STATE		BIT32(8)
#endif /* CFG_FFA_VERSION */

/* FFA_ID_GET */
#define FFA_ID_GET_ID_SHIFT				UINT32_C(0)
#define FFA_ID_GET_ID_MASK				GENMASK_32(15, 0)

/* FFA_MSG_SEND_DIRECT_REQ */
#define FFA_MSG_SEND_DIRECT_REQ_SOURCE_ID_MASK		GENMASK_32(15, 0)
#define FFA_MSG_SEND_DIRECT_REQ_SOURCE_ID_SHIFT		UINT32_C(16)

#define FFA_MSG_SEND_DIRECT_REQ_DEST_ID_MASK		GENMASK_32(15, 0)
#define FFA_MSG_SEND_DIRECT_REQ_DEST_ID_SHIFT		UINT32_C(0)

/* FFA_MSG_SEND_DIRECT_RESP */
#define FFA_MSG_SEND_DIRECT_RESP_SOURCE_ID_MASK		GENMASK_32(15, 0)
#define FFA_MSG_SEND_DIRECT_RESP_SOURCE_ID_SHIFT	UINT32_C(16)

#define FFA_MSG_SEND_DIRECT_RESP_DEST_ID_MASK		GENMASK_32(15, 0)
#define FFA_MSG_SEND_DIRECT_RESP_DEST_ID_SHIFT		UINT32_C(0)

/* FF-A direct message flags */
#define FFA_MSG_FLAG_FRAMEWORK				BIT32(31)
#define FFA_FRAMEWORK_MSG_TYPE_SHIFT			UINT32_C(0)
#define FFA_FRAMEWORK_MSG_TYPE_MASK			GENMASK_32(7, 0)
#define FFA_FRAMEWORK_MSG_PSCI				UINT32_C(0x0)
#define FFA_FRAMEWORK_MSG_VM_CREATED			UINT32_C(0x4)
#define FFA_FRAMEWORK_MSG_VM_CREATED_ACK		UINT32_C(0x5)
#define FFA_FRAMEWORK_MSG_VM_DESTROYED			UINT32_C(0x6)
#define FFA_FRAMEWORK_MSG_VM_DESTROYED_ACK		UINT32_C(0x7)
#define FFA_IS_FRAMEWORK_MSG(x)				(((x) & FFA_MSG_FLAG_FRAMEWORK) != 0)

/* Table 5.15: Memory access permissions descriptor */

/* Memory access permissions */
#define FFA_MEM_ACCESS_PERM_INSTRUCTION_MASK		GENMASK_32(1, 0)
#define FFA_MEM_ACCESS_PERM_INSTRUCTION_SHIFT		UINT32_C(2)

#define FFA_MEM_ACCESS_PERM_INSTRUCTION_NOT_SPECIFIED	UINT32_C(0x00)
#define FFA_MEM_ACCESS_PERM_INSTRUCTION_NOT_EXECUTABLE	UINT32_C(0x01)
#define FFA_MEM_ACCESS_PERM_INSTRUCTION_EXECUTABLE	UINT32_C(0x02)

#define FFA_MEM_ACCESS_PERM_DATA_MASK			GENMASK_32(1, 0)
#define FFA_MEM_ACCESS_PERM_DATA_SHIFT			UINT32_C(0)

#define FFA_MEM_ACCESS_PERM_DATA_NOT_SPECIFIED		UINT32_C(0x00)
#define FFA_MEM_ACCESS_PERM_DATA_READ_ONLY		UINT32_C(0x01)
#define FFA_MEM_ACCESS_PERM_DATA_READ_WRITE		UINT32_C(0x02)

#define FFA_MEM_ACCESS_PERM_FLAGS_NON_RETRIEVAL_BORROWER	BIT(0)

/* Table 5.18: Memory region attributes descriptor */

/* Memory type */
#define FFA_MEM_REGION_ATTR_MEMORY_TYPE_MASK		GENMASK_32(1, 0)
#define FFA_MEM_REGION_ATTR_MEMORY_TYPE_SHIFT		UINT32_C(4)

#if CFG_FFA_VERSION >= FFA_VERSION_1_1
#define FFA_MEM_REGION_ATTR_NS_BIT			BIT32(6)
#endif

#define FFA_MEM_REGION_ATTR_MEMORY_TYPE_NOT_SPECIFIED	UINT32_C(0x00)
#define FFA_MEM_REGION_ATTR_MEMORY_TYPE_DEVICE		UINT32_C(0x01)
#define FFA_MEM_REGION_ATTR_MEMORY_TYPE_NORMAL		UINT32_C(0x02)

/* Cacheability */
#define FFA_MEM_REGION_ATTR_CACHEABILITY_MASK		GENMASK_32(1, 0)
#define FFA_MEM_REGION_ATTR_CACHEABILITY_SHIFT		UINT32_C(2)

#define FFA_MEM_REGION_ATTR_CACHEABILITY_NON_CACHEABLE	UINT32_C(0x01)
#define FFA_MEM_REGION_ATTR_CACHEABILITY_WRITE_BACK	UINT32_C(0x03)

/* Device memory attributes */
#define FFA_MEM_REGION_ATTR_DEVICE_MEM_ATTR_MASK	GENMASK_32(1, 0)
#define FFA_MEM_REGION_ATTR_DEVICE_MEM_ATTR_SHIFT	UINT32_C(2)

#define FFA_MEM_REGION_ATTR_DEVICE_MEM_ATTR_NGNRNE	UINT32_C(0x00)
#define FFA_MEM_REGION_ATTR_DEVICE_MEM_ATTR_NGNRE	UINT32_C(0x01)
#define FFA_MEM_REGION_ATTR_DEVICE_MEM_ATTR_NGRE	UINT32_C(0x02)
#define FFA_MEM_REGION_ATTR_DEVICE_MEM_ATTR_GRE		UINT32_C(0x03)

/* Shareability */
#define FFA_MEM_REGION_ATTR_SHAREABILITY_MASK		GENMASK_32(1, 0)
#define FFA_MEM_REGION_ATTR_SHAREABILITY_SHIFT		UINT32_C(0)

#define FFA_MEM_REGION_ATTR_SHAREABILITY_NON_SHAREABLE		UINT32_C(0x00)
#define FFA_MEM_REGION_ATTR_SHAREABILITY_OUTER_SHAREABLE	UINT32_C(0x02)
#define FFA_MEM_REGION_ATTR_SHAREABILITY_INNER_SHAREABLE	UINT32_C(0x03)

/* Table 5.19: Lend, donate or share memory transaction descriptor */

#define FFA_MEM_TRANSACTION_PAGE_SIZE			UINT32_C(4096)
#define FFA_MEM_TRANSACTION_PAGE_MASK			GENMASK_64(11, 0)

/* Flags for donate, lend, share */

#define FFA_MEM_TRANSACTION_FLAGS_ZERO_MEMORY			BIT32(0)
#define FFA_MEM_TRANSACTION_FLAGS_OPERATION_TIME_SLICING	BIT32(1)

/* Flags for retrieve request */
#define FFA_MEM_TRANSACTION_FLAGS_ZERO_MEMORY_BEFORE_RETRIEVE	BIN32(0)
/* FFA_MEM_TRANSACTION_FLAGS_OPERATION_TIME_SLICING is available too */
#define FFA_MEM_TRANSACTION_FLAGS_ZERO_MEMORY_AFTER_RELINQIUSH	BIT32(2)

#define FFA_MEM_TRANSACTION_FLAGS_TYPE_MASK		GENMASK_32(1, 0)
#define FFA_MEM_TRANSACTION_FLAGS_TYPE_SHIFT		UINT32_C(3)

#define FFA_MEM_TRANSACTION_FLAGS_TYPE_RELAYER_SPECIFIES	UINT32_C(0x00)
#define FFA_MEM_TRANSACTION_FLAGS_TYPE_SHARE			UINT32_C(0x01)
#define FFA_MEM_TRANSACTION_FLAGS_TYPE_LEND			UINT32_C(0x02)
#define FFA_MEM_TRANSACTION_FLAGS_TYPE_DONATE			UINT32_C(0x03)

#define FFA_MEM_TRANSACTION_FLAGS_ALIGNMENT_HINT_MASK	GENMASK_32(3, 0)
#define FFA_MEM_TRANSACTION_FLAGS_ALIGNMENT_HINT_SHIFT	UINT32_C(5)

#define FFA_MEM_TRANSACTION_FLAGS_ALIGNMENT_HINT_VALID	BIT32(9)

/* Flags for retrieve response */
/* FFA_MEM_TRANSACTION_FLAGS_ZERO_MEMORY is available too */
/* FFA_MEM_TRANSACTION_FLAGS_TYPE_* is available too */

/* Handle */
#define FFA_MEM_HANDLE_INVALID				GENMASK_64(63, 0)

/* Table 11.25: Descriptor to relinquish a memory region */
#define FFA_RELINQUISH_FLAGS_ZERO_MEMORY_AFTER_RELINQUISH	BIT32(0)
#define FFA_RELINQUISH_FLAGS_OPERATION_TIME_SLICING		BIT32(1)

/* Flags for memory permission get/set */
#ifndef FFA_MEM_PERM_PAGE_SIZE
#define FFA_MEM_PERM_PAGE_SIZE				UINT32_C(4096)
#endif /* FFA_MEM_PERM_PAGE_SIZE */

#define FFA_MEM_PERM_PAGE_MASK				(FFA_MEM_PERM_PAGE_SIZE - 1)

#define FFA_MEM_PERM_PAGE_COUNT_MAX			GENMASK_32(31, 0)

#define FFA_MEM_PERM_DATA_ACCESS_PERM_INDEX		UINT32_C(2)
#define FFA_MEM_PERM_DATA_ACCESS_PERM_SHIFT		UINT32_C(0)
#define FFA_MEM_PERM_DATA_ACCESS_PERM_MASK		GENMASK_32(1, 0)

#define FFA_MEM_PERM_DATA_ACCESS_PERM_NO_ACCESS		UINT32_C(0x00)
#define FFA_MEM_PERM_DATA_ACCESS_PERM_RW		UINT32_C(0x01)
#define FFA_MEM_PERM_DATA_ACCESS_PERM_RESERVED		UINT32_C(0x02)
#define FFA_MEM_PERM_DATA_ACCESS_PERM_RO		UINT32_C(0x03)

#define FFA_MEM_PERM_INSTRUCTION_ACCESS_PERM_INDEX	UINT32_C(2)
#define FFA_MEM_PERM_INSTRUCTION_ACCESS_PERM_MASK	BIT(2)

#define FFA_MEM_PERM_INSTRUCTION_ACCESS_PERM_X		UINT32_C(0x00)
#define FFA_MEM_PERM_INSTRUCTION_ACCESS_PERM_NX		BIT(2)

#define FFA_MEM_PERM_RESERVED_MASK			GENMASK_32(31, 3)

/* FFA_CONSOLE_LOG */
#define FFA_CONSOLE_LOG_32_MAX_LENGTH			UINT32_C(24)
#define FFA_CONSOLE_LOG_64_MAX_LENGTH			UINT32_C(48)

#endif /* LIBSP_INCLUDE_FFA_API_DEFINES_H_ */
