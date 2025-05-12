#-------------------------------------------------------------------------------
# Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

add_components(TARGET "ftpm"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/rpc/common/caller"
		"components/rpc/ts_rpc/common"
		"components/rpc/ts_rpc/caller/sp"
		"components/service/common/client"
		"components/service/crypto/backend/mbedcrypto/trng_adapter/platform"
		"components/service/locator"
		"components/service/locator/interface"
		"components/service/locator/sp"
		"components/service/locator/sp/ffa"
		"components/service/secure_storage/include"
		"components/service/secure_storage/frontend/psa/ps"
		"components/service/secure_storage/backend/null_store"
		"components/service/secure_storage/backend/secure_storage_client"
		"components/service/secure_storage/factory/sp/rot_store"
		"components/service/tpm/backend/ms_tpm"
)

#-------------------------------------------------------------------------------
#  This infrastructure depends on platform specific drivers
#
#-------------------------------------------------------------------------------
add_platform(TARGET "ftpm")

include(${TS_ROOT}/external/ms_tpm/ms_tpm.cmake)
target_link_libraries(ftpm PRIVATE ms_tpm::tpm)
