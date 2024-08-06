#-------------------------------------------------------------------------------
# Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#  Fetch or use externally provided source tree
#
#-------------------------------------------------------------------------------
if (DEFINED ENV{TS_TFA_PATH})
	# Use externally provided source tree
	set(TFA_SOURCE_DIR $ENV{TS_TFA_PATH} CACHE PATH "tf-a location" FORCE)
else()
	# Otherwise clone the tf-a repo
	set(TFA_URL "https://git.trustedfirmware.org/TF-A/trusted-firmware-a.git" CACHE STRING "tf-a repository URL")
	set(TFA_REFSPEC "v2.7.0" CACHE STRING "tf-a git refspec")
	set(TFA_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps/tf-a-src" CACHE PATH "Location of tf-a source")

	# Checking git
	find_program(GIT_COMMAND "git")
	if (NOT GIT_COMMAND)
		message(FATAL_ERROR "Please install git")
	endif()

	set(GIT_OPTIONS
		GIT_REPOSITORY ${TFA_URL}
		GIT_TAG ${TFA_REFSPEC}
		GIT_SHALLOW FALSE
	)

	include(${TS_ROOT}/tools/cmake/common/LazyFetch.cmake REQUIRED)
	LazyFetch_MakeAvailable(
		DEP_NAME tf-a
		FETCH_OPTIONS "${GIT_OPTIONS}"
		SOURCE_DIR ${TFA_SOURCE_DIR}
	)
endif()

#-------------------------------------------------------------------------------
#  Add tf-a as an external dependency
#
#-------------------------------------------------------------------------------
function(add_tfa_dependency)
	set(options  )
	set(oneValueArgs TARGET)
	cmake_parse_arguments(MY_PARAMS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

	if(NOT DEFINED MY_PARAMS_TARGET)
		message(FATAL_ERROR "add_tf-a: mandatory parameter TARGET not defined!")
	endif()

	# Provide override include files to enable tf-a components to be compiled outside of
	# the tf-a build environment.
	target_include_directories(${MY_PARAMS_TARGET} PRIVATE "${TS_ROOT}/external/tf_a/include")

	# Export tf-a public include files
	target_include_directories(${MY_PARAMS_TARGET} PRIVATE "${TFA_SOURCE_DIR}/include")
	target_include_directories(${MY_PARAMS_TARGET} PRIVATE "${TFA_SOURCE_DIR}/include/arch/aarch64")

	# Disable assertions in TF-A for release builds
	if ("${UC_CMAKE_BUILD_TYPE}" STREQUAL "MINSIZEREL" OR
		"${UC_CMAKE_BUILD_TYPE}" STREQUAL "RELEASE" OR
		"${UC_CMAKE_BUILD_TYPE}" STREQUAL "RELWITHDEBINFO")
    	target_compile_definitions(${MY_PARAMS_TARGET} PRIVATE "ENABLE_ASSERTIONS=0")
	endif()
endfunction()