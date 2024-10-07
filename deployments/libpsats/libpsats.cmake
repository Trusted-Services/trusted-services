#-------------------------------------------------------------------------------
# Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#  The base build file shared between deployments of 'libpsats' for different
#  environments.  libpsats provides an interface for accessing PSA API-s.
#  Building with each build type results in different postfix for the library.
#  For details, please refer to deployment.cmake.
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#  Common API version implemented by all libpsats deployments
#-------------------------------------------------------------------------------
version_semver_read(FILE "${CMAKE_CURRENT_LIST_DIR}/version.txt"
					MAJOR _major MINOR _minor PATCH _patch)
set_target_properties(psats PROPERTIES VERSION "${_major}.${_minor}.${_patch}")
set_target_properties(psats PROPERTIES SOVERSION "${_major}")
unset(_major)
unset(_minor)
unset(_patch)

add_library(libpsats::psats ALIAS psats)

if (COVERAGE)
	set(LIBPSATS_BUILD_TYPE "DebugCoverage" CACHE STRING "Build type." FORCE)
endif()

#-------------------------------------------------------------------------------
#  Use libts for locating and accessing services. An appropriate version of
#  libts will be imported for the environment in which service tests are
#  deployed.
#-------------------------------------------------------------------------------
include(${TS_ROOT}/deployments/libts/libts-import.cmake)
target_link_libraries(psats PUBLIC libts::ts)

#-------------------------------------------------------------------------------
#  Components that are common across all deployments
#
#-------------------------------------------------------------------------------

add_components(
	TARGET "psats"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"environments/${TS_ENV}"
		"components/common/utils"
		"components/common/trace"
		"components/common/libpsats"
		"components/common/tlv"
		"components/service/common/include"
		"components/service/common/client"
		"components/service/crypto/include"
		"components/service/crypto/client/psa"
		"components/service/attestation/include"
		"components/service/attestation/client/psa"
		"components/service/attestation/client/provision"
		"components/service/secure_storage/include"
		"components/service/secure_storage/frontend/psa/its"
		"components/service/secure_storage/frontend/psa/ps"
		"components/service/secure_storage/backend/secure_storage_client"
)

#-------------------------------------------------------------------------------
#  Define public interfaces for library
#
#-------------------------------------------------------------------------------

# Enable exporting interface symbols for library public interface
target_compile_definitions(psats PRIVATE
	EXPORT_PUBLIC_INTERFACE_LIBPSATS
	EXPORT_PUBLIC_INTERFACE_PSA_CRYPTO
	EXPORT_PUBLIC_INTERFACE_PSA_ATTEST
	EXPORT_PUBLIC_INTERFACE_PSA_ITS
	EXPORT_PUBLIC_INTERFACE_PSA_PS
)

#-------------------------------------------------------------------------------
#  Export the library and the corresponding public interface header files
#
#-------------------------------------------------------------------------------
include(${TS_ROOT}/tools/cmake/common/ExportLibrary.cmake REQUIRED)

# Exports library information in preparation for install
export_library(
	TARGET "psats"
	LIB_NAME "libpsats"
	PKG_CONFIG_FILE "${CMAKE_CURRENT_LIST_DIR}/libpsatsConfig.cmake.in"
)

install(DIRECTORY "${TS_ROOT}/components/service/crypto/include"
		DIRECTORY "${TS_ROOT}/components/service/attestation/include"
		DIRECTORY "${TS_ROOT}/components/service/secure_storage/include"
		DESTINATION "${TS_ENV}"
		FILES_MATCHING PATTERN "*.h"
)

install(FILES "${TS_ROOT}/components/service/common/include/psa/error.h"
		DESTINATION ${TS_ENV}/include/psa
)

install(FILES "${TS_ROOT}/components/common/libpsats/libpsats.h"
		DESTINATION ${TS_ENV}/include
)
