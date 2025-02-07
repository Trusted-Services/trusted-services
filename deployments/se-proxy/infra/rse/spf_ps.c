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

#include "service/secure_storage/frontend/secure_storage_provider/secure_storage_uuid.h"
#include "service/secure_storage/backend/secure_storage_ipc/secure_storage_ipc.h"
#include "service/secure_storage/frontend/secure_storage_provider/secure_storage_provider.h"
#include "service/secure_storage/backend/storage_backend.h"
#include "service/common/include/psa/sid.h"

struct rpc_service_interface *ps_proxy_create(void)
{
	static struct secure_storage_provider ps_provider;
	static struct secure_storage_ipc ps_backend;
	struct storage_backend *backend;
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	const struct rpc_uuid ps_uuid = { .uuid = TS_PSA_PROTECTED_STORAGE_UUID };

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

	backend = secure_storage_ipc_init(&ps_backend, &rpc_session);
	ps_backend.service_handle = TFM_PROTECTED_STORAGE_SERVICE_HANDLE;

	return secure_storage_provider_init(&ps_provider, backend, &ps_uuid);
}

ADD_PROXY_SERVICE_FACTORY(ps_proxy_create, PSAPS_PROXY, SE_PROXY_INTERFACE_PRIO_PS);
