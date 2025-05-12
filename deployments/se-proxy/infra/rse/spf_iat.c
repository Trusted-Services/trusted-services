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

#include "service/attestation/client/psa/iat_client.h"
#include "service/attestation/provider/attest_provider.h"
#include "service/attestation/provider/serializer/packed-c/packedc_attest_provider_serializer.h"

struct rpc_service_interface *attest_proxy_create(void)
{
	static struct attest_provider attest_provider = { 0 };
	struct rpc_service_interface *attest_iface = NULL;
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;

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

	/* Initialize the service provider */
	attest_iface = attest_provider_init(&attest_provider);
	psa_iat_client_init(&rpc_session);

	attest_provider_register_serializer(&attest_provider,
					    packedc_attest_provider_serializer_instance());

	return attest_iface;
}

ADD_PROXY_SERVICE_FACTORY(attest_proxy_create, PSAIAT_PROXY, SE_PROXY_INTERFACE_PRIO_ATTEST);
