#-------------------------------------------------------------------------------
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

# Set default platform.
set(TS_PLATFORM "arm/automotive-rd/rdaspen" CACHE STRING "Target platform location")
include(../../../deployment.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  The CMakeLists.txt for building the se-proxy deployment for RD-Aspen
#  using the "opteesp" environment.
#
#-------------------------------------------------------------------------------
include(${TS_ROOT}/environments/opteesp/env.cmake)
project(trusted-services LANGUAGES C ASM)
add_executable(se-proxy)
set(SP_BIN_UUID_CANON "46bb39d1-b4d9-45b5-88ff-040027dab249")
set(SP_FFA_UUID_CANON "${TS_RPC_UUID_CANON}")
set(CFG_FFA_VERSION "0x00010001")

set(SP_HEAP_SIZE "32 * 1024" CACHE STRING "SP heap size in bytes")
set(SP_BOOT_ORDER "1" CACHE STRING "Boot order of the SP")
set(TRACE_PREFIX "SEPROXY" CACHE STRING "Trace prefix")

# Setting the RSE communication carveout region parameters
set(RSE_COMM_CARVEOUT_NAME "rse-carveout" CACHE STRING "RSE communication carveout region name")
set(RSE_COMM_CARVEOUT_BASE "0x00000000 0xFFFFC000" CACHE STRING "RSE communication carveout physical address in DTS format")
set(RSE_COMM_CARVEOUT_PAGE_COUNT "4" CACHE STRING "Size of the RSE communication carveout region in 4k pages")

target_include_directories(
	se-proxy PRIVATE
	"${TOP_LEVEL_INCLUDE_DIRS}"
	${CMAKE_CURRENT_LIST_DIR}
)

#-------------------------------------------------------------------------------
#  Components that are specific to deployment in the opteesp environment.
#
#-------------------------------------------------------------------------------
add_components(
	TARGET "se-proxy"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"environments/opteesp"
)

option(RSE_POINTER_ACCESS "Enable RSE Pointer Access Protocol" ON)

include(../../env/commonsp/se_proxy_sp.cmake REQUIRED)
include(../../infra/automotive-rd/rdaspen/infra.cmake REQUIRED)
include(../../se-proxy.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  Set target platform to provide drivers needed by the deployment
#
#-------------------------------------------------------------------------------
add_platform(TARGET "se-proxy")

#-------------------------------------------------------------------------------
#  Deployment specific build options
#-------------------------------------------------------------------------------
target_compile_definitions(
	se-proxy PRIVATE
	ARM64=1
	RSE_COMM_CARVEOUT_NAME="${RSE_COMM_CARVEOUT_NAME}"
)

if(RSE_POINTER_ACCESS)
	target_compile_definitions(se-proxy PRIVATE RSE_POINTER_ACCESS)
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	target_compile_options(se-proxy PRIVATE -std=c99)
endif()

#-------------------------------------------------------------------------------
#  Deployment specific install options
#-------------------------------------------------------------------------------
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "location to install build output to" FORCE)
endif()

install(TARGETS se-proxy
	PUBLIC_HEADER DESTINATION ${TS_ENV}/include
	RUNTIME DESTINATION ${TS_ENV}/bin
)

get_property(_PROTO_FILES TARGET se-proxy PROPERTY PROTOBUF_FILES)
install(FILES ${_PROTO_FILES} DESTINATION ${TS_ENV}/lib/protobuf)

include(${TS_ROOT}/tools/cmake/common/ExportSp.cmake)
export_sp(
	SP_FFA_UUID_CANON ${SP_FFA_UUID_CANON}
	SP_BIN_UUID_CANON ${SP_BIN_UUID_CANON}
	SP_BOOT_ORDER ${SP_BOOT_ORDER}
	SP_NAME "se-proxy"
	MK_IN ${TS_ROOT}/environments/opteesp/sp.mk.in
	DTS_IN ${CMAKE_CURRENT_LIST_DIR}/se-proxy.dts.in
	JSON_IN ${TS_ROOT}/environments/opteesp/sp_pkg.json.in
)
