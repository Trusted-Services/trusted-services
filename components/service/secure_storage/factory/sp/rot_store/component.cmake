#-------------------------------------------------------------------------------
# Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
if (NOT DEFINED TGT)
	message(FATAL_ERROR "mandatory parameter TGT is not defined.")
endif()

if(DEFINED CFG_STORAGE_CLIENT_RPC_BUF_SIZE)
	target_compile_definitions(${TGT} PRIVATE STORAGE_CLIENT_RPC_BUF_SIZE=${CFG_STORAGE_CLIENT_RPC_BUF_SIZE})
endif()

target_sources(${TGT} PRIVATE
	"${CMAKE_CURRENT_LIST_DIR}/storage_factory.c"
	)
