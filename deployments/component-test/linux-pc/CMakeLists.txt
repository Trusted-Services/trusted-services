#-------------------------------------------------------------------------------
# Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
include(../../deployment.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  The CMakeLists.txt for building the component-test deployment for linux-pc
#
#  Used for building and running component level tests in a native PC enviroment.
#  Tests can be run by running the built executable called "component-test"
#-------------------------------------------------------------------------------
include(${TS_ROOT}/environments/linux-pc/env.cmake)
project(trusted-services LANGUAGES CXX C)

# Preparing firmware-test-build by including it
include(${TS_ROOT}/external/firmware_test_builder/FirmwareTestBuilder.cmake)

include(CTest)
include(UnitTest)

set(COVERAGE FALSE CACHE BOOL "Enable code coverage measurement")
set(UNIT_TEST_PROJECT_PATH ${TS_ROOT} CACHE PATH "Path of the project directory")
set(CMAKE_CXX_STANDARD 11)

unit_test_init_cpputest()

if (COVERAGE)
	include(Coverage)

	set(COVERAGE_FILE "coverage.info")
	set(COMPONENT_TEST_COVERAGE_FILE "component-test-coverage.info" CACHE PATH "Path of coverage info file")
	set(COMPONENT_TEST_COVERAGE_REPORT_DIR "${CMAKE_CURRENT_BINARY_DIR}/component-test-coverage-report" CACHE PATH "Directory of coverage report")

	# Collecting coverage
	coverage_generate(
		NAME "Component test"
		SOURCE_DIR ${TS_ROOT}
		BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}
		OUTPUT_FILE ${COVERAGE_FILE}
	)

	# Filtering project file coverage
	coverage_filter(
		INPUT_FILE ${COVERAGE_FILE}
		OUTPUT_FILE ${COMPONENT_TEST_COVERAGE_FILE}
		INCLUDE_DIRECTORY ${UNIT_TEST_PROJECT_PATH}/components
	)

	# Coverage report
	coverage_generate_report(
		INPUT_FILE ${COMPONENT_TEST_COVERAGE_FILE}
		OUTPUT_DIRECTORY ${COMPONENT_TEST_COVERAGE_REPORT_DIR}
	)
endif()

unit_test_add_suite(
	NAME component-test
)

target_include_directories(component-test PRIVATE "${TOP_LEVEL_INCLUDE_DIRS}")

#-------------------------------------------------------------------------------
#  Components that are specific to deployment in the linux-pc environment.
#
#-------------------------------------------------------------------------------
set(TRACE_PREFIX "TEST" CACHE STRING "Trace prefix")
add_components(
	TARGET "component-test"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/service/crypto/backend/mbedcrypto/trng_adapter/linux"
		"environments/linux-pc"
)

#-------------------------------------------------------------------------------
#  Extend with components that are common across all deployments of
#  component-test
#
#-------------------------------------------------------------------------------
include(../component-test.cmake REQUIRED)
