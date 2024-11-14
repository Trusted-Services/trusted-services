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
#  The CMakeLists.txt for building the psa-api-test deployment for arm-linux
#
#  Used for building and running psa arch tests on an Arm based Linux device
#-------------------------------------------------------------------------------
include(${TS_ROOT}/environments/arm-linux/env.cmake)
project(psa-iat-api-test LANGUAGES CXX C)
add_executable(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PRIVATE "${TOP_LEVEL_INCLUDE_DIRS}")

set(TRACE_PREFIX "PSAIAT" CACHE STRING "Trace prefix")
add_components(TARGET ${PROJECT_NAME}
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/common/trace"
		"environments/arm-linux"
)

#-------------------------------------------------------------------------------
#  Extend with components that are common across all deployments of
#  psa-api-test/initial_attestation
#-------------------------------------------------------------------------------
include(../iat-api-test.cmake REQUIRED)

#-------------------------------------------------------------------------------
#  Linker option to enable repeated searches for undefined references.
#  Required to resolve dependencies between arch-test and mbedcrypto libraries.
#-------------------------------------------------------------------------------
target_link_options(${PROJECT_NAME} PRIVATE
	-Wl,--start-group
	)
