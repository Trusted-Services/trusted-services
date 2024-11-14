#-------------------------------------------------------------------------------
# Copyright (c) 2020-2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# Check mandatory variables.
foreach(_var IN ITEMS TGT TRACE_PREFIX SP_HEAP_SIZE SP_STACK_SIZE SP_FFA_UUID_CANON)
	if (NOT DEFINED ${_var})
		message(FATAL_ERROR "Mandatory parameter '${_var}' missing.")
	endif()
endforeach()

# Ensure elf output naming is symbolize.py compatible.
# If binary UUID is not defined, fall back to using the SP UUID value.
if (NOT SP_BIN_UUID_CANON)
	set(SP_BIN_UUID_CANON "${SP_FFA_UUID_CANON}")
endif()
ts_add_uuid_to_exe_name(TGT "${TGT}" UUID "${SP_BIN_UUID_CANON}" )

target_sources(${TGT} PRIVATE
	"${CMAKE_CURRENT_LIST_DIR}/entry.S"
	"${CMAKE_CURRENT_LIST_DIR}/sp_assert.c"
	"${CMAKE_CURRENT_LIST_DIR}/sp_entry.c"
	"${CMAKE_CURRENT_LIST_DIR}/sp_trace.c"
)

# Default trace level configuration, can be overwritten by setting the same
# variable in the deployment specific file before including this file.
set(TRACE_LEVEL "TRACE_LEVEL_ERROR" CACHE STRING "Trace level")

target_compile_definitions(${TGT} PRIVATE
	TRACE_LEVEL=${TRACE_LEVEL}
	TRACE_PREFIX="${TRACE_PREFIX}"
	SP_HEAP_SIZE=${SP_HEAP_SIZE}
)

add_subdirectory(${TS_ROOT}/components/common/libc ${CMAKE_BINARY_DIR}/libc_build)
add_components(TARGET ${TGT}
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/common/dlmalloc"
)

target_link_libraries(${TGT} PRIVATE
	stdlib::c
)

target_link_options(${TGT} PRIVATE
	-Wl,--hash-style=sysv
	-Wl,--as-needed
	-Wl,--gc-sections
)

compiler_set_linker_script(TARGET ${TGT} FILE ${CMAKE_CURRENT_LIST_DIR}/sp.ld.S DEF ARM64=1 SP_STACK_SIZE=${SP_STACK_SIZE})
