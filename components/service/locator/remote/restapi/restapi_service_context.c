/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "restapi_service_context.h"

#include <stdlib.h>
#include <string.h>

#include "rpc/http/caller/http_caller.h"
#include "trace.h"

/**
 * A service_context that represents a service instance reached via
 * a remote REST API
 */
struct restapi_service_context {
	struct service_context service_context;
	char rpc_call_url[HTTP_CALLER_MAX_URL_LEN];
};

/* Concrete service_context methods */
static rpc_session_handle restapi_service_context_open(void *context, struct rpc_caller **caller);
static void restapi_service_context_close(void *context, rpc_session_handle session_handle);
static void restapi_service_context_relinquish(void *context);

struct service_context *restapi_service_context_create(const char *service_url)
{
	struct restapi_service_context *new_context =
		(struct restapi_service_context *)calloc(1, sizeof(struct restapi_service_context));

	if (!new_context) {
		EMSG("Failed to alloc restapi_service_context");
		return NULL;
	}

	const size_t service_url_len = strlen(service_url);
	const char *str_call = "call/";
	const size_t str_call_len = strlen(str_call);

	if (service_url_len + str_call_len + 1 > HTTP_CALLER_MAX_URL_LEN) {
		EMSG("service URL is too long");
		return NULL;
	}

	strncpy(new_context->rpc_call_url, service_url, HTTP_CALLER_MAX_URL_LEN - 1);
	strncat(new_context->rpc_call_url, str_call, HTTP_CALLER_MAX_URL_LEN - 1 - service_url_len);

	new_context->service_context.context = new_context;
	new_context->service_context.open = restapi_service_context_open;
	new_context->service_context.close = restapi_service_context_close;
	new_context->service_context.relinquish = restapi_service_context_relinquish;

	return &new_context->service_context;
}

static rpc_session_handle restapi_service_context_open(void *context, struct rpc_caller **caller)
{
	struct restapi_service_context *this_context = (struct restapi_service_context *)context;
	rpc_session_handle session_handle = NULL;

	struct http_caller *http_caller =
		(struct http_caller *)calloc(1, sizeof(struct http_caller));

	if (http_caller) {
		*caller = http_caller_init(http_caller);

		int status = http_caller_open(http_caller, this_context->rpc_call_url);

		if (status == 0) {
			/* Successfully opened session */
			session_handle = http_caller;
		} else {
			/* Failed to open session */
			http_caller_close(http_caller);
			http_caller_deinit(http_caller);
			free(http_caller);
		}
	}

	return session_handle;
}

static void restapi_service_context_close(void *context, rpc_session_handle session_handle)
{
	struct http_caller *http_caller = (struct http_caller *)session_handle;

	(void)context;

	if (http_caller) {
		http_caller_close(http_caller);
		http_caller_deinit(http_caller);
		free(http_caller);
	}
}

static void restapi_service_context_relinquish(void *context)
{
	struct restapi_service_context *this_context = (struct restapi_service_context *)context;

	free(this_context);
}
