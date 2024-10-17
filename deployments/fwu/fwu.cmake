#-------------------------------------------------------------------------------
# Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Common components used for any deployment of the fwu service provider.
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#  Components common to all deployments
#
#-------------------------------------------------------------------------------
add_components(TARGET "fwu"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
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
		"components/service/log/factory"
		"components/service/log/client"
		"components/rpc/common/caller"
		"components/rpc/ts_rpc/caller/sp"
		"components/service/common/client"
)

#################################################################

target_include_directories(fwu PRIVATE
	${TS_ROOT}
	${TS_ROOT}/components
)
