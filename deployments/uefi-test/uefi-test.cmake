#-------------------------------------------------------------------------------
# Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#  The base build file shared between deployments of 'uefi-test' for
#  different environments.  Used for running end-to-end service-level tests
#  against SMM service providers that implement UEFI services such as smm
#  variable.
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#  Use libts for locating and accessing services. An appropriate version of
#  libts will be imported for the enviroment in which service tests are
#  deployed.
#-------------------------------------------------------------------------------
if (COVERAGE)
	set(LIBTS_BUILD_TYPE "DEBUGCOVERAGE" CACHE STRING "Libts build type" FORCE)
endif()

include(${TS_ROOT}/deployments/libts/libts-import.cmake)
target_link_libraries(uefi-test PRIVATE libts::ts)

#-------------------------------------------------------------------------------
# Options and variables
#-------------------------------------------------------------------------------
option(UEFI_AUTH_VAR "Enable variable authentication" ON)
if (UEFI_AUTH_VAR)
	target_compile_definitions(uefi-test PRIVATE
		-DUEFI_AUTH_VAR
	)
endif()

#-------------------------------------------------------------------------------
#  Components that are common accross all deployments
#
#-------------------------------------------------------------------------------
add_components(
	TARGET "uefi-test"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/service/uefi/smm_variable/client/cpp"
		"components/service/uefi/smm_variable/test/service"
		"components/common/uuid"
)

#-------------------------------------------------------------------------------
#  Define install content.
#
#-------------------------------------------------------------------------------
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "location to install build output to." FORCE)
endif()
install(TARGETS uefi-test RUNTIME DESTINATION ${TS_ENV}/bin)
