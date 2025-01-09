#-------------------------------------------------------------------------------
# Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# A stub infrastructure for the se-proxy. Infrastructure dependencies are all
# realized with stub components that do absolutely nothing.
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Infrastructure components
#
#-------------------------------------------------------------------------------
add_components(TARGET "se-proxy"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/common/uuid"
		"components/rpc/common/caller"
		"components/rpc/rse_comms"
		"components/messaging/rse_comms/sp"
		"components/service/attestation/client/psa_ipc"
		"components/service/attestation/key_mngr/local"
		"components/service/attestation/reporter/psa_ipc"
		"components/service/crypto/backend/psa_ipc"
		"components/service/fwu/common"
		"components/service/fwu/provider"
		"components/service/fwu/provider/serializer"
		"components/service/fwu/psa_fwu_m/agent"
		"components/service/fwu/psa_fwu_m/interface/psa_ipc"
		"components/service/secure_storage/backend/secure_storage_ipc"
)

target_sources(se-proxy PRIVATE

	${CMAKE_CURRENT_LIST_DIR}/service_proxy_factory.c
)
