#-------------------------------------------------------------------------------
# Copyright (c) 2022-2023, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

#-------------------------------------------------------------------------------
# Set default platform. TS_PLATFORM should be set externally to build for
# an alternative platform from the default one.
#
#-------------------------------------------------------------------------------
set(TS_PLATFORM "arm/n1sdp" CACHE STRING "Target platform location.")
include(../../../deployment.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  The CMakeLists.txt for building the block_storage service for opteesp on a
#  platform with a dedicated secure flash device with driver support from the
#  EDK2 project. Platform drivers that the deployment depends on are fetched
#  from the edk2-platforms upstream repo and built into the SP image genereated
#  by the build process.
#
#  NOTE: In its current state, this configuration doesn't yet build successfully
#  as problems such as resolving EDK2 header file dependencies still need to
#  be resolved.
#-------------------------------------------------------------------------------
include(${TS_ROOT}/environments/opteesp/env.cmake)
project(trusted-services LANGUAGES C ASM)
add_executable(block-storage)
target_include_directories(block-storage PRIVATE "${TOP_LEVEL_INCLUDE_DIRS}")
set(SP_BIN_UUID_CANON "63646e80-eb52-462f-ac4f-8cdf3987519c")
set(SP_FFA_UUID_CANON "${TS_RPC_UUID_CANON}")
set(SP_HEAP_SIZE "120 * 1024" CACHE STRING "SP heap size in bytes")
set(SP_BOOT_ORDER "1" CACHE STRING "Boot order of the SP")
set(TRACE_PREFIX "BLOCK" CACHE STRING "Trace prefix")

target_include_directories(block-storage PRIVATE
	${CMAKE_CURRENT_LIST_DIR}
)

#-------------------------------------------------------------------------------
#  Deployment specific components.
#-------------------------------------------------------------------------------

add_components(TARGET "block-storage"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"environments/opteesp"
)

include(../../env/commonsp/block_storage_sp.cmake REQUIRED)
include(../../block-storage.cmake REQUIRED)

# Replace with suitable infrastructure when edk2 driver building in TS is supported
include(../../infra/ref-ram.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  External project dependencies
#
#-------------------------------------------------------------------------------
include(${TS_ROOT}/external/edk2_platforms/morello.cmake)

#-------------------------------------------------------------------------------
#  Set target platform to provide drivers needed by the deployment
#
#-------------------------------------------------------------------------------
add_platform(TARGET "block-storage")

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

#-------------------------------------------------------------------------------
#  Deployment specific install options
#-------------------------------------------------------------------------------
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "location to install build output to." FORCE)
endif()

install(TARGETS block-storage
			RUNTIME DESTINATION ${TS_ENV}/bin
		)

include(${TS_ROOT}/tools/cmake/common/ExportSp.cmake)
export_sp(
	SP_FFA_UUID_CANON ${SP_FFA_UUID_CANON}
	SP_BIN_UUID_CANON ${SP_BIN_UUID_CANON}
	SP_BOOT_ORDER ${SP_BOOT_ORDER}
	SP_NAME "block-storage"
	MK_IN ${TS_ROOT}/environments/opteesp/sp.mk.in
	DTS_IN ${CMAKE_CURRENT_LIST_DIR}/default_block-storage.dts.in
	JSON_IN ${TS_ROOT}/environments/opteesp/sp_pkg.json.in
)
