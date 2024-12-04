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
	"${CMAKE_CURRENT_LIST_DIR}/block_store_factory.c"
	)

# If none already defined, make this the default factory for the deployment
if (NOT DEFINED TS_BLOCK_STORE_FACTORY)
	set(TS_BLOCK_STORE_FACTORY "ref_encrypt_ram_block_store_factory")
	target_compile_definitions(${TGT} PRIVATE
		CONCRETE_BLOCK_STORE_FACTORY=${TS_BLOCK_STORE_FACTORY})
endif()