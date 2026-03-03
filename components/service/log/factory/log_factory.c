/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2023-2026, Arm Limited and Contributors. All rights reserved.
 */

#include "service/log/factory/log_factory.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "components/common/trace/include/trace.h"
#include "components/rpc/common/caller/rpc_caller.h"
#include "components/rpc/common/caller/rpc_caller_session.h"
#include "components/rpc/ts_rpc/caller/sp/ts_rpc_caller_sp.h"
#include "components/service/log/provider/log_uuid.h"
#include "protocols/rpc/common/packed-c/status.h"
#include "service/log/client/log_client.h"
#include "sp_discovery.h"
#include "trace.h"

/**
 * A log factory that creates log backends that is used
 * to access logging SP from a separate SP.
 */
struct logger {
	struct log_client client;
	struct rpc_caller_interface caller;
	struct rpc_caller_session session;
	bool in_use;
};

/* Only supports construction of a single instance */
static struct logger backend_instance = { .in_use = false };

static const struct rpc_uuid logging_service_uuid = { .uuid = TS_LOG_SERVICE_UUID };

/*
 * Log factory create that is included in the code for other SP's
 */
void log_factory_create(void)
{
	struct logger *new_backend = &backend_instance;
	rpc_status_t sp_init_status = RPC_ERROR_INTERNAL;
	rpc_status_t session_open_status = RPC_ERROR_INTERNAL;
	log_status_t log_status = LOG_STATUS_GENERIC_ERROR;

	if (new_backend->in_use) {
		log_status == LOG_STATUS_SUCCESS;
		goto end;
	}

	sp_init_status = ts_rpc_caller_sp_init(&new_backend->caller);
	if (sp_init_status != RPC_SUCCESS){
		DMSG("Failed to initialize SP: %d", sp_init_status);
		goto end;
	}

	session_open_status = rpc_caller_session_find_and_open(&new_backend->session, &new_backend->caller,
						      &logging_service_uuid, 4096);
	if (session_open_status != RPC_SUCCESS) {
		DMSG("Failed to open session: %d", session_open_status);
		goto end;
	}

	log_status = log_client_init(&new_backend->client, &new_backend->session);
	new_backend->in_use = (log_status == LOG_STATUS_SUCCESS);

end:
	if (log_status != LOG_STATUS_SUCCESS) {
		if (sp_init_status == RPC_SUCCESS)
			(void)ts_rpc_caller_sp_deinit(&new_backend->caller);

		EMSG("Logging service discovery failed, falling back to console log: %d", log_status);
	} else {
		IMSG("Logging service discovery successful");
	}
}

/*
 * Returns the log_backend instance if log_factory_create
 * run successfully, otherwise NULL.
 */
struct log_backend *log_factory_get_backend_instance(void)
{
	if (backend_instance.in_use)
		return &backend_instance.client.backend;

	return NULL;
}

/*
 * Remove the log factory
 */
void log_factory_destroy(struct logger *backend)
{
	if (backend) {
		rpc_caller_session_close(&backend_instance.session);
		ts_rpc_caller_sp_deinit(&backend_instance.caller);
		backend_instance.in_use = false;
	}
}
