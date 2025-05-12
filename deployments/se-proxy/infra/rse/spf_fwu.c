/*
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include "deployments/se-proxy/env/commonsp/proxy_service_factory_list.h"
#include "rpc/common/caller/rpc_caller.h"
#include "rpc/common/caller/rpc_caller_session.h"
#include "rpc/common/interface/rpc_status.h"
#include "rpc/common/interface/rpc_uuid.h"
#include "rpc/rse_comms/caller/sp/rse_comms_caller.h"

#include "service/fwu/provider/fwu_provider.h"
#include "service/fwu/psa_fwu_m/agent/psa_fwu_m_update_agent.h"
#include "service/fwu/provider/fwu_provider.h"
#include "service/fwu/common/update_agent_interface.h"
#include "service/fwu/psa_fwu_m/interface/psa_ipc/psa_fwu_ipc.h"

struct rpc_service_interface *fwu_proxy_create(void)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	static struct update_agent *agent;
	static struct fwu_provider fwu_provider = { 0 };

	/* Static objects for proxy instance */
	static struct rpc_caller_interface rse_comms = { 0 };
	static struct rpc_caller_session rpc_session = { 0 };

	const struct rpc_uuid dummy_uuid = { 0 };

	rpc_status = rse_comms_caller_init(&rse_comms);
	if (rpc_status != RPC_SUCCESS)
		return NULL;

	rpc_status = rpc_caller_session_open(&rpc_session, &rse_comms, &dummy_uuid, 0, 0);
	if (rpc_status != RPC_SUCCESS)
		return NULL;

	agent = psa_fwu_m_update_agent_init(NULL, 0, 4096);
	if (psa_fwu_ipc_init(&rpc_session) != PSA_SUCCESS)
		return NULL;

	return fwu_provider_init(&fwu_provider, agent);
}

ADD_PROXY_SERVICE_FACTORY(fwu_proxy_create, FWU_PROXY, SE_PROXY_INTERFACE_PRIO_FWU);
