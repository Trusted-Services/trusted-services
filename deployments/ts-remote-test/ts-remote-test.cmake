#-------------------------------------------------------------------------------
# Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#  The base build file shared between deployments of 'ts-remote-test' for
#  different environments.  Acts as a client for tests running in a remote
#  processing environment.
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#  Use libts for locating and accessing services. An appropriate version of
#  libts will be imported for the environment in which tests are
#  deployed.
#-------------------------------------------------------------------------------
if (COVERAGE)
	set(LIBTS_BUILD_TYPE "DEBUGCOVERAGE" CACHE STRING "Libts build type" FORCE)
endif()

include(${TS_ROOT}/deployments/libts/libts-import.cmake)
target_link_libraries(ts-remote-test PRIVATE libts::ts)

#-------------------------------------------------------------------------------
#  Common main for all deployments
#
#-------------------------------------------------------------------------------
target_sources(ts-remote-test PRIVATE
	"${CMAKE_CURRENT_LIST_DIR}/ts-remote-test.cpp"
)

#-------------------------------------------------------------------------------
#  Components that are common across all deployments
#
#-------------------------------------------------------------------------------
add_components(
	TARGET "ts-remote-test"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/app/remote-test-runner"
		"components/common/tlv"
		"components/service/common/include"
		"components/service/common/client"
		"components/service/test_runner/client/cpp"
)

#-------------------------------------------------------------------------------
#  Define install content.
#
#-------------------------------------------------------------------------------
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "location to install build output to." FORCE)
endif()
install(TARGETS ts-remote-test RUNTIME DESTINATION ${TS_ENV}/bin)
