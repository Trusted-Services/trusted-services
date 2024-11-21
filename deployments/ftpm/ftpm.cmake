#-------------------------------------------------------------------------------
# Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

add_components(TARGET ftpm
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/rpc/common/interface"
		"components/rpc/common/endpoint"
		"components/service/common/include"
		"components/service/common/provider"
		"components/service/tpm/provider"
)
