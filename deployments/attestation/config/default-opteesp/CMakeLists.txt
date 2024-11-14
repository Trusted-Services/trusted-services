#-------------------------------------------------------------------------------
# Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

# Set default platform.
set(TS_PLATFORM "arm/fvp/fvp_base_revc-2xaemv8a" CACHE STRING "Target platform location.")
include(../../../deployment.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  The CMakeLists.txt for building the attestation deployment for opteesp
#
#  Builds the attestation service provider for running in an SEL0 secure partition
#  hosted by OPTEE in the role of SPM.
#-------------------------------------------------------------------------------
include(${TS_ROOT}/environments/opteesp/env.cmake)
project(trusted-services LANGUAGES C ASM)
add_executable(attestation)
target_include_directories(attestation PRIVATE "${TOP_LEVEL_INCLUDE_DIRS}")
set(SP_BIN_UUID_CANON "a1baf155-8876-4695-8f7c-54955e8db974")
set(SP_FFA_UUID_CANON "${TS_RPC_UUID_CANON}")
set(SP_HEAP_SIZE "32 * 1024" CACHE STRING "SP heap size in bytes")
set(SP_BOOT_ORDER "5" CACHE STRING "Boot order of the SP")
set(TRACE_PREFIX "ATT" CACHE STRING "Trace prefix")

target_include_directories(attestation PRIVATE
	${CMAKE_CURRENT_LIST_DIR}
)

#-------------------------------------------------------------------------------
#  Default deployment specific configuration
#
#-------------------------------------------------------------------------------
set(TS_NO_FLOAT_HW ON)

#-------------------------------------------------------------------------------
#  Deployment specific components
#
#-------------------------------------------------------------------------------

add_components(TARGET "attestation"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"environments/opteesp"
)

include(../../env/commonsp/attestation_sp.cmake REQUIRED)
include(../../infra/tpm-eventlog-psa.cmake REQUIRED)
include(../../attestation.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  Set target platform to provide drivers needed by the deployment
#
#-------------------------------------------------------------------------------
add_platform(TARGET "attestation")

#-------------------------------------------------------------------------------
#  Deployment specific build options
#-------------------------------------------------------------------------------
target_compile_definitions(attestation PRIVATE
	ARM64=1
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	target_compile_options(attestation PRIVATE
		-std=c99
	)

endif()

#-------------------------------------------------------------------------------
#  Deployment specific install options
#-------------------------------------------------------------------------------
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "location to install build output to." FORCE)
endif()
#TODO: api headers

install(TARGETS attestation
			RUNTIME DESTINATION ${TS_ENV}/bin
		)

get_property(_PROTO_FILES TARGET attestation PROPERTY PROTOBUF_FILES)
install(FILES ${_PROTO_FILES} DESTINATION ${TS_ENV}/lib/protobuf)

include(${TS_ROOT}/tools/cmake/common/ExportSp.cmake)
export_sp(
	SP_FFA_UUID_CANON ${SP_FFA_UUID_CANON}
	SP_BIN_UUID_CANON ${SP_BIN_UUID_CANON}
	SP_BOOT_ORDER ${SP_BOOT_ORDER}
	SP_NAME "attestation"
	MK_IN ${TS_ROOT}/environments/opteesp/sp.mk.in
	DTS_IN ${CMAKE_CURRENT_LIST_DIR}/default_attestation.dts.in
	JSON_IN ${TS_ROOT}/environments/opteesp/sp_pkg.json.in
)
