#-------------------------------------------------------------------------------
# Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Import libpsats into a dependent in-tree deployment build.  Where another
# deployment uses libpsats, including this file in the dependent deployment
# CMake build file allows libpsats to be built and installed into the binary
# directory of the dependent.
#-------------------------------------------------------------------------------
option(CFG_FORCE_PREBUILT_LIBPSATS Off)
# Try to find a pre-build package.
version_semver_read(FILE "${CMAKE_CURRENT_LIST_DIR}/version.txt" MAJOR _major MINOR _minor PATCH _patch)
set(_verstring "${_major}.${_minor}.${_patch}")

if (COVERAGE)
	set(LIBPSATS_BUILD_TYPE "DebugCoverage" CACHE STRING "Build type." FORCE)
endif()

find_package(libpsats "${_verstring}" QUIET PATHS ${CMAKE_CURRENT_BINARY_DIR}/libpsats_install/${TS_ENV}/lib/cmake/libpsats)
if(NOT libpsats_FOUND)
	if (CFG_FORCE_PREBUILT_LIBPSATS)
		string(CONCAT _msg "find_package() failed to find the \"libpsats\" package. Please pass -Dlibpsats_ROOT=<full path>"
							" to cmake, where <full path> is the directory of the libpsatsConfig.cmake file, or "
							" pass -DCMAKE_FIND_ROOT_PATH=<path>, where <path> is the INSTALL_PREFIX used"
							" when building libpsats. libpsats_ROOT can be set in the environment too."
						   "If you wish to debug the search process pass -DCMAKE_FIND_DEBUG_MODE=ON to cmake.")
		message(FATAL_ERROR ${_msg})
	endif()
	# Set build type, if a specific value is required. This leaves the default value in the hands of the
	# libpsats deployment being built.
	if (DEFINED LIBPSATS_BUILD_TYPE)
		set(_libpsats_build_type_arg "-DCMAKE_BUILD_TYPE=${LIBPSATS_BUILD_TYPE}")
	endif()

	# If not successful, build libpsats as a sub-project.
	execute_process(COMMAND
		${CMAKE_COMMAND} -E env "CROSS_COMPILE=${CROSS_COMPILE}"
		${CMAKE_COMMAND}
			${_libpsats_build_type_arg}
			-S ${TS_ROOT}/deployments/libpsats/${TS_ENV}
			-B ${CMAKE_CURRENT_BINARY_DIR}/libpsats
		RESULT_VARIABLE
			_exec_error
		)
	unset(_libpsats_build_type_arg)
	if (NOT _exec_error EQUAL 0)
		message(FATAL_ERROR "Configuring libpsats failed. ${_exec_error}")
	endif()
	execute_process(COMMAND
		${CMAKE_COMMAND} -E env "CROSS_COMPILE=${CROSS_COMPILE}"
		${CMAKE_COMMAND}
			--build ${CMAKE_CURRENT_BINARY_DIR}/libpsats
			--parallel ${PROCESSOR_COUNT}
		RESULT_VARIABLE
			_exec_error
		)
	if (NOT _exec_error EQUAL 0)
		message(FATAL_ERROR "Installing libpsats failed. ${_exec_error}")
	endif()
	execute_process(COMMAND
		${CMAKE_COMMAND} -E env "CROSS_COMPILE=${CROSS_COMPILE}"
		${CMAKE_COMMAND}
			--install ${CMAKE_CURRENT_BINARY_DIR}/libpsats
			--prefix ${CMAKE_CURRENT_BINARY_DIR}/libpsats_install
		RESULT_VARIABLE
			_exec_error
		)
	if (NOT _exec_error EQUAL 0)
		message(FATAL_ERROR "Installing libpsats failed. ${_exec_error}")
	endif()

	install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/libpsats/cmake_install.cmake)

	find_package(libpsats "${_verstring}" QUIET REQUIRED PATHS ${CMAKE_CURRENT_BINARY_DIR}/libpsats_install/${TS_ENV}/lib/cmake/libpsats)
else()
	message(STATUS "Using prebuilt libpsats from ${libpsats_DIR}")
endif()

# Cmake will use the same build type of the imported target as used by the main project. If no mapping is configured and
# the matching build type is not found, cmake will fall back to any build type. Details of the fall back mechanism are not
# documented.
# If a mapping is defined, and the imported target does not define the mapped build type, cmake will treat the library
# as not found.
#
# If LIBPSATS_BUILD_TYPE is set and the main project wants to use a specific build type, configure build type mapping to
# only allow using the requested build type.
if (DEFINED LIBPSATS_BUILD_TYPE)
	set_target_properties(libpsats::psats PROPERTIES
		MAP_IMPORTED_CONFIG_DEBUG				${LIBPSATS_BUILD_TYPE}
		MAP_IMPORTED_CONFIG_MINSIZEREL			${LIBPSATS_BUILD_TYPE}
		MAP_IMPORTED_CONFIG_MINSIZWITHDEBINFO	${LIBPSATS_BUILD_TYPE}
		MAP_IMPORTED_CONFIG_RELEASE				${LIBPSATS_BUILD_TYPE}
		MAP_IMPORTED_CONFIG_RELWITHDEBINFO		${LIBPSATS_BUILD_TYPE}
		MAP_IMPORTED_CONFIG_DEBUGCOVERAGE		${LIBPSATS_BUILD_TYPE}
	)

	# Do a manual check and issue a better message than the default one.
	get_property(_libpsats_build_type TARGET libpsats::psats PROPERTY IMPORTED_CONFIGURATIONS)
	string(TOUPPER ${LIBPSATS_BUILD_TYPE} _uc_libpsats_build_type)
	if(${_uc_libpsats_build_type} IN_LIST _libpsats_build_type)
	else()
		message(FATAL_ERROR "Installed libpsats package does not supports required build type ${LIBPSATS_BUILD_TYPE}.")
	endif()
	unset(_libpsats_build_type)
	unset(_uc_libpsats_build_type)
endif()

# libpsats can not be used without libts, so add the needed dependency.
include(${TS_ROOT}/deployments/libts/libts-import.cmake)
target_link_libraries(libpsats::psats INTERFACE libts::ts)
