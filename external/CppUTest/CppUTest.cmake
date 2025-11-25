#-------------------------------------------------------------------------------
# Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------


set(CPPUTEST_URL "https://github.com/cpputest/cpputest.git" CACHE STRING "CppUTest repository URL")
set(CPPUTEST_REFSPEC "v3.8" CACHE STRING "CppUTest git refspec")
set(CPPUTEST_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/CppUTest_install CACHE PATH "CppUTest installation directory")
set(CPPUTEST_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/_deps/cpputest-src CACHE PATH "CppUTest source directory")
set(CPPUTEST_BUILD_TYPE "Release" CACHE STRING "CppUTest build type")

set(GIT_OPTIONS
	GIT_REPOSITORY ${CPPUTEST_URL}
	GIT_TAG ${CPPUTEST_REFSPEC}
	GIT_SHALLOW FALSE
	PATCH_COMMAND
		git stash
		COMMAND git branch -f bf-am
		COMMAND git am ${CMAKE_CURRENT_LIST_DIR}/0001-Fix-cmake-problems.patch
		COMMAND git am ${CMAKE_CURRENT_LIST_DIR}/0002-Fix-cmake-4.0-compatibility.patch
		COMMAND git reset bf-am
	)

include(${TS_ROOT}/tools/cmake/common/LazyFetch.cmake REQUIRED)
LazyFetch_MakeAvailable(DEP_NAME CppUTest
	FETCH_OPTIONS "${GIT_OPTIONS}"
	INSTALL_DIR ${CPPUTEST_INSTALL_DIR}
	PACKAGE_DIR ${CPPUTEST_INSTALL_DIR}/lib/CppUTest/cmake
	CACHE_FILE "${TS_ROOT}/external/CppUTest/cpputest-init-cache.cmake.in"
	SOURCE_DIR ${CPPUTEST_SOURCE_DIR}
	)

# CppUTest package files do not set include path properties on the targets.
# Fix this here.
foreach(_cpputest_target IN LISTS CppUTest_LIBRARIES)
	if (TARGET	${_cpputest_target})
		target_include_directories(${_cpputest_target} INTERFACE ${CppUTest_INCLUDE_DIRS})
		target_compile_features(${_cpputest_target} INTERFACE cxx_std_11)
	endif()
endforeach()
