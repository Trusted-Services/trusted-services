#-------------------------------------------------------------------------------
# Copyright (c) 2021-2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Platform definition for the Corstone-1000 platform.
#-------------------------------------------------------------------------------

# Important: The order and index of values must match C code expectations
set(_corstone_1000_valid_types
    CORSTONE_1000_TYPE_CORTEX_A35_FVP
    CORSTONE_1000_TYPE_CORTEX_A35_MPS3
)
set(CORSTONE_1000_TYPE "CORSTONE_1000_TYPE_CORTEX_A35_FVP" CACHE STRING
    "Corstone-1000 platform type. Valid values are: ${_corstone_1000_valid_types}"
)
set_property(CACHE CORSTONE_1000_TYPE PROPERTY STRINGS ${_corstone_1000_valid_types})
list(FIND _corstone_1000_valid_types "${CORSTONE_1000_TYPE}" CORSTONE_1000_TYPE_INT)
if(CORSTONE_1000_TYPE_INT EQUAL -1)
    message(FATAL_ERROR
        "Invalid CORSTONE_1000_TYPE: ${CORSTONE_1000_TYPE}.
         Valid values are: ${_corstone_1000_valid_types}"
    )
endif()
unset(_corstone_1000_valid_types)

set(SMM_GATEWAY_MAX_UEFI_VARIABLES 80 CACHE STRING "Maximum UEFI variable count")
set(SMM_RPC_CALLER_SESSION_SHARED_MEMORY_SIZE 4*4096 CACHE STRING "RPC caller buffer size in SMMGW")
set(SMM_SP_HEAP_SIZE 80*1024 CACHE STRING "SMM gateway SP heap size")
set(PLAT_RSE_COMMS_PAYLOAD_MAX_SIZE 0x43C0 CACHE STRING "Size of the RSS_COMMS_PAYLOAD buffer")
set(COMMS_MHU_MSG_SIZE 0x4500 CACHE STRING "Max message size that can be transfered via MHU")
set(MM_COMM_BUFFER_ADDRESS "0x00000000 0x81FFF000" CACHE STRING "MM Communication buffer start address")
set(MM_COMM_BUFFER_PAGE_COUNT 0x1 CACHE STRING "MM Communication buffer page count")


# For ESRT v1 details : https://uefi.org/specs/UEFI/2.9_A/23_Firmware_Update_and_Reporting.html#efi-system-resource-table
# `FWU_IMAGE_CAPSULE_COUNT` has to match the number of images declared in corstone_config.h. Note that this does not include the ESRT image.
# Maximum number of FWU Images in a capsule to be updated
set(FWU_IMAGE_CAPSULE_COUNT 4)
# Size of one ESRT v1 Image entry structure object
set(ESRT_IMAGE_ENTRY_SIZE 40)
# Size of remaining fields of ESRT v1 table structure
set(ESRT_REMAINING_FIELDS_SIZE 16)
math(EXPR TOTAL_ESRT_SIZE "${FWU_IMAGE_CAPSULE_COUNT} * ${ESRT_IMAGE_ENTRY_SIZE} + ${ESRT_REMAINING_FIELDS_SIZE}" OUTPUT_FORMAT DECIMAL)

set(TFM_FWU_MAX_DIGEST_SIZE ${TOTAL_ESRT_SIZE} CACHE STRING "Maximum size of ESRT entries of all the images in a bank")

target_compile_definitions(${TGT} PRIVATE
	PLAT_RSE_COMMS_PAYLOAD_MAX_SIZE=${PLAT_RSE_COMMS_PAYLOAD_MAX_SIZE}
	COMMS_MHU_MSG_SIZE=${COMMS_MHU_MSG_SIZE}
	MBEDTLS_ECP_DP_SECP521R1_ENABLED
	CORSTONE_1000_TYPE=${CORSTONE_1000_TYPE_INT}
	TFM_FWU_MAX_DIGEST_SIZE=${TFM_FWU_MAX_DIGEST_SIZE}
)

get_property(_platform_driver_dependencies TARGET ${TGT}
	PROPERTY TS_PLATFORM_DRIVER_DEPENDENCIES
)

#-------------------------------------------------------------------------------
#  Map platform dependencies to suitable drivers for this platform
#
#-------------------------------------------------------------------------------
if ("mhu" IN_LIST _platform_driver_dependencies)
	include(${TS_ROOT}/platform/drivers/arm/mhu_driver/mhu_v2_x/driver.cmake)
endif()
