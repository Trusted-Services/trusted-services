#-------------------------------------------------------------------------------
# Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# A stub infrastructure for the se-proxy. Infrastructure dependencies are all
# realized with stub components that do absolutely nothing.
#-------------------------------------------------------------------------------

# Configure supported set of proxy services.
set(CFG_ENABLE_ITS           ON)
set(CFG_ENABLE_PS            ON)
set(CFG_ENABLE_CRYPTO        ON)
set(CFG_ENABLE_CRYPTO_NANO   ON)
set(CFG_ENABLE_IAT           OFF)
set(CFG_ENABLE_FWU           OFF)
#-------------------------------------------------------------------------------
# Infrastructure components
#
#-------------------------------------------------------------------------------
add_components(TARGET "se-proxy"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/common/uuid"
		"components/messaging/rse_comms/sp"
		"components/rpc/common/caller"
		"components/rpc/rse_comms"
)

include(${TS_ROOT}/deployments/se-proxy/infra/rse/service_proxy_factory.cmake REQUIRED)
