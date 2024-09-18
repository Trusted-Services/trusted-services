#-------------------------------------------------------------------------------
# Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

#[===[.rst:
.. cmake:command:: export_library

	.. code:: cmake

		export_library(TARGET <target name> LIB_NAME <library name> PKG_CONFIG_FILE <file name>)

	INPUTS:

	``TARGET``
	The name of an already defined target that corresponds to the library.

	``LIB_NAME``
	The name of the library.

	``PKG_CONFIG_FILE``
	Name of the package configuration file to generate.

#]===]
function(export_library)
	set(options  )
	set(oneValueArgs TARGET LIB_NAME PKG_CONFIG_FILE)
	set(multiValueArgs)
	cmake_parse_arguments(MY_PARAMS "${options}" "${oneValueArgs}"
						"${multiValueArgs}" ${ARGN} )

	foreach(_param IN ITEMS MY_PARAMS_TARGET MY_PARAMS_LIB_NAME MY_PARAMS_PKG_CONFIG_FILE)
		if(NOT DEFINED ${_param})
			list(APPEND _miss_params "${_param}" )
		endif()
	endforeach()

	if (_miss_params)
		string(REPLACE ";" ", " _miss_params "${_miss_params}")
		message(FATAL_ERROR "export_library: mandatory parameter(s) ${_miss_params} not defined!")
	endif()


	string(TOLOWER "${MY_PARAMS_LIB_NAME}" LC_LIB_NAME)
	set(_version_file_name "${LC_LIB_NAME}ConfigVersion.cmake")

	# Set default install location if none specified
	if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
		set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "location to install build output to." FORCE)
	endif()

	# Specify export name and destinations for install
	install(
		TARGETS ${MY_PARAMS_TARGET}
		EXPORT ${MY_PARAMS_LIB_NAME}_targets
		ARCHIVE
			DESTINATION ${TS_ENV}/lib
		LIBRARY
			DESTINATION ${TS_ENV}/lib
		PUBLIC_HEADER
			DESTINATION ${TS_ENV}/include
	)

	# Create targets file.
	export(
		EXPORT
			${MY_PARAMS_LIB_NAME}_targets
		FILE
			"${CMAKE_CURRENT_BINARY_DIR}/${MY_PARAMS_LIB_NAME}Targets.cmake"
		NAMESPACE
			${MY_PARAMS_LIB_NAME}::
	)

	# Create a config file package.
	include(CMakePackageConfigHelpers)
	get_target_property(_ver ${MY_PARAMS_TARGET} VERSION)
	write_basic_package_version_file(
		"${CMAKE_CURRENT_BINARY_DIR}/${_version_file_name}"
		VERSION "${_ver}"
		COMPATIBILITY SameMajorVersion
	)

	# Finalize config file.
	# Config package location relative to install root.
	set(ConfigPackageLocation ${TS_ENV}/lib/cmake/${MY_PARAMS_LIB_NAME})

	get_filename_component(_configured_pkgcfg_name "${MY_PARAMS_PKG_CONFIG_FILE}" NAME_WLE)
	set(_configured_pkgcfg_name "${CMAKE_CURRENT_BINARY_DIR}/${_configured_pkgcfg_name}")

	configure_package_config_file(
			"${MY_PARAMS_PKG_CONFIG_FILE}"
			"${_configured_pkgcfg_name}"
		INSTALL_DESTINATION
			${ConfigPackageLocation}
	)

	# Install the export details
	install(
		EXPORT ${MY_PARAMS_LIB_NAME}_targets
		FILE ${MY_PARAMS_LIB_NAME}Targets.cmake
		NAMESPACE ${MY_PARAMS_LIB_NAME}::
		DESTINATION ${ConfigPackageLocation}
		COMPONENT ${MY_PARAMS_LIB_NAME}
	)


	# install config and version files
	install(
		FILES
			"${_configured_pkgcfg_name}"
			"${CMAKE_CURRENT_BINARY_DIR}/${_version_file_name}"
		DESTINATION
			${ConfigPackageLocation}
		COMPONENT
			${MY_PARAMS_LIB_NAME}
	)
endfunction()
