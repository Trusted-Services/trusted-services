/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <protocols/rpc/common/packed-c/encoding.h>
#include <service/crypto/client/psa/psa_crypto_client.h>
#include <service_locator.h>
#include <stdio.h>

#include "libpsats.h"
#include "trace.h"

static struct rpc_caller_session *rpc_session;
static struct service_context *crypto_service_context;

LIBPSATS_EXPORTED psa_status_t libpsats_init_crypto_context(const char *service_name)
{
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;

	if (rpc_session || crypto_service_context) {
		EMSG("The client is already initialized\n");
		return result;
	}

	service_locator_init();

	crypto_service_context = service_locator_query(service_name);

	if (!crypto_service_context) {
		EMSG("Failed to discover service\n");
		return result;
	}

	rpc_session = service_context_open(crypto_service_context);

	if (!rpc_session) {
		EMSG("Failed to open rpc session\n");
		libpsats_deinit_crypto_context();
		return result;
	}

	result = psa_crypto_client_init(rpc_session);

	if (result)
		EMSG("psa_crypto_client_init failed\n");

	return result;
}

LIBPSATS_EXPORTED void libpsats_deinit_crypto_context(void)
{
	psa_crypto_client_deinit();

	if (crypto_service_context && rpc_session) {
		service_context_close(crypto_service_context, rpc_session);
		rpc_session = NULL;
	}

	if (crypto_service_context) {
		service_context_relinquish(crypto_service_context);
		crypto_service_context = NULL;
	}
}
