#-------------------------------------------------------------------------------
# Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

set(FFA_DIRECT_MSG_ROUTING_EXTENSION OFF CACHE BOOL "Enable FF-A direct message routing extension")

add_components(TARGET "ftpm"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/common/fdt"
		"components/common/trace"
		"components/common/utils"
		"components/common/uuid"
		"components/config/ramstore"
		"components/config/loader/sp"
		"components/messaging/ffa/libsp"
		"components/rpc/common/endpoint"
		"components/rpc/tpm_crb_ffa/common"
		"components/rpc/tpm_crb_ffa/endpoint/sp"
		"components/service/log/factory"
		"components/service/log/client"
)

target_sources(ftpm PRIVATE
	${CMAKE_CURRENT_LIST_DIR}/ftpm_sp.c
)
