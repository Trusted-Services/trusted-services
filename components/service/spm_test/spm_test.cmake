#-------------------------------------------------------------------------------
# Copyright (c) 2022-2023, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
#  The CMakeLists.txt for building the spm-test sp deployment for opteesp
#
#  Used for building the SPs used in the  spm test. The SP can be build twice
#  , to be able to test inter SPs communication. This is done by passing the
#  -DSP_NUMBER=1 parameter.
#-------------------------------------------------------------------------------
target_include_directories(spm-test${SP_NUMBER} PRIVATE "${TOP_LEVEL_INCLUDE_DIRS}")

#-------------------------------------------------------------------------------
#  Extend with components that are common across all deployments of
#  spm-test
#
#-------------------------------------------------------------------------------
target_include_directories(spm-test${SP_NUMBER} PRIVATE
	${TS_ROOT}
	${TS_ROOT}/components
)

#-------------------------------------------------------------------------------
#  Set target platform to provide drivers needed by the deployment
#
#-------------------------------------------------------------------------------
add_platform(TARGET spm-test${SP_NUMBER})

#################################################################

target_compile_definitions(spm-test${SP_NUMBER} PRIVATE
	ARM64=1
)

target_include_directories(spm-test${SP_NUMBER} PRIVATE
	${TS_ROOT}/components/service/spm_test
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
	target_compile_options(spm-test${SP_NUMBER} PRIVATE
		-std=c99
	)
endif()

#-------------------------------------------------------------------------------
#  Deployment specific source files
#-------------------------------------------------------------------------------
target_sources(spm-test${SP_NUMBER} PRIVATE
	${TS_ROOT}/components/service/spm_test/sp.c
)

######################################## install
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "location to install build output to." FORCE)
endif()

install(TARGETS spm-test${SP_NUMBER}
			RUNTIME DESTINATION ${TS_ENV}/bin
		)


# Convert the base address used for memory region testing for the manifest file. The manifest template will use
# MEM_REG_LO and MEM_REG_HI.
# This value is either defined in platform.cmake or on the command line.
if (NOT DEFINED CFG_TEST_MEM_REGION_ADDRESS)
	message(FATAL_ERROR "Mandatory variable CFG_TEST_MEM_REGION_ADDRESS is not defined.")
endif()
uint64_split(VALUE ${CFG_TEST_MEM_REGION_ADDRESS} OUT_PREFIX MEM_REG)

include(${TS_ROOT}/tools/cmake/common/ExportSp.cmake)
export_sp(
	SP_FFA_UUID_CANON ${SP_FFA_UUID_CANON}
	SP_BIN_UUID_CANON ${SP_BIN_UUID_CANON}
	SP_BOOT_ORDER ${SP_BOOT_ORDER}
	SP_NAME "spm-test${SP_NUMBER}"
	MK_IN ${TS_ROOT}/environments/opteesp/sp.mk.in
	DTS_IN ${TS_ROOT}/deployments/spm-test${SP_NUMBER}/opteesp/default_spm_test${SP_NUMBER}.dts.in
	JSON_IN ${TS_ROOT}/environments/opteesp/sp_pkg.json.in
)
