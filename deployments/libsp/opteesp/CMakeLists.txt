#-------------------------------------------------------------------------------
# Copyright (c) 2020-2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
include(../../deployment.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  The CMakeLists.txt for building the libsp deployment for opteesp
#
#  Used for building the libsp library that provides FFA related functions
#  for applications deployed in a secure partition.
#-------------------------------------------------------------------------------
include(${TS_ROOT}/environments/opteesp/env.cmake)

version_semver_read(FILE "${CMAKE_CURRENT_LIST_DIR}/version.txt"
					MAJOR _major MINOR _minor PATCH _patch)
set(LIBSP_VERSION "${_major}.${_minor}.${_patch}")
project(trusted-services
			VERSION
				${LIBSP_VERSION}
			 LANGUAGES
				C ASM
		)
unset(_major)
unset(_minor)
unset(_patch)

add_library(sp STATIC)

add_components(TARGET sp
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		components/messaging/ffa/libsp
		components/common/utils
)

add_subdirectory(${TS_ROOT}/components/common/libc ${CMAKE_BINARY_DIR}/libc_build)

target_compile_definitions(sp PRIVATE
	ARM64=1
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	target_compile_options(sp PRIVATE
		-std=c99
	)
endif()

######################################## install
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "location to install build output to." FORCE)
endif()

install(
	TARGETS
		sp
	EXPORT
		LibspTargets
	ARCHIVE	DESTINATION
		${TS_ENV}/lib
	PUBLIC_HEADER DESTINATION
		${TS_ENV}/include
	COMPONENT
		libsp
)

### Create a config file package.
set(ConfigPackageLocation ${TS_ENV}/lib/cmake/libsp)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/LibspConfigVersion.cmake"
	VERSION "${LIBSP_VERSION}"
	COMPATIBILITY SameMajorVersion
)

# Create targets file.
export(
	EXPORT
		LibspTargets
	FILE
		"${CMAKE_CURRENT_BINARY_DIR}/LibspTargets.cmake"
	NAMESPACE
		libsp::
)

# Finalize config file.
configure_package_config_file(
		LibspConfig.cmake.in
		"${CMAKE_CURRENT_BINARY_DIR}/LibspConfig.cmake"
	PATH_VARS

	INSTALL_DESTINATION
		${ConfigPackageLocation}
)

install(
	EXPORT
		LibspTargets
	FILE
		LibspTargets.cmake
	NAMESPACE
		libsp::
	DESTINATION
		${ConfigPackageLocation}
	COMPONENT
		libsp
)

# install config and version files
install(
	FILES
		"${CMAKE_CURRENT_BINARY_DIR}/LibspConfig.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}/LibspConfigVersion.cmake"
	DESTINATION
		${ConfigPackageLocation}
	COMPONENT
		libsp
)
