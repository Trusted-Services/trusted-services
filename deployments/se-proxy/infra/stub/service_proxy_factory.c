/*
 * Copyright (c) 2021-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include "rpc/common/endpoint/rpc_service_interface.h"
#include "service/attestation/provider/attest_provider.h"
#include "service/attestation/provider/serializer/packed-c/packedc_attest_provider_serializer.h"
#include "service/crypto/factory/crypto_provider_factory.h"
#include "service/fwu/psa_fwu_m/agent/psa_fwu_m_update_agent.h"
#include "service/fwu/provider/fwu_provider.h"
#include "service/secure_storage/frontend/secure_storage_provider/secure_storage_provider.h"
#include "service/secure_storage/frontend/secure_storage_provider/secure_storage_uuid.h"

/* Stub backends */
#include <service/crypto/backend/stub/stub_crypto_backend.h>
#include <service/secure_storage/backend/mock_store/mock_store.h>

struct rpc_service_interface *attest_proxy_create(void)
{
	struct rpc_service_interface *attest_iface = NULL;

	/* Static objects for proxy instance */
	static struct attest_provider attest_provider = { 0 };

	/* Initialize the service provider */
	attest_iface = attest_provider_init(&attest_provider);

	attest_provider_register_serializer(&attest_provider,
					    packedc_attest_provider_serializer_instance());

	return attest_iface;
}

struct rpc_service_interface *crypto_proxy_create(void)
{
	struct rpc_service_interface *crypto_iface = NULL;
	struct crypto_provider *crypto_provider = NULL;

	if (stub_crypto_backend_init() == PSA_SUCCESS) {

		crypto_provider = crypto_provider_factory_create();
		crypto_iface = service_provider_get_rpc_interface(&crypto_provider->base_provider);
	}

	return crypto_iface;
}

struct rpc_service_interface *ps_proxy_create(void)
{
	static struct mock_store ps_backend;
	static struct secure_storage_provider ps_provider;
	const struct rpc_uuid service_uuid = { .uuid = TS_PSA_PROTECTED_STORAGE_UUID };

	struct storage_backend *backend = mock_store_init(&ps_backend);

	return secure_storage_provider_init(&ps_provider, backend, &service_uuid);
}

struct rpc_service_interface *its_proxy_create(void)
{
	static struct mock_store its_backend;
	static struct secure_storage_provider its_provider;
	const struct rpc_uuid service_uuid = { .uuid = TS_PSA_INTERNAL_TRUSTED_STORAGE_UUID };

	struct storage_backend *backend = mock_store_init(&its_backend);

	return secure_storage_provider_init(&its_provider, backend, &service_uuid);
}

struct rpc_service_interface *fwu_proxy_create(void)
{
	static struct update_agent *agent;
	static struct fwu_provider fwu_provider = { 0 };

	agent = psa_fwu_m_update_agent_init(NULL, 0, 4096);

	return fwu_provider_init(&fwu_provider, agent);
}
