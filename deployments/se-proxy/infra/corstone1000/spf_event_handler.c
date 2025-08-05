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

#include "corstone1000_event_handling.h"

struct rpc_service_interface *event_handler_proxy_create(void)
{
	static struct event_provider event_provider = {0};
	const struct rpc_uuid dummy_uuid = { .uuid = { 0 }};
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;

	/* Static objects for proxy instance */
	static struct rpc_caller_interface rse_comms = { 0 };
	static struct rpc_caller_session rpc_session = { 0 };

	rpc_status = rse_comms_caller_init(&rse_comms);
	if (rpc_status != RPC_SUCCESS)
		return NULL;

	rpc_status = rpc_caller_session_open(&rpc_session, &rse_comms, &dummy_uuid, 0, 0);
	if (rpc_status != RPC_SUCCESS)
		return NULL;

	event_provider.client.session = &rpc_session;
	event_provider.client.rpc_status = RPC_SUCCESS;
	event_provider.client.service_info.supported_encodings = 0;
	event_provider.client.service_info.max_payload = 4096;

	return corstone1000_event_provider_init(&event_provider);
}

ADD_PROXY_SERVICE_FACTORY(event_handler_proxy_create,
			  EVENT_HANDLER_PROXY, SE_PROXY_INTERFACE_PRIO_FWU + 1);
