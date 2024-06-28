#-------------------------------------------------------------------------------
# Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Common components used for any deployment of the fwu service provider.
#-------------------------------------------------------------------------------

if (NOT DEFINED TGT)
	message(FATAL_ERROR "Mandatory parameter TGT is not defined.")
endif()

#-------------------------------------------------------------------------------
#  Components common to all deployments
#
#-------------------------------------------------------------------------------
add_components(TARGET ${TGT}
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/app/fwu-tool"
		"components/common/uuid"
		"components/common/endian"
		"components/media/disk/gpt_iterator"
		"components/media/volume/index"
		"components/service/common/include"
		"components/service/fwu/agent"
		"components/service/fwu/common"
		"components/service/fwu/config"
		"components/service/fwu/config/gpt"
		"components/service/fwu/fw_store/banked"
		"components/service/fwu/fw_store/banked/metadata_serializer/v1"
		"components/service/fwu/fw_store/banked/metadata_serializer/v2"
		"components/service/fwu/installer"
		"components/service/fwu/installer/raw"
		"components/service/fwu/installer/copy"
		"components/service/fwu/installer/factory/default"
		"components/service/fwu/inspector/direct"
)

#################################################################

target_include_directories(${TGT} PRIVATE
	${TS_ROOT}
	${TS_ROOT}/components
)

#-------------------------------------------------------------------------------
#  Define install content.
#
#-------------------------------------------------------------------------------
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "location to install build output to." FORCE)
endif()
install(TARGETS ${TGT}
		RUNTIME DESTINATION ${TS_ENV}/bin
		PUBLIC_HEADER DESTINATION ${TS_ENV}/include)
