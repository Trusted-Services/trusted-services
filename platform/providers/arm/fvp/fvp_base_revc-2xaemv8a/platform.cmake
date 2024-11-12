#-------------------------------------------------------------------------------
# Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Platform definition for the 'fvp_base_revc-2xaem8a' virtual platform.
#-------------------------------------------------------------------------------
if (NOT DEFINED TGT)
	message(FATAL_ERROR "mandatory parameter TGT is not defined.")
endif()

get_property(_platform_driver_dependencies TARGET ${TGT}
	PROPERTY TS_PLATFORM_DRIVER_DEPENDENCIES
)

set(CFG_SFS_FLASH_AREA_SIZE "128*1024" CACHE STRING "Size of SFS ram store")
set(CFG_SFS_SECTORS_PER_BLOCK "4" CACHE STRING "Number of sectors per block in the SFS ram store")
set(CFG_SFS_MAX_ASSET_SIZE "16*1024" CACHE STRING "Max asset size of SFS ram store")

# Set the RPC buffer size of the storage client so the maximum supported asset size can be transmitted
set(CFG_STORAGE_CLIENT_RPC_BUF_SIZE "${CFG_SFS_MAX_ASSET_SIZE} + 4096" CACHE STRING "RPC buffer size of storage client")

# Test memory region base address for manifest testing in spm_test deployments.
set(CFG_TEST_MEM_REGION_ADDRESS  0x6248000 CACHE STRING "Base address of memory region used to test mainfest processing.")

#-------------------------------------------------------------------------------
#  Map platform dependencies to suitable drivers for this platform
#
#-------------------------------------------------------------------------------
if ("trng" IN_LIST _platform_driver_dependencies)
	include(${TS_ROOT}/platform/drivers/arm/juno_trng/driver.cmake)
endif()

if ("secure-nor-flash" IN_LIST _platform_driver_dependencies)
	include(${TS_ROOT}/platform/drivers/tf-a/drivers/cfi/v2m/v2m_flash.cmake)
endif()

if ("semihosting" IN_LIST _platform_driver_dependencies)
	include(${TS_ROOT}/platform/drivers/tf-a/lib/semihosting/driver.cmake)
endif()

if ("uart" IN_LIST _platform_driver_dependencies)
        include(${TS_ROOT}/platform/drivers/arm/uart/driver.cmake)
endif()