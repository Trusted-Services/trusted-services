#-------------------------------------------------------------------------------
# Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Import libpsa into a dependent in-tree deployment build.  Where another
# deployment uses libpsa, including this file in the dependent deployment
# CMake build file allows libpsa to be built and installed into the binary
# directory of the dependent.
#-------------------------------------------------------------------------------
option(CFG_FORCE_PREBUILT_LIBPSA Off)
# Try to find a pre-build package.
version_semver_read(FILE "${CMAKE_CURRENT_LIST_DIR}/version.txt" MAJOR _major MINOR _minor PATCH _patch)
set(_verstring "${_major}.${_minor}.${_patch}")

if (COVERAGE)
	set(LIBPSA_BUILD_TYPE "DebugCoverage" CACHE STRING "Build type." FORCE)
endif()

find_package(libpsa "${_verstring}" QUIET PATHS ${CMAKE_CURRENT_BINARY_DIR}/libpsa_install/${TS_ENV}/lib/cmake/libpsa)
if(NOT libpsa_FOUND)
	if (CFG_FORCE_PREBUILT_LIBPSA)
		string(CONCAT _msg "find_package() failed to find the \"libpsa\" package. Please pass -Dlibpsa_ROOT=<full path>"
							" to cmake, where <full path> is the directory of the libpsaConfig.cmake file, or "
							" pass -DCMAKE_FIND_ROOT_PATH=<path>, where <path> is the INSTALL_PREFIX used"
							" when building libpsa. libpsa_ROOT can be set in the environment too."
						   "If you wish to debug the search process pass -DCMAKE_FIND_DEBUG_MODE=ON to cmake.")
		message(FATAL_ERROR ${_msg})
	endif()
	# Set build type, if a specific value is required. This leaves the default value in the hands of the
	# libpsa deployment being built.
	if (DEFINED LIBPSA_BUILD_TYPE)
		set(_libpsa_build_type_arg "-DCMAKE_BUILD_TYPE=${LIBPSA_BUILD_TYPE}")
	endif()

	# If not successful, build libpsa as a sub-project.
	execute_process(COMMAND
		${CMAKE_COMMAND} -E env "CROSS_COMPILE=${CROSS_COMPILE}"
		${CMAKE_COMMAND}
			${_libpsa_build_type_arg}
			-S ${TS_ROOT}/deployments/libpsa/${TS_ENV}
			-B ${CMAKE_CURRENT_BINARY_DIR}/libpsa
		RESULT_VARIABLE
			_exec_error
		)
	unset(_libpsa_build_type_arg)
	if (NOT _exec_error EQUAL 0)
		message(FATAL_ERROR "Configuring libpsa failed. ${_exec_error}")
	endif()
	execute_process(COMMAND
		${CMAKE_COMMAND} -E env "CROSS_COMPILE=${CROSS_COMPILE}"
		${CMAKE_COMMAND}
			--build ${CMAKE_CURRENT_BINARY_DIR}/libpsa
			--parallel ${PROCESSOR_COUNT}
		RESULT_VARIABLE
			_exec_error
		)
	if (NOT _exec_error EQUAL 0)
		message(FATAL_ERROR "Installing libpsa failed. ${_exec_error}")
	endif()
	execute_process(COMMAND
		${CMAKE_COMMAND} -E env "CROSS_COMPILE=${CROSS_COMPILE}"
		${CMAKE_COMMAND}
			--install ${CMAKE_CURRENT_BINARY_DIR}/libpsa
			--prefix ${CMAKE_CURRENT_BINARY_DIR}/libpsa_install
		RESULT_VARIABLE
			_exec_error
		)
	if (NOT _exec_error EQUAL 0)
		message(FATAL_ERROR "Installing libpsa failed. ${_exec_error}")
	endif()

	install(SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/libpsa/cmake_install.cmake)

	find_package(libpsa "${_verstring}" QUIET REQUIRED PATHS ${CMAKE_CURRENT_BINARY_DIR}/libpsa_install/${TS_ENV}/lib/cmake/libpsa)
else()
	message(STATUS "Using prebuilt libpsa from ${libpsa_DIR}")
endif()

# Cmake will use the same build type of the imported target as used by the main project. If no mapping is configured and
# the matching build type is not found, cmake will fall back to any build type. Details of the fall back mechanism are not
# documented.
# If a mapping is defined, and the imported target does not define the mapped build type, cmake will treat the library
# as not found.
#
# If LIBPSA_BUILD_TYPE is set and the main project wants to use a specific build type, configure build type mapping to
# only allow using the requested build type.
if (DEFINED LIBPSA_BUILD_TYPE)
	set_target_properties(libpsa::psa PROPERTIES
		MAP_IMPORTED_CONFIG_DEBUG				${LIBPSA_BUILD_TYPE}
		MAP_IMPORTED_CONFIG_MINSIZEREL			${LIBPSA_BUILD_TYPE}
		MAP_IMPORTED_CONFIG_MINSIZWITHDEBINFO	${LIBPSA_BUILD_TYPE}
		MAP_IMPORTED_CONFIG_RELEASE				${LIBPSA_BUILD_TYPE}
		MAP_IMPORTED_CONFIG_RELWITHDEBINFO		${LIBPSA_BUILD_TYPE}
		MAP_IMPORTED_CONFIG_DEBUGCOVERAGE		${LIBPSA_BUILD_TYPE}
	)

	# Do a manual check and issue a better message than the default one.
	get_property(_libpsa_build_type TARGET libpsa::psa PROPERTY IMPORTED_CONFIGURATIONS)
	string(TOUPPER ${LIBPSA_BUILD_TYPE} _uc_libpsa_build_type)
	if(${_uc_libpsa_build_type} IN_LIST _libpsa_build_type)
	else()
		message(FATAL_ERROR "Installed libpsa package does not supports required build type ${LIBPSA_BUILD_TYPE}.")
	endif()
	unset(_libpsa_build_type)
	unset(_uc_libpsa_build_type)
endif()

# libpsa can not be used without libts, so add the needed dependency.
include(${TS_ROOT}/deployments/libts/libts-import.cmake)
target_link_libraries(libpsa::psa INTERFACE libts::ts)
