#-------------------------------------------------------------------------------
# Copyright (c) 2020-2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
include(../../deployment.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  The CMakeLists.txt for building the ts-service-test deployment for linux-pc
#
#  Used for building and running service level tests in a native PC enviroment.
#  Tests can be run by running the built executable called "ts-service-test"
#-------------------------------------------------------------------------------
include(${TS_ROOT}/environments/linux-pc/env.cmake)
project(trusted-services LANGUAGES CXX C)

# Prevents symbols in the ts-service-test executable overriding symbols with
# with same name in libts during dyanmic linking performed by the program
# loader.  This avoid psa crypto api symbols provided by the mbedcrypto
# library from being overridden by the same symbols in the ts-service-test
# executable.
set(CMAKE_C_VISIBILITY_PRESET hidden)

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
	set(TS_SERVICE_TEST_COVERAGE_FILE "ts-service-test-coverage.info" CACHE PATH "Path of coverage info file")
	set(TS_SERVICE_TEST_COVERAGE_REPORT_DIR "${CMAKE_CURRENT_BINARY_DIR}/ts-service-coverage-report" CACHE PATH "Directory of coverage report")

	# Collecting coverage
	coverage_generate(
		NAME "ts-service test"
		SOURCE_DIR ${TS_ROOT}
		BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}
		OUTPUT_FILE ${COVERAGE_FILE}
	)

	# Filtering project file coverage
	coverage_filter(
		INPUT_FILE ${COVERAGE_FILE}
		OUTPUT_FILE ${TS_SERVICE_TEST_COVERAGE_FILE}
		INCLUDE_DIRECTORY ${UNIT_TEST_PROJECT_PATH}/components
	)

	# Coverage report
	coverage_generate_report(
		INPUT_FILE ${TS_SERVICE_TEST_COVERAGE_FILE}
		OUTPUT_DIRECTORY ${TS_SERVICE_TEST_COVERAGE_REPORT_DIR}
	)
endif()

unit_test_add_suite(
	NAME ts-service-test
)

target_include_directories(ts-service-test PRIVATE "${TOP_LEVEL_INCLUDE_DIRS}")

#-------------------------------------------------------------------------------
#  External project source-level dependencies
#
#-------------------------------------------------------------------------------
include(${TS_ROOT}/external/tf_a/tf-a.cmake)
add_tfa_dependency(TARGET "ts-service-test")

#-------------------------------------------------------------------------------
#  Components that are specific to deployment in the linux-pc environment.
#
#-------------------------------------------------------------------------------
add_components(
	TARGET "ts-service-test"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/common/crc32"
		"components/service/test_runner/client/cpp"
		"components/service/test_runner/test/service"
		"components/service/block_storage/block_store"
		"components/service/block_storage/block_store/client"
		"components/service/block_storage/block_store/partitioned"
		"components/service/block_storage/block_store/device"
		"components/service/block_storage/block_store/device/ram"
		"components/service/block_storage/factory/client"
		"components/service/block_storage/test/service"
		"components/service/common/provider"
		"components/service/fwu/agent"
		"components/service/fwu/common"
		"components/service/fwu/fw_store/banked"
		"components/service/fwu/fw_store/banked/metadata_serializer/v1"
		"components/service/fwu/fw_store/banked/metadata_serializer/v2"
		"components/service/fwu/installer"
		"components/service/fwu/installer/raw"
		"components/service/fwu/installer/copy"
		"components/service/fwu/inspector/direct"
		"components/service/fwu/provider"
		"components/service/fwu/provider/serializer"
		"components/service/fwu/test/fwu_client/direct"
		"components/service/fwu/test/fwu_dut/sim"
		"components/service/fwu/test/fwu_dut_factory/remote_sim"
		"components/service/fwu/test/metadata_fetcher/volume"
		"components/service/fwu/test/ref_scenarios"
		"components/service/rpmb/backend/emulated"
		"components/service/rpmb/provider"
		"components/media/volume"
		"components/media/volume/index"
		"components/media/volume/base_io_dev"
		"components/media/volume/block_volume"
)

#-------------------------------------------------------------------------------
#  Extend with components that are common across all deployments of
#  ts-service-test
#
#-------------------------------------------------------------------------------
include(../ts-service-test.cmake REQUIRED)
