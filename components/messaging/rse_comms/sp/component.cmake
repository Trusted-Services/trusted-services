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
	"${CMAKE_CURRENT_LIST_DIR}/rse_comms_messenger_api.h"
	)

target_sources(${TGT} PRIVATE
	"${CMAKE_CURRENT_LIST_DIR}/rse_comms_messenger.c"
	"${CMAKE_CURRENT_LIST_DIR}/rse_comms_platform.c"
	)

target_include_directories(${TGT}
	 PUBLIC
		"${CMAKE_CURRENT_LIST_DIR}"
	)

set_property(TARGET ${TGT} APPEND PROPERTY TS_PLATFORM_DRIVER_DEPENDENCIES
	"mhu"
)
