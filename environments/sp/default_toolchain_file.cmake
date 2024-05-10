#-------------------------------------------------------------------------------
# Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

# Since we append to default compilation flags stop multiple inclusion to avoid
# flags being added multiple times.
include_guard(GLOBAL)

#GNUARM v8 and v9 compilers use a different triplet.
if(NOT CROSS_COMPILE AND NOT DEFINED ENV{CROSS_COMPILE})
	set(CROSS_COMPILE "aarch64-elf-;aarch64-none-elf-;aarch64-linux-gnu-;aarch64-none-linux-gnu-" CACHE STRING "List of GCC prefix triplets to use.")
endif()

set(CMAKE_CROSSCOMPILING True)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_POSITION_INDEPENDENT_CODE True)

set(TS_DEBUG_INFO_FLAGS "-fdiagnostics-show-option -gdwarf-2" CACHE STRING "Compiler flags to add debug information.")
set(TS_MANDATORY_AARCH_FLAGS "-fpie -mstrict-align -march=armv8-a+crc" CACHE STRING "Compiler flags configuring architecture specific ")
set(TS_WARNING_FLAGS "-Wall" CACHE STRING "Compiler flags affecting generating warning messages.")
set(TS_MANDATORY_LINKER_FLAGS "-Wl,-pie -Wl,--no-dynamic-linker -Wl,--sort-section=alignment -zmax-page-size=4096" CACHE STRING "Linker flags needed for correct builds.")

set(BTI_ENABLED unset CACHE STRING "Enable Branch Target Identification (BTI)")
set_property(CACHE BTI_ENABLED PROPERTY STRINGS unset OFF ON)

if(BTI_ENABLED STREQUAL "ON")
	# branch-protection enables bti while compile force-bti tells the linker to
	# warn if some object files lack the .note.gnu.property section with the BTI
	# flag, and to turn on the BTI flag in the output anyway.
	set(TS_MANDATORY_AARCH_FLAGS "${TS_MANDATORY_AARCH_FLAGS} -mbranch-protection=bti")
	set(TS_MANDATORY_LINKER_FLAGS "${TS_MANDATORY_LINKER_FLAGS} -zforce-bti")
elseif(BTI_ENABLED STREQUAL "OFF")
	set(TS_MANDATORY_AARCH_FLAGS "${TS_MANDATORY_AARCH_FLAGS} -mbranch-protection=none")
endif()

# Set flags affecting all build types
string(APPEND CMAKE_C_FLAGS_INIT " ${TS_MANDATORY_AARCH_FLAGS}")
string(APPEND CMAKE_CXX_FLAGS_INIT " ${TS_MANDATORY_AARCH_FLAGS}")
string(APPEND CMAKE_EXE_LINKER_FLAGS_INIT " ${TS_MANDATORY_LINKER_FLAGS}")
if(DEFINED TS_ROOT)
	# Flags not to be used with external components.
	string(APPEND CMAKE_C_FLAGS_INIT " ${TS_WARNING_FLAGS}")
	string(APPEND CMAKE_CXX_FLAGS_INIT " ${TS_WARNING_FLAGS}")
endif()

# Set flags affecting all build types supporting debugging.
foreach(_b_type IN ITEMS DEBUG RELWITHDEBINFO MINSIZWITHDEBINFO)
	string(APPEND CMAKE_C_FLAGS_${_b_type}_INIT " ${TS_DEBUG_INFO_FLAGS}")
	string(APPEND CMAKE_CXX_FLAGS_${_b_type}_INIT " ${TS_DEBUG_INFO_FLAGS}")
endforeach()

# Build type specific flags
string(APPEND CMAKE_C_FLAGS_DEBUG_INIT " -O0")
string(APPEND CMAKE_C_FLAGS_MINSIZEREL_INIT  " -Os")
string(APPEND CMAKE_C_FLAGS_MINSIZWITHDEBINFO_INIT " -Os")
string(APPEND CMAKE_C_FLAGS_RELEASE_INIT " -O2")
string(APPEND CMAKE_C_FLAGS_RELWITHDEBINFO_INIT " -O2")
string(APPEND CMAKE_CXX_FLAGS_DEBUG_INIT " -O0")
string(APPEND CMAKE_CXX_FLAGS_MINSIZEREL_INIT  " -Os")
string(APPEND CMAKE_CXX_FLAGS_MINSIZWITHDEBINFO_INIT " -Os")
string(APPEND CMAKE_CXX_FLAGS_RELEASE_INIT " -O2")
string(APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT " -O2")

include($ENV{TS_ROOT}/tools/cmake/compiler/GCC.cmake REQUIRED)
include($ENV{TS_ROOT}/tools/cmake/compiler/config_iface.cmake REQUIRED)
