#-------------------------------------------------------------------------------
# Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Import libts into a dependent in-tree deployment build.  Where another
# deployment uses libts, including this file in the dependent deployment
# CMake build file allows libts to be built and installed into the binary
# directory of the dependent.
#-------------------------------------------------------------------------------
option(CFG_FORCE_PREBUILT_LIBTS Off)
# Try to find a pre-build package.
version_semver_read(FILE "${CMAKE_CURRENT_LIST_DIR}/version.txt" MAJOR _major MINOR _minor PATCH _patch)
set(_verstring "${_major}.${_minor}.${_patch}")

if (COVERAGE)
	set(LIBTS_BUILD_TYPE "DebugCoverage" CACHE STRING "Build type." FORCE)
endif()

find_package(libts "${_verstring}" QUIET PATHS ${CMAKE_CURRENT_BINARY_DIR}/libts_install/${TS_ENV}/lib/cmake/libts)
if(NOT libts_FOUND)
	if (CFG_FORCE_PREBUILT_LIBTS)
		string(CONCAT _msg "find_package() failed to find the \"libts\" package. Please pass -Dlibts_ROOT=<path> or"
						   " -DCMAKE_FIND_ROOT_PATH=<path> cmake variable, where <path> is the INSTALL_PREFIX used"
						   " when building libts. libts_ROOT can be set in the environment too."
						   "If you wish to debug the search process pass -DCMAKE_FIND_DEBUG_MODE=ON to cmake.")
		message(FATAL_ERROR ${_msg})
	endif()
	# Set build type, if a specific value is required. This leaves the default value in the hands of the
	# libts deployment being built.
	if (DEFINED LIBTS_BUILD_TYPE)
		set(_libts_build_type_arg "-DCMAKE_BUILD_TYPE=${LIBTS_BUILD_TYPE}")
	endif()

	# If not successful, build libts as a sub-project.
	execute_process(COMMAND
		${CMAKE_COMMAND} -E env "CROSS_COMPILE=${CROSS_COMPILE}"
		${CMAKE_COMMAND}
			${_libts_build_type_arg}
			-S ${TS_ROOT}/deployments/libts/${TS_ENV}
			-B ${CMAKE_CURRENT_BINARY_DIR}/libts
		RESULT_VARIABLE
			_exec_error
		)
	unset(_libts_build_type_arg)
	if (NOT _exec_error EQUAL 0)
		message(FATAL_ERROR "Configuring libts failed. ${_exec_error}")
	endif()
	execute_process(COMMAND
		${CMAKE_COMMAND} -E env "CROSS_COMPILE=${CROSS_COMPILE}"
		${CMAKE_COMMAND}
			--build ${CMAKE_CURRENT_BINARY_DIR}/libts
			--parallel ${PROCESSOR_COUNT}
		RESULT_VARIABLE
			_exec_error
		)
	if (NOT _exec_error EQUAL 0)
		message(FATAL_ERROR "Installing libts failed. ${_exec_error}")
	endif()
	execute_process(COMMAND
		${CMAKE_COMMAND} -E env "CROSS_COMPILE=${CROSS_COMPILE}"
		${CMAKE_COMMAND}
			--install ${CMAKE_CURRENT_BINARY_DIR}/libts
			--prefix ${CMAKE_CURRENT_BINARY_DIR}/libts_install
		RESULT_VARIABLE
			_exec_error
		)
	if (NOT _exec_error EQUAL 0)
		message(FATAL_ERROR "Installing libts failed. ${_exec_error}")
	endif()

	install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/libts/cmake_install.cmake)

	find_package(libts "${_verstring}" QUIET REQUIRED PATHS ${CMAKE_CURRENT_BINARY_DIR}/libts_install/${TS_ENV}/lib/cmake/libts)
else()
	message(STATUS "Using prebuilt libts from ${libts_DIR}")
endif()

# Cmake will use the same build type of the imported target as used by the main project. If no mapping is configured and
# the matching build type is not found, cmake will fall back to any build type. Details of the fall back mechanism are not
# documented.
# If a mapping is defined, and the imported target does not define the mapped build type, cmake will treat the library
# as not found.
#
# If LIBTS_BUILD_TYPE is set and the main project wants to use a specific build type, configure build type mapping to
# only allow using the requested build type.
if (DEFINED LIBTS_BUILD_TYPE)
	set_target_properties(libts::ts PROPERTIES
		MAP_IMPORTED_CONFIG_DEBUG				${LIBTS_BUILD_TYPE}
		MAP_IMPORTED_CONFIG_MINSIZEREL			${LIBTS_BUILD_TYPE}
		MAP_IMPORTED_CONFIG_MINSIZWITHDEBINFO	${LIBTS_BUILD_TYPE}
		MAP_IMPORTED_CONFIG_RELEASE				${LIBTS_BUILD_TYPE}
		MAP_IMPORTED_CONFIG_RELWITHDEBINFO		${LIBTS_BUILD_TYPE}
		MAP_IMPORTED_CONFIG_DEBUGCOVERAGE		${LIBTS_BUILD_TYPE}
	)

	# Do a manual check and issue a better message than the default one.
	get_property(_libts_build_type TARGET libts::ts PROPERTY IMPORTED_CONFIGURATIONS)
	string(TOUPPER ${LIBTS_BUILD_TYPE} _uc_libts_build_type)
	if(${_uc_libts_build_type} IN_LIST _libts_build_type)
	else()
		message(FATAL_ERROR "Installed libts package does not supports required build type ${LIBTS_BUILD_TYPE}.")
	endif()
	unset(_libts_build_type)
	unset(_uc_libts_build_type)
endif()
