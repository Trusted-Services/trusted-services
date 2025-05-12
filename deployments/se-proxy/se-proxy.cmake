#-------------------------------------------------------------------------------
# Copyright (c) 2021-2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# Check config options
foreach( _cfg IN ITEMS CFG_ENABLE_ITS CFG_ENABLE_PS CFG_ENABLE_CRYPTO CFG_ENABLE_IAT CFG_ENABLE_FWU)
	if( NOT DEFINED ${_cfg})
		message(FATAL_ERROR "Mandatory variable \"${_cfg}\" is not defined.")
	endif()
endforeach()

target_include_directories(se-proxy PRIVATE "${TOP_LEVEL_INCLUDE_DIRS}")

#-------------------------------------------------------------------------------
#  Service specific components
#
#-------------------------------------------------------------------------------
add_components(TARGET "se-proxy"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/common/tlv"
		"components/rpc/common/interface"
		"components/rpc/common/endpoint"
		"components/service/common/include"
		"components/service/common/client"
		"components/service/common/provider"
)

if (CFG_ENABLE_PS OR CFG_ENABLE_ITS)
add_components(TARGET "se-proxy"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/service/secure_storage/include"
		"components/service/secure_storage/frontend/secure_storage_provider"
		"protocols/service/secure_storage/packed-c"
)
endif()

if (CFG_ENABLE_CRYPTO)
add_components(TARGET "se-proxy"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/service/crypto/client/psa"
		"components/service/crypto/include"
		"components/service/crypto/provider"
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
)

if (CFG_ENABLE_CRYPTO_NANO)
add_components(TARGET "se-proxy"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/service/common/serializer/protobuf"
		"components/service/crypto/provider/serializer/protobuf"
		"protocols/service/crypto/protobuf"
)
endif()

endif()

if(CFG_ENABLE_IAT)
add_components(TARGET "se-proxy"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/service/attestation/include"
		"components/service/attestation/provider"
		"components/service/attestation/provider/serializer/packed-c"
		"protocols/rpc/common/packed-c"
)
endif()

if (CFG_ENABLE_FWU)
add_components(TARGET "se-proxy"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/service/fwu/common"
		"components/service/fwu/provider"
		"components/service/fwu/provider/serializer"
		"components/service/secure_storage/backend/secure_storage_ipc"
)
endif()
#-------------------------------------------------------------------------------
#  Components used from external projects
#
#-------------------------------------------------------------------------------

# Nanopb
if (CFG_ENABLE_CRYPTO_NANO)
	include(${TS_ROOT}/external/nanopb/nanopb.cmake)
	target_link_libraries(se-proxy PRIVATE nanopb::protobuf-nanopb-static)
	protobuf_generate_all(TGT "se-proxy" NAMESPACE "protobuf" BASE_DIR "${TS_ROOT}/protocols")
endif()

#################################################################
