#-------------------------------------------------------------------------------
# Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#-------------------------------------------------------------------------------

target_sources(${TGT} PRIVATE
	"${CMAKE_CURRENT_LIST_DIR}/malloc_wrapper.c"
)

# TODO: remove workaround
set_source_files_properties(
	"${CMAKE_CURRENT_LIST_DIR}/malloc_wrapper.c" PROPERTIES COMPILE_FLAGS
	"-fno-builtin"
)
