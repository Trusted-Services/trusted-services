#-------------------------------------------------------------------------------
# Copyright (c) 2020-2021, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

set(MBEDTLS_URL "https://github.com/ARMmbed/mbedtls.git" CACHE STRING "Mbed TLS repository URL")
set(MBEDTLS_REFSPEC "mbedtls-2.26.0" CACHE STRING "Mbed TLS git refspec")
set(MBEDTLS_INSTALL_PATH "${CMAKE_CURRENT_BINARY_DIR}/mbedtls_install" CACHE PATH "Mbed TLS installation directory")
set(MBEDTLS_PACKAGE_PATH "${MBEDTLS_INSTALL_PATH}/lib/mbedtls/cmake" CACHE PATH "Mbed TLS CMake package directory")

include(FetchContent)

# Checking git
find_program(GIT_COMMAND "git")
if (NOT GIT_COMMAND)
	message(FATAL_ERROR "Please install git")
endif()

# Fetching Mbed TLS
FetchContent_Declare(
	mbedtls
	GIT_REPOSITORY ${MBEDTLS_URL}
	GIT_TAG ${MBEDTLS_REFSPEC}
	GIT_SHALLOW TRUE
)

# FetchContent_GetProperties exports mbedtls_SOURCE_DIR and mbedtls_BINARY_DIR variables
FetchContent_GetProperties(mbedtls)
if(NOT mbedtls_POPULATED)
	message(STATUS "Fetching Mbed TLS")
	FetchContent_Populate(mbedtls)
endif()

# Convert the include path list to a string. Needed to make parameter passing to
# Mbed TLS build work fine.
string(REPLACE ";" "\\;" MBEDTLS_EXTRA_INCLUDES "${MBEDTLS_EXTRA_INCLUDES}")

find_package(Python3 COMPONENTS Interpreter)
if (NOT Python3_Interpreter_FOUND)
	message(FATAL_ERROR "Python 3 interpreter not found.")
endif()

#Configure Mbed TLS to build only mbedcrypto lib
execute_process(COMMAND ${Python3_EXECUTABLE} scripts/config.py crypto WORKING_DIRECTORY ${mbedtls_SOURCE_DIR})

#Configure the library
if(NOT CMAKE_CROSSCOMPILING)
	execute_process(COMMAND
		${CMAKE_COMMAND}
			-DENABLE_PROGRAMS=OFF
			-DENABLE_TESTING=OFF
			-DUNSAFE_BUILD=ON
			-DCMAKE_INSTALL_PREFIX=${MBEDTLS_INSTALL_PATH}
			-DCMAKE_TOOLCHAIN_FILE=${TS_EXTERNAL_LIB_TOOLCHAIN_FILE}
			-Dthirdparty_def=-DMBEDTLS_USER_CONFIG_FILE="${MBEDTLS_USER_CONFIG_FILE}"
			-Dthirdparty_inc=${MBEDTLS_EXTRA_INCLUDES}
			-GUnix\ Makefiles
			${mbedtls_SOURCE_DIR}
		WORKING_DIRECTORY
			${mbedtls_BINARY_DIR}
		)
else()
	execute_process(COMMAND
		${CMAKE_COMMAND}
			-DENABLE_PROGRAMS=OFF
			-DENABLE_TESTING=OFF
			-DUNSAFE_BUILD=ON
			-DCMAKE_INSTALL_PREFIX=${MBEDTLS_INSTALL_PATH}
			-DCMAKE_TOOLCHAIN_FILE=${TS_EXTERNAL_LIB_TOOLCHAIN_FILE}
			-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY
			-Dthirdparty_def=-DMBEDTLS_USER_CONFIG_FILE="${MBEDTLS_USER_CONFIG_FILE}"
			-Dthirdparty_inc=${MBEDTLS_EXTRA_INCLUDES}
			-GUnix\ Makefiles
			${mbedtls_SOURCE_DIR}
		WORKING_DIRECTORY
			${mbedtls_BINARY_DIR}
		RESULT_VARIABLE _exec_error
	)

	if (_exec_error)
		message(FATAL_ERROR "Configuration step of Mbed TLS failed with ${_exec_error}.")
	endif()
endif()

#TODO: add dependnecy to generated project on this file!
#TODO: add custom target to rebuild Mbed TLS

#Build the library
execute_process(COMMAND
		${CMAKE_COMMAND} --build ${mbedtls_BINARY_DIR} -- install -j8
		RESULT_VARIABLE _exec_error
	)
if (_exec_error)
	message(FATAL_ERROR "Build step of Mbed TLS failed with ${_exec_error}.")
endif()

#Create an imported target to have clean abstraction in the build-system.
add_library(mbedcrypto STATIC IMPORTED)
set_property(TARGET mbedcrypto PROPERTY IMPORTED_LOCATION "${MBEDTLS_INSTALL_PATH}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}mbedcrypto${CMAKE_STATIC_LIBRARY_SUFFIX}")
set_property(TARGET mbedcrypto PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${MBEDTLS_INSTALL_PATH}/include")