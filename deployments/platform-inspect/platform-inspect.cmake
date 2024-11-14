#-------------------------------------------------------------------------------
# Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#  The base build file shared between deployments of 'platform-inspect' for
#  different environments.
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#  Use libpsats for locating PSA services. An appropriate version of
#  libpsats will be imported for the environment.
#-------------------------------------------------------------------------------
if (COVERAGE)
	set(LIBPSATS_BUILD_TYPE "DEBUGCOVERAGE" CACHE STRING "Libpsats build type" FORCE)
	set(LIBTS_BUILD_TYPE "DEBUGCOVERAGE" CACHE STRING "Libts build type" FORCE)
endif()

include(${TS_ROOT}/deployments/libpsats/libpsats-import.cmake)
target_link_libraries(platform-inspect PRIVATE libpsats::psats)

#-------------------------------------------------------------------------------
#  Components that are common across all deployments
#
#-------------------------------------------------------------------------------
add_components(
	TARGET "platform-inspect"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/app/platform-inspect"
		"components/common/tlv"
		"components/common/cbor_dump"
		"components/service/attestation/reporter/dump/raw"
		"components/service/attestation/reporter/dump/pretty"
)

#-------------------------------------------------------------------------------
#  Components used from external projects
#
#-------------------------------------------------------------------------------

# MbedTLS provides libmbedcrypto
set(MBEDTLS_CONFIG_FILE "${TS_ROOT}/external/MbedTLS/config/crypto_posix.h"
	CACHE STRING "Configuration file for mbedcrypto")
include(${TS_ROOT}/external/MbedTLS/MbedTLS.cmake)
target_link_libraries(platform-inspect PRIVATE MbedTLS::mbedcrypto)

# Use Mbed TLS to provide the psa crypto api interface files
set(PSA_CRYPTO_API_INCLUDE "${MBEDTLS_PUBLIC_INCLUDE_PATH}")

# Qcbor
include(${TS_ROOT}/external/qcbor/qcbor.cmake)

# t_cose
include(${TS_ROOT}/external/t_cose/t_cose.cmake)
# Ensure correct order of libraries on the command line of LD. t_cose depends on qcbor thus
# qcbor must come later.
target_link_libraries(platform-inspect PRIVATE t_cose qcbor)

#-------------------------------------------------------------------------------
#  Define install content.
#
#-------------------------------------------------------------------------------
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "location to install build output to." FORCE)
endif()
install(TARGETS platform-inspect RUNTIME DESTINATION ${TS_ENV}/bin)
