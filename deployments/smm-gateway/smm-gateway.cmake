#-------------------------------------------------------------------------------
# Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

option(UEFI_INTERNAL_CRYPTO "Use internal mbedtls instance" OFF)

if (UEFI_AUTH_VAR)

# If enabled an internal mbedtls instance will be used instead of the crypto SP
if (UEFI_INTERNAL_CRYPTO)
set(MBEDTLS_CONFIG_FILE "${TS_ROOT}/external/MbedTLS/config/internal_crypto_smmgw.h"
	CACHE STRING "Configuration file for Mbed TLS" FORCE)
include(${TS_ROOT}/external/MbedTLS/MbedTLS.cmake)
target_link_libraries(smm-gateway PRIVATE MbedTLS::mbedcrypto)
target_link_libraries(smm-gateway PRIVATE MbedTLS::mbedx509)

# Pass the mbedtls config file to C preprocessor so the uefi
# direct backend will access the mbedtls headers
target_compile_definitions(smm-gateway PRIVATE
	MBEDTLS_CONFIG_FILE="${MBEDTLS_CONFIG_FILE}"
)

target_compile_definitions(smm-gateway PRIVATE
	-DUEFI_INTERNAL_CRYPTO
)

add_components(TARGET "smm-gateway"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/common/mbedtls"
		"components/service/uefi/smm_variable/backend/direct"
		"components/service/crypto/backend/mbedcrypto/trng_adapter/stub"
)

else()
add_components(TARGET "smm-gateway"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/common/tlv"
		"components/service/crypto/include"
		"components/service/crypto/client/psa"
)
endif()

endif()

add_components(TARGET "smm-gateway"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/common/trace"
		"components/common/utils"
		"components/common/uuid"
		"components/rpc/common/interface"
		"components/service/common/include"
		"components/service/common/provider"
		"components/service/uefi/smm_variable/backend"
		"components/service/uefi/smm_variable/provider"
		"components/service/secure_storage/include"
		"components/service/secure_storage/backend/mock_store"
		"protocols/rpc/common/packed-c"
)

target_include_directories(smm-gateway PRIVATE
	${TS_ROOT}
	${TS_ROOT}/components
)
