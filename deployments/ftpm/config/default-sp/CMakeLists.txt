#-------------------------------------------------------------------------------
# Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

# Set default platform.
set(TS_PLATFORM "arm/fvp/fvp_base_revc-2xaemv8a" CACHE STRING "Target platform location.")

# Set default build type to MinSizWithDebInfo to avoid running ouf of secure memory on low resource
# IoT platforms.
if (NOT DEFINED CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "MinSizWithDebInfo" CACHE STRING "Build type.")
endif()

include(../../../deployment.cmake REQUIRED)

include(${TS_ROOT}/environments/sp/env.cmake)
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type")
project(trusted-services LANGUAGES C ASM)
add_executable(ftpm)
target_include_directories(ftpm PRIVATE "${TOP_LEVEL_INCLUDE_DIRS}")

set(SP_NAME "ftpm")
set(SP_BIN_UUID_CANON "17b862a4-1806-4faf-86b3-089a58353861")
set(SP_FFA_UUID_CANON "${SP_BIN_UUID_CANON}")
set(SP_STACK_SIZE "64 * 1024" CACHE STRING "Stack size")
set(SP_HEAP_SIZE "128 * 1024" CACHE STRING "Heap size")
set(SP_BOOT_ORDER "10" CACHE STRING "Boot order of the SP")
set(TRACE_PREFIX "FTPM" CACHE STRING "Trace prefix")

set(TPM_CRB_NS_ADDRESS "0x84000000" CACHE STRING "Address of non-secure TPM CRB")
set(TPM_CRB_NS_PAGE_COUNT "4" CACHE STRING "Size of non-secure TPM CRB in 4k pages")
set(TPM_CRB_S_ADDRESS "0x07900000" CACHE STRING "Address of secure TPM CRB")
set(TPM_CRB_S_PAGE_COUNT "1" CACHE STRING "Size of secure TPM CRB in 4k pages")

# Convert to format used in the SP manifest DT
uint64_split(VALUE ${TPM_CRB_NS_ADDRESS} OUT_PREFIX CRB_NS_ADDR)
uint64_split(VALUE ${TPM_CRB_S_ADDRESS} OUT_PREFIX CRB_S_ADDR)

#-------------------------------------------------------------------------------
#  Set target platform to provide drivers needed by the deployment
#
#-------------------------------------------------------------------------------
add_platform(TARGET "ftpm")

add_components(TARGET "ftpm"
	BASE_DIR ${TS_ROOT}
	COMPONENTS environments/sp
)

include(../../env/commonsp/ftpm_sp.cmake REQUIRED)
include(../../ftpm.cmake REQUIRED)
include(../../infra/ms_tpm_backend.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  Deployment specific build options
#
#-------------------------------------------------------------------------------
target_compile_definitions(ftpm PRIVATE
	ARM64=1
	TPM_CRB_NS_PA=${TPM_CRB_NS_ADDRESS}
	TPM_CRB_S_PA=${TPM_CRB_S_ADDRESS}
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	target_compile_options(ftpm PRIVATE
		-std=c99
	)

endif()

compiler_generate_binary_output(TARGET ftpm NAME "${SP_BIN_UUID_CANON}.bin" SP_BINARY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SP_BIN_UUID_CANON}.bin DESTINATION ${TS_ENV}/bin)

include(${TS_ROOT}/tools/cmake/common/ExportMemoryRegionsToManifest.cmake REQUIRED)
export_memory_regions_to_manifest(TARGET ftpm NAME "${SP_BIN_UUID_CANON}_memory_regions.dtsi" RES EXPORT_MEMORY_REGIONS_DTSI)

#-------------------------------------------------------------------------------
#  Deployment specific install options
#
#-------------------------------------------------------------------------------
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "location to install build output to." FORCE)
endif()
install(TARGETS ftpm
	PUBLIC_HEADER DESTINATION ${TS_ENV}/include
	RUNTIME DESTINATION ${TS_ENV}/bin
)

include(${TS_ROOT}/tools/cmake/common/ExportSp.cmake REQUIRED)
export_sp(
	SP_FFA_UUID_CANON ${SP_FFA_UUID_CANON}
	SP_BIN_UUID_CANON ${SP_BIN_UUID_CANON}
	SP_BOOT_ORDER ${SP_BOOT_ORDER}
	SP_NAME ${SP_NAME}
	DTS_IN ${CMAKE_CURRENT_LIST_DIR}/default_${SP_NAME}.dts.in
	DTS_MEM_REGIONS ${SP_BIN_UUID_CANON}_memory_regions.dtsi
	JSON_IN ${TS_ROOT}/environments/sp/sp_pkg.json.in
)
