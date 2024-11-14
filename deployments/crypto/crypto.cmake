#-------------------------------------------------------------------------------
# Copyright (c) 2020-2023, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

add_components(TARGET "crypto"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/common/mbedtls"
		"components/common/tlv"
		"components/rpc/common/interface"
		"components/rpc/common/endpoint"
		"components/service/common/include"
		"components/service/common/client"
		"components/service/common/serializer/protobuf"
		"components/service/common/provider"
		"components/service/crypto/provider"
		"components/service/crypto/provider/serializer/protobuf"
		"components/service/crypto/provider/serializer/packed-c"
		"components/service/crypto/provider/extension/hash"
		"components/service/crypto/provider/extension/hash/serializer/packed-c"
		"components/service/crypto/provider/extension/cipher"
		"components/service/crypto/provider/extension/cipher/serializer/packed-c"
		"components/service/crypto/provider/extension/key_derivation"
		"components/service/crypto/provider/extension/key_derivation/serializer/packed-c"
		"components/service/crypto/provider/extension/mac"
		"components/service/crypto/provider/extension/mac/serializer/packed-c"
		"components/service/crypto/provider/extension/aead"
		"components/service/crypto/provider/extension/aead/serializer/packed-c"
		"components/service/crypto/factory/full"
		"components/service/crypto/backend/mbedcrypto"
		"components/service/crypto/backend/mbedcrypto/mbedtls_fake_x509"
		"protocols/rpc/common/packed-c"
		"protocols/service/crypto/protobuf"
)

#-------------------------------------------------------------------------------
#  Components used from external projects
#
#-------------------------------------------------------------------------------

# Nanopb
include(${TS_ROOT}/external/nanopb/nanopb.cmake)
target_link_libraries(crypto PRIVATE nanopb::protobuf-nanopb-static)
protobuf_generate_all(TGT "crypto" NAMESPACE "protobuf" BASE_DIR "${TS_ROOT}/protocols")

# Mbed TLS provides libmbedcrypto
set(MBEDTLS_CONFIG_FILE "${TS_ROOT}/external/MbedTLS/config/crypto_provider_x509.h"
	CACHE STRING "Configuration file for Mbed TLS" FORCE)
include(${TS_ROOT}/external/MbedTLS/MbedTLS.cmake)
target_link_libraries(crypto PRIVATE MbedTLS::mbedcrypto)
target_link_libraries(crypto PRIVATE MbedTLS::mbedx509)

# Provide the config path to mbedtls
target_compile_definitions(crypto
	PRIVATE
		MBEDTLS_CONFIG_FILE="${MBEDTLS_CONFIG_FILE}"
)

#################################################################

target_include_directories(crypto PRIVATE
	${TS_ROOT}
	${TS_ROOT}/components
)
