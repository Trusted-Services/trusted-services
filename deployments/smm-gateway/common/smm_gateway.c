/*
 * Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <protocols/rpc/common/packed-c/encoding.h>
#include <service/uefi/smm_variable/provider/smm_variable_provider.h>
#include <service/crypto/client/psa/psa_crypto_client.h>
#include "psa/crypto.h"
#include <service/secure_storage/backend/secure_storage_client/secure_storage_client.h>
#include <service/secure_storage/backend/mock_store/mock_store.h>
#include <service/locator/sp/ffa/spffa_service_context.h>
#include <service_locator.h>

/* Build-time default configuration */

/* Default to using the Protected Storage SP */
#ifndef SMM_GATEWAY_NV_STORE_SN
#define SMM_GATEWAY_NV_STORE_SN		"sn:ffa:751bf801-3dde-4768-a514-0f10aeed1790:0"
#endif

#if defined(UEFI_AUTH_VAR) && !defined(UEFI_INTERNAL_CRYPTO)
/* Default to using the Crypto SP */
#ifndef SMM_GATEWAY_CRYPTO_SN
#define SMM_GATEWAY_CRYPTO_SN "sn:ffa:d9df52d5-16a2-4bb2-9aa4-d26d3b84e8c0:0"
#endif
#endif

/* Default maximum number of UEFI variables */
#ifndef SMM_GATEWAY_MAX_UEFI_VARIABLES
#define SMM_GATEWAY_MAX_UEFI_VARIABLES		(40)
#endif

/**
 * The UEFI variable store index must fit into the RPC shared memory, otherwise
 * load_variable_index/sync_variable_index will fail.
 */
#define SMM_UEFI_VARIABLE_STORE_INDEX_SIZE \
	UEFI_VARIABLE_STORE_INDEX_SIZE(SMM_GATEWAY_MAX_UEFI_VARIABLES)

/**
 * The SP heap must be large enough for storing the UEFI variable index, the RPC shared memory and
 * ~16kB of miscellaneous data.
 */
#define SMM_MIN_HEAP_SIZE \
	SMM_UEFI_VARIABLE_STORE_INDEX_SIZE + RPC_CALLER_SESSION_SHARED_MEMORY_SIZE + 16 * 1024

_Static_assert(SP_HEAP_SIZE > SMM_MIN_HEAP_SIZE, "Please increase SP_HEAP_SIZE");

/* The smm_gateway instance - it's a singleton */
static struct smm_gateway
{
	struct smm_variable_provider smm_variable_provider;
	struct secure_storage_client nv_store_client;
	struct mock_store volatile_store;
	struct service_context *nv_storage_service_context;
	struct rpc_caller_session *nv_storage_session;
#if defined(UEFI_AUTH_VAR) && !defined(UEFI_INTERNAL_CRYPTO)
	struct service_context *crypto_service_context;
	struct rpc_caller_session *crypto_session;
#endif

} smm_gateway_instance;

#if defined(UEFI_AUTH_VAR) && !defined(UEFI_INTERNAL_CRYPTO)
bool create_crypto_binding(void)
{
 	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;

	smm_gateway_instance.crypto_service_context = NULL;
	smm_gateway_instance.crypto_session = NULL;

	smm_gateway_instance.crypto_service_context = service_locator_query(SMM_GATEWAY_CRYPTO_SN);
	if (!smm_gateway_instance.crypto_service_context)
		goto err;

	smm_gateway_instance.crypto_session =
		service_context_open(smm_gateway_instance.crypto_service_context);
	if (!smm_gateway_instance.crypto_session)
		goto err;

	/* Initialize the crypto client */
	psa_status = psa_crypto_client_init(smm_gateway_instance.crypto_session);
	if (psa_status != PSA_SUCCESS)
		goto err;

	psa_status = psa_crypto_init();
	if (psa_status != PSA_SUCCESS)
		goto err;

	return true;

err:
	if (smm_gateway_instance.crypto_session != NULL)
	{
		service_context_close(smm_gateway_instance.crypto_service_context, smm_gateway_instance.crypto_session);
		smm_gateway_instance.crypto_session = NULL;
	}

	if (smm_gateway_instance.crypto_service_context != NULL)
	{
		service_context_relinquish(smm_gateway_instance.crypto_service_context);
		smm_gateway_instance.crypto_service_context = NULL;
	}

	return false;
}
#else
#define create_crypto_binding(a) (true)
#endif

struct rpc_service_interface *smm_gateway_create(uint32_t owner_id)
{
	service_locator_envinit();

	/* todo - add option to use configurable service location */
	smm_gateway_instance.nv_storage_service_context =
		service_locator_query(SMM_GATEWAY_NV_STORE_SN);

	if (!smm_gateway_instance.nv_storage_service_context)
		return NULL;

	smm_gateway_instance.nv_storage_session = service_context_open(
		smm_gateway_instance.nv_storage_service_context);

	if (!smm_gateway_instance.nv_storage_session)
		return NULL;

	/* Initialize a storage client to access the remote NV store */
	struct storage_backend *persistent_backend = secure_storage_client_init(
		&smm_gateway_instance.nv_store_client,
		smm_gateway_instance.nv_storage_session);
	if (!persistent_backend)
		return NULL;

	/* Initialize the volatile storage backend */
	struct storage_backend *volatile_backend  = mock_store_init(
		&smm_gateway_instance.volatile_store);
	if (!volatile_backend)
		return NULL;

	/* Initialize the smm_variable service provider */
	struct rpc_service_interface *service_iface = smm_variable_provider_init(
		&smm_gateway_instance.smm_variable_provider,
 		owner_id,
		SMM_GATEWAY_MAX_UEFI_VARIABLES,
		persistent_backend,
		volatile_backend);

	if (!create_crypto_binding())
		return NULL;

	return service_iface;
}
