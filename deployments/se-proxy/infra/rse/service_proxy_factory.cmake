#-------------------------------------------------------------------------------
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# A configurable A+M infrastructure for the se-proxy.
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Infrastructure components
#
#-------------------------------------------------------------------------------

# Check config options
foreach( _cfg IN ITEMS CFG_ENABLE_PS CFG_ENABLE_ITS CFG_ENABLE_CRYPTO CFG_ENABLE_CRYPTO_NANO CFG_ENABLE_IAT CFG_ENABLE_FWU)
	if( NOT DEFINED ${_cfg})
		message(FATAL_ERROR "Mandatory variable ${_cfg} is not defined.")
	endif()
endforeach()

# storage common
if (CFG_ENABLE_ITS OR CFG_ENABLE_PS)
add_components(TARGET "se-proxy"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/service/secure_storage/backend/secure_storage_ipc"
)
endif()

if (CFG_ENABLE_ITS)
target_sources(se-proxy PRIVATE
	${CMAKE_CURRENT_LIST_DIR}/spf_its.c
)
endif()

if (CFG_ENABLE_PS)
target_sources(se-proxy PRIVATE
	${CMAKE_CURRENT_LIST_DIR}/spf_ps.c
)
endif()

# crypto
if (CFG_ENABLE_CRYPTO)
add_components(TARGET "se-proxy"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/service/crypto/backend/psa_ipc"
)

target_sources(se-proxy PRIVATE
	${CMAKE_CURRENT_LIST_DIR}/spf_crypto.c
)

if (CFG_ENABLE_CRYPTO_NANO)
	set_property(SOURCE ${CMAKE_CURRENT_LIST_DIR}/spf_crypto.c APPEND PROPERTY COMPILE_DEFINITIONS "CFG_ENABLE_CRYPTO_NANO")
endif()

endif()

# initial attestation
if (CFG_ENABLE_IAT)
add_components(TARGET "se-proxy"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/service/attestation/client/psa_ipc"
		"components/service/attestation/key_mngr/local"
		"components/service/attestation/reporter/psa_ipc"
)

target_sources(se-proxy PRIVATE
	${CMAKE_CURRENT_LIST_DIR}/spf_iat.c
)
endif()

# fwu
if (CFG_ENABLE_FWU)
add_components(TARGET "se-proxy"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
	"components/service/fwu/common"
	"components/service/fwu/provider"
	"components/service/fwu/provider/serializer"
	"components/service/fwu/psa_fwu_m/agent"
	"components/service/fwu/psa_fwu_m/interface/psa_ipc"
)

target_sources(se-proxy PRIVATE
	${CMAKE_CURRENT_LIST_DIR}/spf_fwu.c
)
endif()
