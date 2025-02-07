#-------------------------------------------------------------------------------
# Copyright (c) 2023-2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# A stub infrastructure for the se-proxy. Infrastructure dependencies are all
# realized with stub components that do absolutely nothing.
#-------------------------------------------------------------------------------

# Configure supported set of proxy services.
set(CFG_ENABLE_ITS           On)
set(CFG_ENABLE_PS            On)
set(CFG_ENABLE_CRYPTO        On)
set(CFG_ENABLE_CRYPTO_NANO   On)
set(CFG_ENABLE_IAT           On)
set(CFG_ENABLE_FWU           On)
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
)

include(../../infra/rse/service_proxy_factory.cmake REQUIRED)
