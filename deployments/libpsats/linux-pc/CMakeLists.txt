#-------------------------------------------------------------------------------
# Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
include(../../deployment.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  Options and variables
#-------------------------------------------------------------------------------
set(BUILD_SHARED_LIBS On CACHE BOOL "Determine if a shared library is being built.")
if(NOT BUILD_SHARED_LIBS)
	message(FATAL_ERROR "Building static library is not yet supported. Call cmake with -DBUILD_SHARED_LIBS=1")
endif()

#-------------------------------------------------------------------------------
#  The CMakeLists.txt for building the libpsats deployment for arm-linux
#
#  Used for building the libpsats library for the arm-linux environment. Used for
#  locating and accessing services from a Linux userspace client. Service
#  instances can be located in any supported secure processing environment.
#-------------------------------------------------------------------------------
include(${TS_ROOT}/environments/linux-pc/env_shared_lib.cmake)
project(psats LANGUAGES CXX C)

add_library(psats)

target_include_directories(psats PRIVATE "${TOP_LEVEL_INCLUDE_DIRS}")

#-------------------------------------------------------------------------------
#  For user-specific tracing set to TRACE_LEVEL_NONE and implement:
#  void trace_puts(const char *str)
#-------------------------------------------------------------------------------

set(TRACE_PREFIX "LIBPSATS" CACHE STRING "Trace prefix")
set(TRACE_LEVEL "TRACE_LEVEL_DEBUG" CACHE STRING "Trace level")
#-------------------------------------------------------------------------------
#  Extend with components that are common across all deployments of
#  libpsats
#
#-------------------------------------------------------------------------------
include(../libpsats.cmake REQUIRED)
