#-------------------------------------------------------------------------------
# Copyright (c) 2020-2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# Check mandatory variables.
foreach(_var IN ITEMS TGT)
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

get_target_property(_tgt_type ${TGT} TYPE)
if ("${_tgt_type}" STREQUAL "EXECUTABLE")
	compiler_generate_stripped_elf(TARGET ${TGT} NAME "${SP_BIN_UUID_CANON}.stripped.elf" RES STRIPPED_ELF)
	install(FILES ${STRIPPED_ELF} DESTINATION ${TS_ENV}/bin)

	# Get the name of the SP.
	get_target_property(_tgt_name ${TGT} NAME )
	set(SP_NAME "${_tgt_name}" CACHE STRING "Name of the SP.")

	include(${TS_ROOT}/tools/cmake/common/TargetCompileDefinitions.cmake)
	set_target_uuids(
		SP_UUID ${SP_FFA_UUID_CANON}
		TGT ${SP_NAME}
	)

endif()

target_sources(${TGT} PRIVATE
	"${CMAKE_CURRENT_LIST_DIR}/optee_sp_header.c"
	"${CMAKE_CURRENT_LIST_DIR}/sp_assert.c"
	"${CMAKE_CURRENT_LIST_DIR}/sp_entry.c"
	"${CMAKE_CURRENT_LIST_DIR}/sp_trace.c"
)

target_include_directories(${TGT}
	PUBLIC
		"${CMAKE_CURRENT_LIST_DIR}/include"
	)

# Default trace configuration, can be overwritten by setting the same variables
# in the deployment specific file before including this file.
set(TRACE_PREFIX "SP" CACHE STRING "Trace prefix")
set(TRACE_LEVEL "TRACE_LEVEL_ERROR" CACHE STRING "Trace level")

if (NOT DEFINED SP_HEAP_SIZE)
	message(FATAL_ERROR "SP_HEAP_SIZE is not defined")
endif()

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

compiler_set_linker_script(TARGET ${TGT} FILE ${CMAKE_CURRENT_LIST_DIR}/sp.ld.S DEF ARM64=1)
