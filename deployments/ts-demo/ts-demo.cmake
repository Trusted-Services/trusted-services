#-------------------------------------------------------------------------------
# Copyright (c) 2020-2023, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#  The base build file shared between deployments of 'ts-demo' for
#  different environments.  Demonstrates use of trusted services by a
#  client application.
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#  Use libpsa for locating PSA services. An appropriate version of
#  libpsa will be imported for the environment.
#-------------------------------------------------------------------------------
include(${TS_ROOT}/deployments/libpsa/libpsa-import.cmake)
target_link_libraries(ts-demo PRIVATE libpsa::psa)

#-------------------------------------------------------------------------------
#  Common main for all deployments
#
#-------------------------------------------------------------------------------
target_sources(ts-demo PRIVATE
	"${CMAKE_CURRENT_LIST_DIR}/ts-demo.c"
)

#-------------------------------------------------------------------------------
#  Components that are common across all deployments
#
#-------------------------------------------------------------------------------
add_components(
	TARGET "ts-demo"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/app/ts-demo"
)

#-------------------------------------------------------------------------------
#  Define install content.
#
#-------------------------------------------------------------------------------
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "location to install build output to." FORCE)
endif()
install(TARGETS ts-demo RUNTIME DESTINATION ${TS_ENV}/bin)
