#-------------------------------------------------------------------------------
# Copyright (c) 2020-2023, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
set(TS_PLATFORM "arm/fvp/fvp_base_revc-2xaemv8a" CACHE STRING "Target platform location.")
include(../../../deployment.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  The CMakeLists.txt for building the internal-trusted-storage deployment for opteesp
#
#  Builds the secure storage service provider for running in an SEL0 secure
#  partition hosted by OPTEE in the role of SPM.
#-------------------------------------------------------------------------------
include(${TS_ROOT}/environments/opteesp/env.cmake)
project(trusted-services LANGUAGES C ASM)
add_executable(internal-trusted-storage)
target_include_directories(internal-trusted-storage PRIVATE "${TOP_LEVEL_INCLUDE_DIRS}")
set(SP_BIN_UUID_CANON "dc1eef48-b17a-4ccf-ac8b-dfcff7711b14")
set(SP_FFA_UUID_CANON "${TS_RPC_UUID_CANON}")

set(SP_HEAP_SIZE "32 * 1024" CACHE STRING "SP heap size in bytes")
set(SP_BOOT_ORDER "2" CACHE STRING "Boot order of the SP")
set(TRACE_PREFIX "ITS" CACHE STRING "Trace prefix")

target_include_directories(internal-trusted-storage PRIVATE
	${CMAKE_CURRENT_LIST_DIR}
)

#-------------------------------------------------------------------------------
#  Set target platform to provide drivers needed by the deployment
#
#-------------------------------------------------------------------------------
add_platform(TARGET "internal-trusted-storage")

#-------------------------------------------------------------------------------
#  Add components - this deployment uses an infrastructure that provides
#  ram backed storage for SFS.
#
#-------------------------------------------------------------------------------

add_components(TARGET "internal-trusted-storage"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		environments/opteesp
)

include(../../env/commonsp/its_sp.cmake REQUIRED)
include(../../internal-trusted-storage.cmake REQUIRED)
include(../../infra/sfs-ram.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  Deployment specific build options
#
#-------------------------------------------------------------------------------
target_compile_definitions(internal-trusted-storage PRIVATE
	ARM64=1
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	target_compile_options(internal-trusted-storage PRIVATE
		-std=c99
	)

endif()

#-------------------------------------------------------------------------------
#  Deployment specific install options
#
#-------------------------------------------------------------------------------
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "location to install build output to." FORCE)
endif()
install(TARGETS internal-trusted-storage
			RUNTIME DESTINATION ${TS_ENV}/bin
		)

include(${TS_ROOT}/tools/cmake/common/ExportSp.cmake)
export_sp(
	SP_FFA_UUID_CANON ${SP_FFA_UUID_CANON}
	SP_BIN_UUID_CANON ${SP_BIN_UUID_CANON}
	SP_BOOT_ORDER ${SP_BOOT_ORDER}
	SP_NAME "internal-trusted-storage"
	MK_IN ${TS_ROOT}/environments/opteesp/sp.mk.in
	DTS_IN ${CMAKE_CURRENT_LIST_DIR}/default_internal-trusted-storage.dts.in
	JSON_IN ${TS_ROOT}/environments/opteesp/sp_pkg.json.in
)
