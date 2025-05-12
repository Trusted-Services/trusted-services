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

#include "service_provider.h"

#include "service/crypto/backend/psa_ipc/crypto_ipc_backend.h"
#include "service/crypto/factory/crypto_provider_factory.h"
#include "service/crypto/provider/crypto_provider.h"

struct rpc_service_interface *crypto_proxy_create(void)
{
	struct crypto_provider *crypto_provider = NULL;
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

	if (crypto_ipc_backend_init(&rpc_session) != PSA_SUCCESS)
		return NULL;

	crypto_provider = crypto_provider_factory_create();
	return service_provider_get_rpc_interface(&crypto_provider->base_provider);
}

ADD_PROXY_SERVICE_FACTORY(crypto_proxy_create, PSACRYPTO_PROXY, SE_PROXY_INTERFACE_PRIO_CRYPTO);


#ifdef CFG_ENABLE_CRYPTO_NANO
struct rpc_service_interface *crypto_protobuf_proxy_create(void)
{
	struct crypto_provider *crypto_protobuf_provider = NULL;

	crypto_protobuf_provider = crypto_protobuf_provider_factory_create();
	if (!crypto_protobuf_provider) {
		return NULL;
	}

	return service_provider_get_rpc_interface(&crypto_protobuf_provider->base_provider);
}

ADD_PROXY_SERVICE_FACTORY(crypto_protobuf_proxy_create, PSACRYPTO_PROXY_NANO, SE_PROXY_INTERFACE_PRIO_CRYPTO_NANO);
#endif
