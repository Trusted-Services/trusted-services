/*
 * Copyright (c) 2021-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "spffa_service_context.h"
#include "rpc/ts_rpc/caller/sp/ts_rpc_caller_sp.h"
#include "sp_discovery.h"
#include <stdlib.h>
#include <string.h>

/* Concrete service_context methods */
static struct rpc_caller_session *sp_ts_service_context_open(void *context);
static void sp_ts_service_context_close(void *context, struct rpc_caller_session *session);
static void sp_ts_service_context_relinquish(void *context);


struct sp_ts_service_context *spffa_service_context_create(const struct rpc_uuid *service_uuid)
{
	struct sp_ts_service_context *new_context =
		(struct sp_ts_service_context *)calloc(1, sizeof(struct sp_ts_service_context));
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;

	if (!new_context)
		return NULL;

	rpc_status = ts_rpc_caller_sp_init(&new_context->caller);
	if (rpc_status != RPC_SUCCESS) {
		free(new_context);
		return NULL;
	}

	memcpy(&new_context->service_uuid, service_uuid, sizeof(new_context->service_uuid));

	new_context->service_context.context = new_context;
	new_context->service_context.open = sp_ts_service_context_open;
	new_context->service_context.close = sp_ts_service_context_close;
	new_context->service_context.relinquish = sp_ts_service_context_relinquish;

	return new_context;
}

static struct rpc_caller_session *sp_ts_service_context_open(void *context)
{
	struct sp_ts_service_context *this_context = (struct sp_ts_service_context *)context;
	struct rpc_caller_session *session =
		(struct rpc_caller_session *)calloc(1, sizeof(struct rpc_caller_session));
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;

	if (!session)
		return NULL;

	rpc_status = rpc_caller_session_find_and_open(session, &this_context->caller,
						      &this_context->service_uuid,
						      RPC_CALLER_SESSION_SHARED_MEMORY_SIZE);
	if (rpc_status != RPC_SUCCESS) {
		free(session);
		return NULL;
	}

	return session;
}

static void sp_ts_service_context_close(void *context, struct rpc_caller_session *session)
{
	(void)context;

	rpc_caller_session_close(session);
	free(session);
}

static void sp_ts_service_context_relinquish(void *context)
{
	free(context);
}
