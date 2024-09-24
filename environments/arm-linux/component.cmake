#-------------------------------------------------------------------------------
# Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
if (NOT DEFINED TGT)
	message(FATAL_ERROR "mandatory parameter TGT is not defined.")
endif()

if (NOT DEFINED TRACE_PREFIX)
	message(FATAL_ERROR "mandatory parameter TRACE_PREFIX is not defined.")
endif()

# Default to trace output disabled
set(TRACE_LEVEL "TRACE_LEVEL_NONE" CACHE STRING "Trace level")

target_compile_definitions(${TGT} PRIVATE
	TRACE_LEVEL=${TRACE_LEVEL}
	TRACE_PREFIX="${TRACE_PREFIX}"
)

target_sources(${TGT} PRIVATE
	"${CMAKE_CURRENT_LIST_DIR}/posix_trace.c"
	)
