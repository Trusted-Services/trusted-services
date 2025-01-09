/*
 * Copyright (c) 2021-2024, Arm Limited and Contributors. All rights reserved.
 * Copyright (c) 2021-2023, Linaro Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <psa/sid.h>
#include "rpc/common/endpoint/rpc_service_interface.h"
#include <rpc/rse_comms/caller/sp/rse_comms_caller.h>
#include <service/attestation/provider/attest_provider.h>
#include <service/attestation/provider/serializer/packed-c/packedc_attest_provider_serializer.h>
#include <service/crypto/factory/crypto_provider_factory.h>
#include "service/fwu/psa_fwu_m/agent/psa_fwu_m_update_agent.h"
#include "service/fwu/provider/fwu_provider.h"
#include "service/fwu/psa_fwu_m/interface/psa_ipc/psa_fwu_ipc.h"
#include <service/secure_storage/frontend/secure_storage_provider/secure_storage_provider.h>
#include "service/secure_storage/frontend/secure_storage_provider/secure_storage_uuid.h"
#include <trace.h>

/* backends */
#include <service/crypto/backend/psa_ipc/crypto_ipc_backend.h>
#include <service/secure_storage/backend/secure_storage_ipc/secure_storage_ipc.h>
#include <service/attestation/client/psa/iat_client.h>

static const struct rpc_uuid dummy_uuid = { 0 };

struct rpc_service_interface *attest_proxy_create(void)
{
	struct rpc_service_interface *attest_iface = NULL;
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;

	/* Static objects for proxy instance */
	static struct rpc_caller_interface rse_comms = { 0 };
	static struct rpc_caller_session rpc_session = { 0 };
	static struct attest_provider attest_provider = { 0 };

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

struct rpc_service_interface *crypto_proxy_create(void)
{
	struct rpc_service_interface *crypto_iface = NULL;
	struct crypto_provider *crypto_provider;
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

	if (crypto_ipc_backend_init(&rpc_session) != PSA_SUCCESS)
		return NULL;

	crypto_provider = crypto_provider_factory_create();
	crypto_iface = service_provider_get_rpc_interface(&crypto_provider->base_provider);

	return crypto_iface;
}

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

struct rpc_service_interface *its_proxy_create(void)
{
	static struct secure_storage_provider its_provider;
	static struct secure_storage_ipc its_backend;
	struct storage_backend *backend;
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	const struct rpc_uuid its_uuid = { .uuid = TS_PSA_INTERNAL_TRUSTED_STORAGE_UUID };

	/* Static objects for proxy instance */
	static struct rpc_caller_interface rse_comms = { 0 };
	static struct rpc_caller_session rpc_session = { 0 };

	rpc_status = rse_comms_caller_init(&rse_comms);
	if (rpc_status != RPC_SUCCESS)
		return NULL;

	rpc_status = rpc_caller_session_open(&rpc_session, &rse_comms, &dummy_uuid, 0, 0);
	if (rpc_status != RPC_SUCCESS)
		return NULL;

	backend = secure_storage_ipc_init(&its_backend, &rpc_session);
	its_backend.service_handle = TFM_INTERNAL_TRUSTED_STORAGE_SERVICE_HANDLE;

	return secure_storage_provider_init(&its_provider, backend, &its_uuid);
}

struct rpc_service_interface *fwu_proxy_create(void)
{
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	static struct update_agent *agent;
	static struct fwu_provider fwu_provider = { 0 };

	/* Static objects for proxy instance */
	static struct rpc_caller_interface rse_comms = { 0 };
	static struct rpc_caller_session rpc_session = { 0 };

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
