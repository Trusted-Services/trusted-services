#-------------------------------------------------------------------------------
# Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#  The base build file shared between deployments of 'psa-api-test' for
#  different environments.  Used for running PSA API tests.
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#  Components that are common across all deployments
#
#-------------------------------------------------------------------------------

target_sources(${PROJECT_NAME} PRIVATE
	${TS_ROOT}/deployments/psa-api-test/arch_test_runner.c
)

#-------------------------------------------------------------------------------
#  Use libpsats for locating PSA services. An appropriate version of
#  libpsats will be imported for the environment. Making sure the link order is
#  correct.
#-------------------------------------------------------------------------------
if (COVERAGE)
	set(LIBPSATS_BUILD_TYPE "DEBUGCOVERAGE" CACHE STRING "Libpsats build type" FORCE)
	set(LIBTS_BUILD_TYPE "DEBUGCOVERAGE" CACHE STRING "Libts build type" FORCE)
endif()

include(${TS_ROOT}/deployments/libpsats/libpsats-import.cmake)
target_link_libraries( ${PROJECT_NAME} PRIVATE libpsats::psats)

target_link_libraries(${PROJECT_NAME} PRIVATE val_nspe test_combine pal_nspe)

#-------------------------------------------------------------------------------
#  Export project header paths for arch tests
#
#-------------------------------------------------------------------------------
get_target_property(_include_paths ${PROJECT_NAME} INCLUDE_DIRECTORIES)
list(APPEND PSA_ARCH_TESTS_EXTERNAL_INCLUDE_PATHS ${_include_paths})
include(${TS_ROOT}/external/psa_arch_tests/psa_arch_tests.cmake)

#-------------------------------------------------------------------------------
#  Define install content.
#
#-------------------------------------------------------------------------------
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "location to install build output to." FORCE)
endif()
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${TS_ENV}/bin)
