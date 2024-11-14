#-------------------------------------------------------------------------------
# Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# Use libfdt path from the environment if defined
if (DEFINED ENV{TS_LIBFDT_PATH})
	set(LIBFDT_PATH $ENV{TS_LIBFDT_PATH} CACHE PATH "Location of libfdt" FORCE)
endif()

# Otherwise clone the dtc repo (which contains libfdt)
set(DTC_URL "https://github.com/dgibson/dtc" CACHE STRING "dtc repository URL")
set(DTC_REFSPEC "v1.6.1" CACHE STRING "dtc git refspec")
set(DTC_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps/dtc-src" CACHE PATH "Location of dtc source")

set(GIT_OPTIONS
	GIT_REPOSITORY ${DTC_URL}
	GIT_TAG ${DTC_REFSPEC}
)

include(${TS_ROOT}/tools/cmake/common/LazyFetch.cmake REQUIRED)
LazyFetch_MakeAvailable(
	DEP_NAME dtc
	FETCH_OPTIONS "${GIT_OPTIONS}"
	SOURCE_DIR ${DTC_SOURCE_DIR}
)

find_path(LIBFDT_PATH
	NAMES fdt.c
	PATHS ${DTC_SOURCE_DIR}/libfdt
	NO_DEFAULT_PATH
	REQUIRED
	DOC "Location of libfdt"
)

# Add libfdt source files to the target
if (NOT DEFINED TGT)
	message(FATAL_ERROR "mandatory parameter TGT is not defined.")
endif()

target_sources(${TGT} PRIVATE
	"${LIBFDT_PATH}/fdt.c"
	"${LIBFDT_PATH}/fdt_ro.c"
	"${LIBFDT_PATH}/fdt_wip.c"
	"${LIBFDT_PATH}/fdt_sw.c"
	"${LIBFDT_PATH}/fdt_rw.c"
	"${LIBFDT_PATH}/fdt_strerror.c"
	"${LIBFDT_PATH}/fdt_empty_tree.c"
	"${LIBFDT_PATH}/fdt_addresses.c"
	"${LIBFDT_PATH}/fdt_overlay.c"
	"${LIBFDT_PATH}/fdt_check.c"
)

target_include_directories(${TGT} PRIVATE ${LIBFDT_PATH})
