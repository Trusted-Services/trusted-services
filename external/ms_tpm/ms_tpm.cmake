#-------------------------------------------------------------------------------
# Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

set(MS_TPM_URL "https://github.com/microsoft/ms-tpm-20-ref.git" CACHE STRING "MS TPM repository URL")
set(MS_TPM_REFSPEC "e9fc7b89d865536c46deb63f9c7d0121a3ded49c" CACHE STRING "MS TPM git refspec")
set(MS_TPM_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps/ms_tpm-src" CACHE PATH "MS TPM source directory")
set(MS_TPM_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/ms_tpm-install" CACHE PATH "MS TPM install directory")
set(MS_TPM_BUILD_TYPE "Debug" CACHE STRING "MS TPM build type")

# Add Mbed TLS
set(MBEDTLS_CONFIG_FILE "${TS_ROOT}/external/MbedTLS/config/ms_tpm_config.h" CACHE STRING "" FORCE)
include(${TS_ROOT}/external/MbedTLS/MbedTLS.cmake)

# Pass include paths to ms_tpm
get_target_property(_mbedcrypto_includes MbedTLS::mbedcrypto INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(_mbedx509_includes MbedTLS::mbedx509 INTERFACE_INCLUDE_DIRECTORIES)
set(TPM_EXTRA_INCLUDES
	# MbedTLS includes
	"${_mbedcrypto_includes}"
	"${_mbedx509_includes}"
	# TS utilities
	"${TS_ROOT}/components/common/trace/include"
	"${TS_ROOT}/components/common/utils/include"
	# PSA headers
	"${TS_ROOT}/components/service/common/include"
	"${TS_ROOT}/components/service/secure_storage/include"
	CACHE STRING "" FORCE)

set(TPM_EXTRA_DEFINITIONS
	# TPM config
	_ARM_
	GCC
	HASH_LIB=Mbed
	SYM_LIB=Mbed
	MATH_LIB=Mbed
	SIMULATION=NO
	VTPM=NO
	ECC_NIST_P521=YES # if not defined, the TPM internal bignum allocation won't be large enough
	CERTIFYX509_DEBUG=NO

	# Pass MbedTLS config
	MBEDTLS_CONFIG_FILE="${MBEDTLS_CONFIG_FILE}"

	# Pass TS trace config
	TRACE_LEVEL=${TRACE_LEVEL}
	TRACE_PREFIX="${TRACE_PREFIX}"
	CACHE STRING "" FORCE)

# Only pass libc settings to ms_tpm if needed. For environments where the standard library is not
# overridden, this is not needed.
if(TARGET stdlib::c)
	include(${TS_ROOT}/tools/cmake/common/PropertyCopy.cmake)
	# Save libc settings
	save_interface_target_properties(TGT stdlib::c PREFIX LIBC)
	# Translate libc settings to CMake code fragment. Will be inserted into
	# mbedtls-init-cache.cmake.in when LazyFetch configures the file.
	translate_interface_target_properties(PREFIX LIBC RES _cmake_fragment)
	unset_saved_properties(LIBC)
endif()

set(GIT_OPTIONS
	GIT_REPOSITORY ${MS_TPM_URL}
	GIT_TAG ${MS_TPM_REFSPEC}
	GIT_SHALLOW TRUE
	PATCH_COMMAND
		git stash
		COMMAND git branch -f bf-am
		COMMAND git am ${CMAKE_CURRENT_LIST_DIR}/0001-Add-MbedTLS-crypto-port.patch
		COMMAND git am ${CMAKE_CURRENT_LIST_DIR}/0002-Add-PSA-platform-port.patch
		COMMAND git am ${CMAKE_CURRENT_LIST_DIR}/0003-Add-CMake-support.patch
		COMMAND git reset bf-am
)

include(${TS_ROOT}/tools/cmake/common/LazyFetch.cmake REQUIRED)
LazyFetch_MakeAvailable(DEP_NAME ms_tpm
	FETCH_OPTIONS ${GIT_OPTIONS}
	INSTALL_DIR ${MS_TPM_INSTALL_DIR}
	PACKAGE_DIR ${MS_TPM_INSTALL_DIR}
	CACHE_FILE "${TS_ROOT}/external/ms_tpm/ms_tpm-init-cache.cmake.in"
	SOURCE_DIR "${MS_TPM_SOURCE_DIR}"
)
unset(_cmake_fragment)

target_link_libraries(ms_tpm::tpm INTERFACE MbedTLS::mbedcrypto)
target_link_libraries(ms_tpm::tpm INTERFACE MbedTLS::mbedx509)
