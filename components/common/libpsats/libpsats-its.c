/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <protocols/rpc/common/packed-c/encoding.h>
#include <service/secure_storage/backend/secure_storage_client/secure_storage_client.h>
#include <service/secure_storage/frontend/psa/its/its_frontend.h>
#include <service_locator.h>
#include <stdio.h>

#include "libpsats.h"
#include "trace.h"

static struct rpc_caller_session *rpc_session;
static struct service_context *its_service_context;
static struct secure_storage_client its_storage_client;

LIBPSATS_EXPORTED psa_status_t libpsats_init_its_context(const char *service_name)
{
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;

	if (rpc_session || its_service_context) {
		EMSG("The client is already initialized\n");
		return result;
	}

	service_locator_init();

	its_service_context = service_locator_query(service_name);

	if (!its_service_context) {
		EMSG("Failed to discover service\n");
		return result;
	}

	rpc_session = service_context_open(its_service_context);

	if (!rpc_session) {
		EMSG("Failed to open rpc session\n");
		libpsats_deinit_its_context();
		return result;
	}

	struct storage_backend *its_storage_backend =
		secure_storage_client_init(&its_storage_client, rpc_session);

	if (!its_storage_backend) {
		EMSG("Failed to initialize storage backend\n");
		libpsats_deinit_its_context();
		return result;
	}

	result = psa_its_frontend_init(its_storage_backend);

	return result;
}

LIBPSATS_EXPORTED void libpsats_deinit_its_context(void)
{
	psa_its_frontend_init(NULL);
	secure_storage_client_deinit(&its_storage_client);

	if (its_service_context && rpc_session) {
		service_context_close(its_service_context, rpc_session);
		rpc_session = NULL;
	}

	if (its_service_context) {
		service_context_relinquish(its_service_context);
		its_service_context = NULL;
	}
}
