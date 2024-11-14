#-------------------------------------------------------------------------------
# Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
if (NOT DEFINED TGT)
	message(FATAL_ERROR "mandatory parameter TGT is not defined.")
endif()

set_property(TARGET ${TGT} APPEND PROPERTY PUBLIC_HEADER
	${CMAKE_CURRENT_LIST_DIR}/caller/sp/rse_comms_caller.h
	)

target_sources(${TGT} PRIVATE
	"${CMAKE_CURRENT_LIST_DIR}/rse_comms.c"
	"${CMAKE_CURRENT_LIST_DIR}/caller/sp/rse_comms_caller.c"
	"${CMAKE_CURRENT_LIST_DIR}/caller/sp/rse_comms_protocol.c"
	"${CMAKE_CURRENT_LIST_DIR}/caller/sp/rse_comms_protocol_embed.c"
	"${CMAKE_CURRENT_LIST_DIR}/caller/sp/rse_comms_protocol_pointer_access.c"
	)

target_include_directories(${TGT}
	 PUBLIC
		"${CMAKE_CURRENT_LIST_DIR}/caller/sp/"
	)

