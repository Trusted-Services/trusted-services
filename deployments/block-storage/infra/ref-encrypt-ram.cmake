#-------------------------------------------------------------------------------
# Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Lists components that provide an infrastructure layer for the block-storage
# service provider that uses a ram-backed block store, partitioned and encrypted
# by default using the 'ref' configuration. This infrastructure is intended for
# test purposes.
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Infrastructure components
#
#-------------------------------------------------------------------------------
add_components(TARGET "block-storage"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/service/block_storage/block_store/device"
		"components/service/block_storage/block_store/device/ram"
		"components/service/block_storage/block_store/partitioned"
		"components/service/block_storage/block_store/encrypted"
		"components/service/block_storage/config/ref"
		"components/service/block_storage/factory/ref_encrypt_ram"
		"components/service/crypto/backend/mbedcrypto/mbedtls_fake_external_get_random"
)

#-------------------------------------------------------------------------------
#  External project source-level dependencies
#
#-------------------------------------------------------------------------------
set(MBEDTLS_CONFIG_FILE "${TS_ROOT}/external/MbedTLS/config/blk_encrypt_config.h"
	CACHE STRING "Configuration file for Mbed TLS")
set(MBEDTLS_PSA_CRYPTO_CONFIG_FILE "${TS_ROOT}/external/MbedTLS/config/blk_encrypt_config_psa_aes_cbc_ecb_hkdf.h"
	CACHE STRING "PSA crypto config file for Mbed TLS")
include(${TS_ROOT}/external/MbedTLS/MbedTLS.cmake)
target_link_libraries(block-storage PRIVATE MbedTLS::mbedcrypto)

# Pass the location of the mbedtls config file to C preprocessor.
target_compile_definitions(block-storage PRIVATE
		MBEDTLS_CONFIG_FILE="${MBEDTLS_CONFIG_FILE}"
		MBEDTLS_PSA_CRYPTO_CONFIG_FILE="${MBEDTLS_PSA_CRYPTO_CONFIG_FILE}"
)
