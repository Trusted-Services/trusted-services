#-------------------------------------------------------------------------------
# Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
if (NOT DEFINED TGT)
	message(FATAL_ERROR "mandatory parameter TGT is not defined.")
endif()

set(CFG_FFA_VERSION 0x00010001 CACHE STRING "The supported FF-A protocol's version: (major << 16) | minor")

target_sources(${TGT} PRIVATE
	"${CMAKE_CURRENT_LIST_DIR}/mock_assert.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/mock_ffa_api.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/mock_ffa_internal_api.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/mock_sp_discovery.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/mock_sp_memory_management.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/mock_sp_messaging.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/mock_sp_rxtx.cpp"
	)

target_include_directories(${TGT}
	PUBLIC
		${CMAKE_CURRENT_LIST_DIR}
		${CMAKE_CURRENT_LIST_DIR}/../include
)

target_compile_definitions(${TGT}
	PUBLIC
		"ARM64=1"
		"CFG_FFA_VERSION=${CFG_FFA_VERSION}"
)