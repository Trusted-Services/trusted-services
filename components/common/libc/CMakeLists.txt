#-------------------------------------------------------------------------------
# Copyright (c) 2024, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(c LANGUAGES C)

# Define our target
add_library(c STATIC "")

# Define include directories
target_include_directories(c PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(c PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/aarch64")

include(${TS_ROOT}/tools/cmake/compiler/GCC.cmake)
compiler_set_freestanding(TARGET c)

# Add source files to be compiled.
target_sources(c PRIVATE
        "src/abort.c"
        "src/assert.c"
        "src/ctype.c"
        "src/exit.c"
        "src/memchr.c"
        "src/memcmp.c"
        "src/memcpy.c"
        "src/memmove.c"
        "src/memset.c"
        "src/printf.c"
        "src/putchar.c"
        "src/puts.c"
        "src/snprintf.c"
        "src/strcpy.c"
        "src/strlcat.c"
        "src/strchr.c"
        "src/strcmp.c"
        "src/strlcpy.c"
        "src/strlen.c"
        "src/strncmp.c"
        "src/strnlen.c"
        "src/strrchr.c"
        "src/strstr.c"
        "src/strtok.c"
        "src/strtol.c"
        "src/strtok.c"
        "src/strtoll.c"
        "src/strtoul.c"
        "src/strtoull.c"
)

target_compile_definitions(c INTERFACE ENABLE_CDEFSH_FIX)

add_library(stdlib::c ALIAS c)