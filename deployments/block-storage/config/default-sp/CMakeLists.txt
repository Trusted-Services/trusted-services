#-------------------------------------------------------------------------------
# Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

include(../../../deployment.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  The CMakeLists.txt for building the block-storage deployment for generic sp
#  environment.
#
#  Builds the block storage service provider for running in an SEL0 secure partition
#  hosted by any SPM.
#-------------------------------------------------------------------------------
include(${TS_ROOT}/environments/sp/env.cmake)
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type")
project(trusted-services LANGUAGES C ASM)
add_executable(block-storage)
target_include_directories(block-storage PRIVATE "${TOP_LEVEL_INCLUDE_DIRS}")

set(SP_NAME "block-storage")
set(SP_BIN_UUID_CANON "63646e80-eb52-462f-ac4f-8cdf3987519c")
set(SP_FFA_UUID_CANON "${TS_RPC_UUID_CANON}")
set(SP_HEAP_SIZE "120 * 1024" CACHE STRING "SP heap size in bytes")
set(SP_STACK_SIZE "64 * 1024" CACHE STRING "Stack size")
set(SP_BOOT_ORDER "1" CACHE STRING "Boot order of the SP")
set(TRACE_PREFIX "BLOCK" CACHE STRING "Trace prefix")

#-------------------------------------------------------------------------------
#  Deployment specific components. This deployment uses an infrastructure that
#  that provides ram-backed block stoarged, configured with storage partitions
#  that conform to the 'ref' scheme used for test.
#-------------------------------------------------------------------------------
add_components(TARGET "block-storage"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		environments/sp
)

include(../../env/commonsp/block_storage_sp.cmake REQUIRED)
include(../../block-storage.cmake REQUIRED)
include(../../infra/ref-ram.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  Deployment specific build options
#-------------------------------------------------------------------------------
target_compile_definitions(block-storage PRIVATE
	ARM64=1
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	target_compile_options(block-storage PRIVATE
		-std=c99
	)

endif()

compiler_generate_binary_output(TARGET block-storage NAME "${SP_BIN_UUID_CANON}.bin" SP_BINARY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SP_BIN_UUID_CANON}.bin DESTINATION ${TS_ENV}/bin)

include(${TS_ROOT}/tools/cmake/common/ExportMemoryRegionsToManifest.cmake REQUIRED)
export_memory_regions_to_manifest(TARGET block-storage NAME "${SP_BIN_UUID_CANON}_memory_regions.dtsi" RES EXPORT_MEMORY_REGIONS_DTSI)

#-------------------------------------------------------------------------------
#  Deployment specific install options
#-------------------------------------------------------------------------------
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "location to install build output to." FORCE)
endif()
install(TARGETS block-storage
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
