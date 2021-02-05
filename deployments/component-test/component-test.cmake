#-------------------------------------------------------------------------------
# Copyright (c) 2020-2021, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#  The base build file shared between deployments of 'component-test' for
#  different environments.  Used for running standalone component tests
#  contained within a single executable.
#-------------------------------------------------------------------------------
add_components(
	TARGET "component-test"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/app/ts-demo"
		"components/app/ts-demo/test"
		"components/common/uuid"
		"components/common/uuid/test"
		"components/common/tlv"
		"components/common/tlv/test"
		"components/config/ramstore"
		"components/config/ramstore/test"
		"components/rpc/common/caller"
		"components/rpc/common/interface"
		"components/rpc/common/test/protocol"
		"components/rpc/direct"
		"components/rpc/dummy"
		"components/service/common"
		"components/service/common/serializer/protobuf"
		"components/service/common/provider"
		"components/service/common/provider/test"
		"components/service/locator"
		"components/service/locator/interface"
		"components/service/locator/test"
		"components/service/locator/standalone"
		"components/service/locator/standalone/services/crypto"
		"components/service/locator/standalone/services/test-runner"
		"components/service/crypto/client/cpp"
		"components/service/crypto/client/cpp/protobuf"
		"components/service/crypto/client/cpp/packed-c"
		"components/service/crypto/client/test"
		"components/service/crypto/client/test/standalone"
		"components/service/crypto/provider/mbedcrypto"
		"components/service/crypto/provider/serializer/protobuf"
		"components/service/crypto/provider/serializer/packed-c"
		"components/service/crypto/test/unit"
		"components/service/crypto/test/service"
		"components/service/crypto/test/service/protobuf"
		"components/service/crypto/test/service/packed-c"
		"components/service/crypto/test/protocol"
		"components/service/secure_storage/client/psa"
		"components/service/secure_storage/provider/mock_store"
		"components/service/secure_storage/provider/secure_flash_store"
		"components/service/secure_storage/provider/secure_flash_store/flash_fs"
		"components/service/secure_storage/provider/secure_flash_store/flash"
		"components/service/secure_storage/test"
		"components/service/test_runner/provider"
		"components/service/test_runner/provider/serializer/packed-c"
		"components/service/test_runner/provider/backend/null"
		"protocols/rpc/common/protobuf"
		"protocols/rpc/common/packed-c"
		"protocols/service/crypto/packed-c"
		"protocols/service/crypto/protobuf"
		"protocols/service/secure_storage/packed-c"
)

#-------------------------------------------------------------------------------
#  Components used from external projects
#
#-------------------------------------------------------------------------------

# Nanopb
include(${TS_ROOT}/external/nanopb/nanopb.cmake)
target_link_libraries(component-test PRIVATE nanopb::protobuf-nanopb-static)
protobuf_generate_all(TGT "component-test" NAMESPACE "protobuf" BASE_DIR "${TS_ROOT}/protocols")

# Mbedcrypto
include(${TS_ROOT}/external/mbed-crypto/mbedcrypto.cmake)
target_link_libraries(component-test PRIVATE mbedcrypto)

#-------------------------------------------------------------------------------
#  Define install content.
#
#-------------------------------------------------------------------------------
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "location to install build output to." FORCE)
endif()
install(TARGETS component-test DESTINATION ${TS_ENV}/bin)
