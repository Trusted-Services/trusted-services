#-------------------------------------------------------------------------------
# Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
include(../../deployment.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  The CMakeLists.txt for building the libts deployment for arm-linux
#
#  Used for building the libts library for the arm-linux enviroment.  Used for
#  locating and accessing services from a Linux userspace client.  Service
#  instances can be located in any supported secure processing enviroment.
#-------------------------------------------------------------------------------
include(${TS_ROOT}/environments/arm-linux/env_shared_lib.cmake)
project(trusted-services LANGUAGES CXX C)
add_library(ts SHARED)
target_include_directories(ts PRIVATE "${TOP_LEVEL_INCLUDE_DIRS}")

# Setting the MM communication buffer parameters
set(MM_COMM_BUFFER_ADDRESS "0x881000000" CACHE STRING "Address of MM communicte buffer")
set(MM_COMM_BUFFER_SIZE "8*4*1024" CACHE STRING "Size of the MM communicate buffer in bytes")

#-------------------------------------------------------------------------------
#  For user-specific tracing set to TRACE_LEVEL_NONE and implement:
#  void trace_puts(const char *str)
#-------------------------------------------------------------------------------

set(TRACE_PREFIX "LIBTS" CACHE STRING "Trace prefix")
set(TRACE_LEVEL "TRACE_LEVEL_DEBUG" CACHE STRING "Trace level")
#-------------------------------------------------------------------------------
#  Components that are specific to deployment in the arm-linux environment.
#
#-------------------------------------------------------------------------------
add_components(
	TARGET "ts"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/common/utils"
		"components/rpc/ts_rpc/caller/linux"
		"components/rpc/mm_communicate/caller/linux"
		"components/service/locator/linux"
		"components/service/locator/linux/ffa"
		"components/service/locator/linux/mm_communicate"
		"components/common/uuid"
)

#-------------------------------------------------------------------------------
#  Extend with components that are common across all deployments of
#  libts
#
#-------------------------------------------------------------------------------
include(../libts.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  Define library options and dependencies.
#
#-------------------------------------------------------------------------------
target_link_libraries(ts PRIVATE gcc)
