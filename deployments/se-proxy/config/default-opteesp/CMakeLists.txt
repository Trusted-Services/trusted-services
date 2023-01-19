#-------------------------------------------------------------------------------
# Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

# Set default platform.
set(TS_PLATFORM "arm/fvp/fvp_base_revc-2xaemv8a" CACHE STRING "Target platform location.")
include(../../../deployment.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  The CMakeLists.txt for building the se-proxy deployment for opteesp
#
#  Builds proxy service providers that communicate with a separate secure element
#  that hosts a set of service endpoints.  This deployment is for running in an
#  SEL0 secure partition hosted by OPTEE in the role of SPM.
#-------------------------------------------------------------------------------
include(${TS_ROOT}/environments/opteesp/env.cmake)
project(trusted-services LANGUAGES C ASM)
add_executable(se-proxy)
target_include_directories(se-proxy PRIVATE "${TOP_LEVEL_INCLUDE_DIRS}")
set(SP_UUID_CANON "46bb39d1-b4d9-45b5-88ff-040027dab249")
set(SP_HEAP_SIZE "32 * 1024" CACHE STRING "SP heap size in bytes")
set(TRACE_PREFIX "SEPROXY" CACHE STRING "Trace prefix")
include(${TS_ROOT}/tools/cmake/common/TargetCompileDefinitions.cmake)
set_target_uuids(
	SP_UUID ${SP_UUID_CANON}
	SP_NAME "se-proxy"
)

target_include_directories(se-proxy PRIVATE
	${CMAKE_CURRENT_LIST_DIR}
)

#-------------------------------------------------------------------------------
#  Components that are specific to deployment in the opteesp environment.
#
#-------------------------------------------------------------------------------
add_components(TARGET "se-proxy"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"environments/opteesp"
)

include(../../env/commonsp/se_proxy_sp.cmake REQUIRED)
include(../../infra/stub/stub.cmake REQUIRED)
include(../../se-proxy.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  Set target platform to provide drivers needed by the deployment
#
#-------------------------------------------------------------------------------
add_platform(TARGET "se-proxy")

#-------------------------------------------------------------------------------
#  Deployment specific build options
#-------------------------------------------------------------------------------
target_compile_definitions(se-proxy PRIVATE
	ARM64=1
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	target_compile_options(se-proxy PRIVATE
		-std=c99
	)

endif()

compiler_generate_stripped_elf(TARGET se-proxy NAME "${SP_UUID_CANON}.stripped.elf" RES STRIPPED_ELF)

#-------------------------------------------------------------------------------
#  Deployment specific install options
#-------------------------------------------------------------------------------
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "location to install build output to." FORCE)
endif()
#TODO: api headers

install(TARGETS se-proxy
			PUBLIC_HEADER DESTINATION ${TS_ENV}/include
			RUNTIME DESTINATION ${TS_ENV}/bin
		)
install(FILES ${STRIPPED_ELF} DESTINATION ${TS_ENV}/bin)

get_property(_PROTO_FILES TARGET se-proxy PROPERTY PROTOBUF_FILES)
install(FILES ${_PROTO_FILES} DESTINATION ${TS_ENV}/lib/protobuf)

include(${TS_ROOT}/tools/cmake/common/ExportSp.cmake)
export_sp(
	SP_UUID_CANON ${SP_UUID_CANON}
	SP_UUID_LE ${SP_UUID_LE}
	SP_NAME "se-proxy"
	MK_IN ${TS_ROOT}/environments/opteesp/sp.mk.in
	DTS_IN ${CMAKE_CURRENT_LIST_DIR}/default_se-proxy.dts.in
	JSON_IN ${TS_ROOT}/environments/opteesp/sp_pkg.json.in
)