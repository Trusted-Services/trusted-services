#-------------------------------------------------------------------------------
# Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#  The base build file shared between deployments of 'ts-service-test' for
#  different environments.  Used for running end-to-end service-level tests
#  where test cases exercise trusted service client interfaces.
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#  Use libts for locating and accessing services. An appropriate version of
#  libts will be imported for the environment in which service tests are
#  deployed.
#-------------------------------------------------------------------------------
if (COVERAGE)
	set(LIBTS_BUILD_TYPE "DEBUGCOVERAGE" CACHE STRING "Libts build type" FORCE)
endif()

include(${TS_ROOT}/deployments/libts/libts-import.cmake)
target_link_libraries(ts-service-test PRIVATE libts::ts)

#-------------------------------------------------------------------------------
#  Components that are common across all deployments
#
#-------------------------------------------------------------------------------
add_components(
	TARGET "ts-service-test"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/common/endian"
		"components/common/tlv"
		"components/common/uuid"
		"components/service/common/client"
		"components/service/common/include"
		"components/service/crypto/include"
		"components/service/crypto/test/service"
		"components/service/crypto/test/service/protobuf"
		"components/service/crypto/test/service/packed-c"
		"components/service/crypto/test/service/extension/hash"
		"components/service/crypto/test/service/extension/hash/packed-c"
		"components/service/crypto/test/service/extension/cipher"
		"components/service/crypto/test/service/extension/cipher/packed-c"
		"components/service/crypto/test/service/extension/mac"
		"components/service/crypto/test/service/extension/mac/packed-c"
		"components/service/crypto/test/service/extension/key_derivation"
		"components/service/crypto/test/service/extension/key_derivation/packed-c"
		"components/service/crypto/client/psa"
		"components/service/crypto/client/cpp"
		"components/service/crypto/client/cpp/protocol/protobuf"
		"components/service/crypto/client/cpp/protocol/packed-c"
		"components/service/common/serializer/protobuf"
		"components/service/attestation/include"
		"components/service/attestation/client/psa"
		"components/service/attestation/client/provision"
		"components/service/attestation/test/service"
		"components/service/rpmb/backend"
		"components/service/rpmb/client"
		"components/service/rpmb/frontend"
		"components/service/rpmb/frontend/platform/default"
		"protocols/service/crypto/protobuf"
		"protocols/service/crypto/packed-c"
		"components/service/secure_storage/include"
		"components/service/secure_storage/test/service"
		"components/service/secure_storage/frontend/psa/its"
		"components/service/secure_storage/frontend/psa/its/test"
		"components/service/secure_storage/frontend/psa/ps"
		"components/service/secure_storage/frontend/psa/ps/test"
		"components/service/secure_storage/backend/secure_storage_client"
		"components/service/fwu/test/service"
		"components/service/fwu/test/fwu_client/remote"
		"components/service/fwu/test/fwu_dut"
		"components/service/fwu/test/fwu_dut/proxy"
		"components/service/fwu/test/image_directory_checker"
		"components/service/fwu/test/metadata_checker"
		"components/service/fwu/test/metadata_fetcher/client"
)

#-------------------------------------------------------------------------------
#  Component configurations
#
#-------------------------------------------------------------------------------
target_compile_definitions(ts-service-test PRIVATE
	"TRACE_PREFIX=\"TEST\""
	"TRACE_LEVEL=0"
)

#-------------------------------------------------------------------------------
#  Components used from external projects
#
#-------------------------------------------------------------------------------

# Nanopb
include(${TS_ROOT}/external/nanopb/nanopb.cmake)
target_link_libraries(ts-service-test PRIVATE nanopb::protobuf-nanopb-static)
protobuf_generate_all(TGT "ts-service-test" NAMESPACE "protobuf" BASE_DIR "${TS_ROOT}/protocols")

#-------------------------------------------------------------------------------
#  Define install content.
#
#-------------------------------------------------------------------------------
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "location to install build output to." FORCE)
endif()
install(TARGETS ts-service-test RUNTIME DESTINATION ${TS_ENV}/bin)
