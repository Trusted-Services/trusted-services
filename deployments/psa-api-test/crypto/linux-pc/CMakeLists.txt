#-------------------------------------------------------------------------------
# Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
include(../../../deployment.cmake REQUIRED)
include(../../psa-api-test-config.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  The CMakeLists.txt for building the psa-api-test/crypto deployment for
#  linux-pc
#
#  Used for building and running psa arch crypto tests in a native PC enviroment.
#  Tests can be run by running the built executable called "psa-crypto-api-test"
#-------------------------------------------------------------------------------
include(${TS_ROOT}/environments/linux-pc/env.cmake)
project(psa-crypto-api-test LANGUAGES CXX C)
add_executable(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PRIVATE "${TOP_LEVEL_INCLUDE_DIRS}")

set(TRACE_PREFIX "PSACRYPTO" CACHE STRING "Trace prefix")
add_components(TARGET ${PROJECT_NAME}
	BASE_DIR ${TS_ROOT}
	COMPONENTS "environments/linux-pc"
)

#-------------------------------------------------------------------------------
#  Extend with components that are common across all deployments of
#  psa-api-test/crypto
#-------------------------------------------------------------------------------
include(../crypto-api-test.cmake REQUIRED)
