#-------------------------------------------------------------------------------
# Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

#[===[.rst:
  The base deployment CMake file
  ------------------------------

  Contains common CMake definitions that are used by concrete deployments.
  This file should be included first by a concrete deployment's CMakeLists.txt.
#]===]

# Sets TS-ROOT which is used as the reference directory for everything contained within the project
get_filename_component(TS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../" ABSOLUTE CACHE PATH "Trusted Services root directory.")

# Replicate TS_ROOT as environment variable to allow access from child CMake contexts
set(ENV{TS_ROOT} "${TS_ROOT}")

# Common utilities used by the build system
include(${TS_ROOT}/tools/cmake/common/Utils.cmake REQUIRED)
include(${TS_ROOT}/tools/cmake/common/AddComponents.cmake REQUIRED)
include(${TS_ROOT}/tools/cmake/common/AddPlatform.cmake REQUIRED)
include(${TS_ROOT}/tools/cmake/common/TsGetVersion.cmake REQUIRED)

# Check build environment requirements are met
ts_verify_build_env()

# Project wide include directories
set(TOP_LEVEL_INCLUDE_DIRS
  "${TS_ROOT}"
  "${TS_ROOT}/components"
  )

# Set platform provider root default to use if no commandline variable value has been specified.
# The root path may be specified to allow an external project to provide platform definitions.
if (DEFINED ENV{TS_PLATFORM_ROOT})
  set(_default_platform_root ENV{TS_PLATFORM_ROOT})
else()
  set(_default_platform_root "${TS_ROOT}/platform/providers")
endif()
set(TS_PLATFORM_ROOT ${_default_platform_root} CACHE STRING "Platform provider path")

# Set the default platform to use if no explict platform has been specified on the cmake commandline.
if (DEFINED ENV{TS_PLATFORM})
  set(_default_platform ENV{TS_PLATFORM})
else()
  set(_default_platform "ts/vanilla")
endif()
set(TS_PLATFORM ${_default_platform} CACHE STRING "Selected platform")

# Custom property for defining platform feature dependencies based on components used in a deployment
define_property(TARGET PROPERTY TS_PLATFORM_DRIVER_DEPENDENCIES
  BRIEF_DOCS "List of platform driver interfaces used for a deployment."
  FULL_DOCS "Used by the platform specific builder to specify a configuration for the built platform components."
  )

# Set default build type to Debug
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type.")
endif()

if (COVERAGE)
	set(CMAKE_BUILD_TYPE "DebugCoverage" CACHE STRING "Build type" FORCE)
endif()

# List of supported build types. Needs to be in alignment with the toolchain file
set(TS_SUPPORTED_BUILD_TYPES "DEBUG" "MINSIZEREL" "MINSIZWITHDEBINFO" "RELEASE" "RELWITHDEBINFO" "DEBUGCOVERAGE" CACHE
  STRING "List of supported build types.")

# Convert the build type string to upper case to help case insensitive comparison.
string(TOUPPER "${CMAKE_BUILD_TYPE}" UC_CMAKE_BUILD_TYPE CACHE STRING "Easy to compare build type.")
mark_as_advanced(UC_CMAKE_BUILD_TYPE)

# Validate build type
if (NOT "${UC_CMAKE_BUILD_TYPE}" IN_LIST TS_SUPPORTED_BUILD_TYPES)
	message(FATAL_ERROR "Unknown build type \"${CMAKE_BUILD_TYPE}\" specified in CMAKE_BUILD_TYPE.")
endif()

# Set postfix of libraries according the build type.
set(CMAKE_DEBUGCOVERAGE_POSTFIX "c")
set(CMAKE_DEBUG_POSTFIX "d")

# Default protocol UUID used by TS SPs.
set(TS_RPC_UUID_CANON "bdcd76d7-825e-4751-963b-86d4f84943ac" CACHE STRING "Trusted Services PRC (protocol) UUID.")
