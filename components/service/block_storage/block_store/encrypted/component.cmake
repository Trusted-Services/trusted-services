#-------------------------------------------------------------------------------
# Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
if (NOT DEFINED TGT)
	message(FATAL_ERROR "mandatory parameter TGT is not defined.")
endif()

target_sources(${TGT} PRIVATE
	"${CMAKE_CURRENT_LIST_DIR}/encrypted_block_store.c"
	)

set(ENCRYPTED_BLK_AES_KEY_BITS "256" CACHE STRING "AES Key length: 128, 192 or 256 bits are supported")
set(ENCRYPTED_BLK_BLOCK_ENCRYPTION_ROOT_KEY
	"{0x32, 0x2b, 0x78, 0x27, 0xa3, 0x08, 0xcb, 0x5e, 0xb4, 0x12, 0x0b, 0xab, 0x96, 0xd4, 0x3d, 0x4e, 0x7b, 0xc4, 0x46, 0x46, 0xad, 0x93, 0xe9, 0x03, 0x28, 0x47, 0xe8, 0xb6, 0x2c, 0xec, 0x5f, 0x14}"
	CACHE STRING "Root key to derive the essiv and encryption/decryption keys from"
)
set(ENCRYPTED_BLK_BLOCK_ENCRYPTION_SALT
	"{0xcf, 0x9e, 0x66, 0xf1, 0x96, 0x91, 0x2d, 0x78, 0x50, 0xa7, 0x18, 0x32, 0x1f, 0x48, 0x50, 0x94}"
	CACHE STRING "Salt value for the key derivations"
)

target_compile_definitions(${TGT} PRIVATE
    BLK_AES_KEY_BITS=${ENCRYPTED_BLK_AES_KEY_BITS}
    BLOCK_ENCRYPTION_ROOT_KEY=${ENCRYPTED_BLK_BLOCK_ENCRYPTION_ROOT_KEY}
	BLOCK_ENCRYPTION_SALT=${ENCRYPTED_BLK_BLOCK_ENCRYPTION_SALT}
)
