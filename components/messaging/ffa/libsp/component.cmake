#-------------------------------------------------------------------------------
# Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
if (NOT DEFINED TGT)
	message(FATAL_ERROR "mandatory parameter TGT is not defined.")
endif()

set(FFA_DIRECT_MSG_ROUTING_EXTENSION ON CACHE BOOL "Enable FF-A direct message routing extension")
set(CFG_FFA_VERSION 0x00010001 CACHE STRING "The supported FF-A protocol's version: (major << 16) | minor")

target_sources(${TGT} PRIVATE
	"${CMAKE_CURRENT_LIST_DIR}/aarch64/ffa_syscalls_a64.S"
	"${CMAKE_CURRENT_LIST_DIR}/ffa.c"
	"${CMAKE_CURRENT_LIST_DIR}/ffa_interrupt_handler.c"
	"${CMAKE_CURRENT_LIST_DIR}/ffa_memory_descriptors.c"
	"${CMAKE_CURRENT_LIST_DIR}/sp_discovery.c"
	"${CMAKE_CURRENT_LIST_DIR}/sp_memory_management.c"
	"${CMAKE_CURRENT_LIST_DIR}/sp_messaging.c"
	"${CMAKE_CURRENT_LIST_DIR}/sp_rxtx.c"
	)

set_property(TARGET ${TGT} APPEND PROPERTY PUBLIC_HEADER
	${CMAKE_CURRENT_LIST_DIR}/include/ffa_api.h
	${CMAKE_CURRENT_LIST_DIR}/include/ffa_api_defines.h
	${CMAKE_CURRENT_LIST_DIR}/include/ffa_api_types.h
	${CMAKE_CURRENT_LIST_DIR}/include/ffa_internal_api.h
	${CMAKE_CURRENT_LIST_DIR}/include/ffa_memory_descriptors.h
	${CMAKE_CURRENT_LIST_DIR}/include/sp_api.h
	${CMAKE_CURRENT_LIST_DIR}/include/sp_api_defines.h
	${CMAKE_CURRENT_LIST_DIR}/include/sp_api_types.h
	${CMAKE_CURRENT_LIST_DIR}/include/sp_discovery.h
	${CMAKE_CURRENT_LIST_DIR}/include/sp_memory_management.h
	${CMAKE_CURRENT_LIST_DIR}/include/sp_messaging.h
	${CMAKE_CURRENT_LIST_DIR}/include/sp_rxtx.h
	)

if (FFA_DIRECT_MSG_ROUTING_EXTENSION)
	target_sources(${TGT} PRIVATE
		"${CMAKE_CURRENT_LIST_DIR}/ffa_direct_msg_routing_extension.c"
		)

	target_compile_options(${TGT} PUBLIC
		-DFFA_DIRECT_MSG_ROUTING_EXTENSION=1
		)
endif()

target_compile_definitions(${TGT}
	PUBLIC
		"CFG_FFA_VERSION=${CFG_FFA_VERSION}"
	)

target_include_directories(${TGT}
	 PUBLIC
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
		"$<INSTALL_INTERFACE:${TS_ENV}/include>"
	)
