#-------------------------------------------------------------------------------
# Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

add_components(TARGET "smm-gateway"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/common/uuid"
		"components/rpc/common/interface"
		"components/service/common/include"
		"components/service/common/provider"
		"components/service/smm_variable/backend"
		"components/service/smm_variable/provider"
		"components/service/secure_storage/include"
		"components/service/secure_storage/backend/mock_store"
		"protocols/rpc/common/packed-c"
)

target_include_directories(smm-gateway PRIVATE
	${TS_ROOT}
	${TS_ROOT}/components
)
