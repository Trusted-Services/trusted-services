#-------------------------------------------------------------------------------
# Copyright (c) 2020, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

#[===[.rst:
Misc utilities
--------------
#]===]

include_guard(DIRECTORY)

#[===[.rst:
.. cmake:command:: check_args

  .. code-block:: cmake

    check_args(func_name REQ_ARG1 REQ_ARG2)

  Helper macro for argument checking in functions. First argument *func_name* is
  the name of the function, other arguments are the names of the required
  arguments to that function. The macro iterates through the list, and prints
  and error message if not all arguments are defined.

#]===]
macro(check_args)
	set(_argv "${ARGV}")
	list(SUBLIST _argv 0 1 _func)
	list(SUBLIST _argv 1 -1 _args)
	foreach(_arg IN LISTS _args)
		if (NOT DEFINED _MY_PARAMS_${_arg})
			message(FATAL_ERROR "${_func}(): mandatory parameter '${_arg}' missing.")
		endif()
	endforeach()
endmacro()

# Verify MSYS environment.
function(ts_verify_build_env)
    if (WIN32)
        #On MSYS2 64 bit builds do not work. Verify environment.
        execute_process(COMMAND uname -s
                        OUTPUT_VARIABLE _os_name)
        #If uname is present we assume MSYS environment and the os name must
        #contain MING32.
        if(_os_name STREQUAL "" AND NOT _os_name MATCHES ".*MINGW32.*")
            message(FATAL_ERROR "This seems to be a 64 bit MINGW shell, which has issues. Please run the 32bit version.")
        endif()
    endif()
endFunction()

#[===[.rst:
.. cmake:command:: ts_add_uuid_to_exe_name

  .. code-block:: cmake

    ts_add_uuid_to_exe_name(TGT <target name> UUID "canonical string")

	A function to modify the file name of the binary produced by a deployment to allow the OP-TEE symbolize.py tool to
	find it when analyzing stack dumps. This is only useful for SP deployments targeting OP-TEE.
	The filename will follow the template <file name>_<UUID>.elf format, where
	   - file name is the original name already configured for the target
	   - UUID is an argument of this function

	INPUTS:

	``TGT``
	Mandatory. The name of the target to manipulate.

	``UUID``
	Mandatory. The UUID to be used to identify the SP. This has to match the UUID used by OP-TEE OS to identify the SP
	runtime.

#]===]
function(ts_add_uuid_to_exe_name)
	set(options)
	set(oneValueArgs TGT UUID)
	set(multiValueArgs)
	cmake_parse_arguments(_MY_PARAMS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	check_args(ts_add_uuid_to_exe_name TGT)

	get_target_property(_tgt_type ${_MY_PARAMS_TGT} TYPE)
	if ("${_tgt_type}" STREQUAL "EXECUTABLE")
		check_args(ts_add_uuid_to_exe_name UUID)
		get_target_property(_out_name ${_MY_PARAMS_TGT} OUTPUT_NAME)
		if (NOT _out_name)
			set(_out_name "${_MY_PARAMS_TGT}")
		endif()
		get_target_property(_suffix ${_MY_PARAMS_TGT} SUFFIX)
		if (NOT _suffix)
			# Note CMAKE_EXECUTABLE_SUFFIX_<lang> might be needed here. Unfortunately
			# this is only set, when it is set manually. It overrides the EXE_SUFFIX
			# when set.
			set(_suffix ${CMAKE_EXECUTABLE_SUFFIX})
		endif()
		# If executable suffix is still not set at this point, use the full name as basename.
		if (_suffix)
			string(REGEX REPLACE "${_suffix}$" "" _base_name "${_out_name}")
		else()
			set(_base_name "${_out_name}")
		endif()

		set(_out_name "${_base_name}_${_MY_PARAMS_UUID}${_suffix}")
		set_target_properties(${_MY_PARAMS_TGT} PROPERTIES OUTPUT_NAME "${_out_name}")
	endif()
endfunction()

#[===[.rst:
.. cmake:command:: uint64_split

	.. code-block:: cmake

		uint64_split(VALUE 4294967296 OUT_PREFIX RES)
		message("RES_LO=${RES_LO} RES_HI=${RES_HI}")

		uint64_split(VALUE 0x1122334455667788 OUT_PREFIX RES DECIMAL)
		message("RES_LO=${RES_LO} RES_HI=${RES_HI}")

	Split an uint64 integer to uint32 integers. The returned values will be hexadecimal unless the 	``DECIMAL``
	argument is passed.
	The result is returned in two values <OUT_PREFIX>_LO and <OUT_PREFIX>_HI.

	INPUTS:

	``VALUE``
	Mandatory. uint64 value to be converted. The value shall either be an integer (e.g. 123) or a string representing
	an integer (e.g. "123"). Hexadecimal numbers can be specified with "0x" prefix.

	``DECIMAL``
	Optional. Set the format of the returned values to be decimal instead of hexadecimal.

	OUTPUTS:

	``OUT_PREFIX``
	Mandatory. The prefix of the output variables. Two variable will be created in the callers scope. <OUT_PREFIX>_LO
	is the lower 32 bits and <OUT_PREFIX>_HI is the higher 32 bits.

#]===]
function(uint64_split )
	set(options DECIMAL)
	set(oneValueArgs VALUE OUT_PREFIX)
	set(multiValueArgs)
	cmake_parse_arguments(_MY_PARAMS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	check_args(uint64_split VALUE OUT_PREFIX)

	# Ensure the input is a valid uint64 integer
	if (NOT "${_MY_PARAMS_VALUE}" MATCHES "^(0x[0-9a-fA-F]+)|([0-9]+)$")
		message(FATAL_ERROR "${CMAKE_CURRENT_FUNCTION}: Invalid uint64 integer: ${_MY_PARAMS_VALUE}")
	endif()

	if (_MY_PARAMS_DECIMAL)
		set(_out_format "DECIMAL")
	else()
		set(_out_format "HEXADECIMAL")
	endif()

	# Split the uint64 integer into two uint32 integers
	math(EXPR _high_uint32 "(${_MY_PARAMS_VALUE} >> 32) & 0xFFFFFFFF" OUTPUT_FORMAT ${_out_format})
	math(EXPR _low_uint32 "${_MY_PARAMS_VALUE} & 0xFFFFFFFF" OUTPUT_FORMAT ${_out_format})

	# Return the results
	set(${_MY_PARAMS_OUT_PREFIX}_LO ${_low_uint32} PARENT_SCOPE)
	set(${_MY_PARAMS_OUT_PREFIX}_HI ${_high_uint32} PARENT_SCOPE)
endfunction()
