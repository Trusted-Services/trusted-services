#-------------------------------------------------------------------------------
# Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.18 FATAL_ERROR)
include(../../deployment.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  The CMakeLists.txt for building the component-test deployment for arm-linux
#
#  Used for building and running component level tests as a Linux userspace
#  program running on Arm. Tests can be run by running the built executable
#  called "component-test"
#-------------------------------------------------------------------------------
include(${TS_ROOT}/environments/arm-linux/env.cmake)
project(trusted-services LANGUAGES CXX C)
add_executable(component-test)
target_include_directories(component-test PRIVATE "${TOP_LEVEL_INCLUDE_DIRS}")

add_components(
	TARGET "component-test"
	BASE_DIR ${TS_ROOT}
	COMPONENTS   "components/app/test-runner"
)

include(${TS_ROOT}/external/CppUTest/CppUTest.cmake)
target_link_libraries(component-test PRIVATE ${CppUTest_LIBRARIES})

#-------------------------------------------------------------------------------
#  Components that are specific to deployment in the arm-linux environment.
#
#-------------------------------------------------------------------------------
set(TRACE_PREFIX "TEST" CACHE STRING "Trace prefix")
add_components(
	TARGET "component-test"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/service/crypto/backend/mbedcrypto/trng_adapter/linux"
		"environments/arm-linux"
)

#-------------------------------------------------------------------------------
#  Extend with components that are common across all deployments of
#  component-test
#
#-------------------------------------------------------------------------------
include(../component-test.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  Define library options and dependencies.
#
#-------------------------------------------------------------------------------
target_link_libraries(component-test PRIVATE stdc++ gcc m)
