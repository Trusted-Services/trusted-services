/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <protocols/rpc/common/packed-c/encoding.h>
#include <service/secure_storage/backend/secure_storage_client/secure_storage_client.h>
#include <service/secure_storage/frontend/psa/ps/ps_frontend.h>
#include <service_locator.h>
#include <stdio.h>

#include "libpsats.h"
#include "trace.h"

static struct rpc_caller_session *rpc_session;
static struct service_context *ps_service_context;
static struct secure_storage_client ps_storage_client;

LIBPSATS_EXPORTED psa_status_t libpsats_init_ps_context(const char *service_name)
{
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;

	if (rpc_session || ps_service_context) {
		EMSG("The client is already initialized\n");
		return result;
	}

	service_locator_init();

	ps_service_context = service_locator_query(service_name);

	if (!ps_service_context) {
		EMSG("Failed to discover service\n");
		return result;
	}

	rpc_session = service_context_open(ps_service_context);

	if (!rpc_session) {
		EMSG("Failed to open rpc session\n");
		libpsats_deinit_ps_context();
		return result;
	}

	struct storage_backend *ps_storage_backend =
		secure_storage_client_init(&ps_storage_client, rpc_session);

	if (!ps_storage_backend) {
		EMSG("Failed to initialize storage backend\n");
		libpsats_deinit_ps_context();
		return result;
	}

	result = psa_ps_frontend_init(ps_storage_backend);

	return result;
}

LIBPSATS_EXPORTED void libpsats_deinit_ps_context(void)
{
	psa_ps_frontend_init(NULL);
	secure_storage_client_deinit(&ps_storage_client);

	if (ps_service_context && rpc_session) {
		service_context_close(ps_service_context, rpc_session);
		rpc_session = NULL;
	}

	if (ps_service_context) {
		service_context_relinquish(ps_service_context);
		ps_service_context = NULL;
	}
}
