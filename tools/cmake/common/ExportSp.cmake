#-------------------------------------------------------------------------------
# Copyright (c) 2020-2023, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

include(${CMAKE_CURRENT_LIST_DIR}/Uuid.cmake)

#[===[.rst:
.. cmake:command:: export_sp

	.. code:: cmake

		export_sp(
			SP_FFA_UUID_CANON <uuid_str_canon>
			SP_NAME <name> MK_IN <.mk path>
			SP_BOOT_ORDER <number>
			DTS_IN <DTS path>
			DTS_MEM_REGIONS <Memory region manifest path>
			JSON_IN <JSON path>
		)

	INPUTS:

	``SP_FFA_UUID_CANON``
	The FF-A UUID of the SP as a canonical string.

	``SP_BIN_UUID_CANON``
	The UUID of the SP binary a canonical string. When not set use the
	SP_FFA_UUID_CANON as the SP_BIN_UUID_CANON.

	``SP_BOOT_ORDER``
	Boot-order of the SP. 0 will be booted first.

	``SP_NAME``
	The name of the SP.

	``MK_IN``
	Optional, Makefile template for OP-TEE build

	``DTS_IN``
	Manifest file template

	`DTS_MEM_REGIONS`
	Optional, Memory region manifest file

	``JSON_IN``
	Optional, SP layout JSON file template for TF-A

#]===]
function (export_sp)
	set(options)
	set(oneValueArgs SP_FFA_UUID_CANON SP_BIN_UUID_CANON SP_BOOT_ORDER SP_NAME MK_IN DTS_IN DTS_MEM_REGIONS JSON_IN)
	set(multiValueArgs)
	cmake_parse_arguments(EXPORT "${options}" "${oneValueArgs}"
						"${multiValueArgs}" ${ARGN} )

	if(NOT DEFINED EXPORT_SP_FFA_UUID_CANON)
		message(FATAL_ERROR "export_sp: mandatory parameter SP_FFA_UUID_CANON not defined!")
	endif()
	if(NOT DEFINED EXPORT_SP_BIN_UUID_CANON)
		# We use the same UUID for the binary and FF-A if the UUID of the SP binary is not set
		set(EXPORT_SP_BIN_UUID_CANON ${EXPORT_SP_FFA_UUID_CANON})
	endif()
	if(NOT DEFINED EXPORT_SP_BOOT_ORDER)
		message(FATAL_ERROR "export_sp: mandatory parameter SP_BOOT_ORDER not defined!")
	endif()
	if(NOT DEFINED EXPORT_SP_NAME)
		message(FATAL_ERROR "export_sp: mandatory parameter SP_NAME not defined!")
	endif()
	if(NOT DEFINED EXPORT_DTS_IN)
		message(FATAL_ERROR "export_sp: mandatory parameter DTS_IN not defined!")
	endif()

	if (DEFINED EXPORT_MK_IN)
		configure_file(${EXPORT_MK_IN} ${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_SP_NAME}.mk @ONLY NEWLINE_STYLE UNIX)
		install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_SP_NAME}.mk DESTINATION ${TS_ENV}/lib/make)
	endif()

	# In the SP manifest DT the UUID format is four uint32 numbers (little-endian)
	# Create a litte endian 4 digit octests representation.
	uuid_canon_to_le_words(UUID ${EXPORT_SP_FFA_UUID_CANON} RES _le_words)
	list(JOIN _le_words " 0x" _uuid_le)
	set(SP_UUID_LE " 0x${_uuid_le}" PARENT_SCOPE)
	set(EXPORT_SP_UUID_DT " 0x${_uuid_le}")

	# The .dts file is a standalone structure, thus it should have the /dts-v1/ tag and it
	# starts with the root node.
	set(DTS_TAG "/dts-v1/;")
	set(DTS_NODE "/")
	configure_file(${EXPORT_DTS_IN} ${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_SP_BIN_UUID_CANON}_before_preprocessing.dts @ONLY NEWLINE_STYLE UNIX)

	compiler_preprocess_file(
		SRC ${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_SP_BIN_UUID_CANON}_before_preprocessing.dts
		DST ${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_SP_BIN_UUID_CANON}.dts
		TARGET ${EXPORT_SP_NAME}
	)

	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_SP_BIN_UUID_CANON}.dts DESTINATION ${TS_ENV}/manifest)

	if (DEFINED EXPORT_DTS_MEM_REGIONS)
		install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_DTS_MEM_REGIONS} DESTINATION ${TS_ENV}/manifest)
	endif()

	if (DEFINED EXPORT_JSON_IN)
		configure_file(${EXPORT_JSON_IN} ${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_SP_NAME}.json @ONLY NEWLINE_STYLE UNIX)
		install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_SP_NAME}.json DESTINATION ${TS_ENV}/json)
	endif()
endfunction()
