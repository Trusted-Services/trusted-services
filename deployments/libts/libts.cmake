#-------------------------------------------------------------------------------
# Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#  The base build file shared between deployments of 'libts' for
#  different environments.  libts provides a client interface for locating
#  service instances and establishing RPC sessions for using services.
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#  Common API version implemented by all libts deployments
#-------------------------------------------------------------------------------
version_semver_read(FILE "${CMAKE_CURRENT_LIST_DIR}/version.txt"
					MAJOR _major MINOR _minor PATCH _patch)
set_target_properties(ts PROPERTIES VERSION "${_major}.${_minor}.${_patch}")
set_target_properties(ts PROPERTIES SOVERSION "${_major}")
unset(_major)
unset(_minor)
unset(_patch)

add_library(libts::ts ALIAS ts)

if (COVERAGE)
	set(LIBTS_BUILD_TYPE "DebugCoverage" CACHE STRING "Build type." FORCE)
endif()

#-------------------------------------------------------------------------------
#  Components that are common across all deployments
#
#-------------------------------------------------------------------------------
add_components(
	TARGET "ts"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"environments/${TS_ENV}"
		"components/common/trace"
		"components/rpc/common/caller"
		"components/rpc/common/interface"
		"components/service/locator"
		"components/service/locator/interface"
)

#-------------------------------------------------------------------------------
#  Define public interfaces for library
#
#-------------------------------------------------------------------------------

# Enable exporting interface symbols for library public interface
target_compile_definitions(ts PRIVATE
	EXPORT_PUBLIC_INTERFACE_TRACE
	EXPORT_PUBLIC_INTERFACE_RPC_CALLER
	EXPORT_PUBLIC_INTERFACE_RPC_SERVICE
	EXPORT_PUBLIC_INTERFACE_SERVICE_LOCATOR
)

# Do not export symbols from static libraries linked to this library
target_link_options(ts PRIVATE -Wl,--exclude-libs,ALL)

#-------------------------------------------------------------------------------
#  Export the library and the corresponding public interface header files
#
#-------------------------------------------------------------------------------
include(${TS_ROOT}/tools/cmake/common/ExportLibrary.cmake REQUIRED)

set_property(TARGET "ts" APPEND PROPERTY
			PUBLIC_HEADER "${TS_ROOT}/components/common/trace/include/trace.h")

# Exports library information in preparation for install
export_library(
	TARGET "ts"
	LIB_NAME "libts"
	PKG_CONFIG_FILE "${CMAKE_CURRENT_LIST_DIR}/libtsConfig.cmake.in"
)
