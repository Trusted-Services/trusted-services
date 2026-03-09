// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2023-2026, Arm Limited and Contributors. All rights reserved.
 */

#include "log_client.h"

#include <string.h>

#include "components/service/log/factory/log_factory.h"
#include "protocols/rpc/common/packed-c/status.h"
#include "protocols/service/log/packed-c/log_proto.h"
#include "rpc_caller_session.h"
#include "util.h"
#include "trace.h"
#include "ffa_api.h"

/*
 * Custom logging function, which sends the log message to
 * the logging backend, but can fall back to the default
 * logging interface of the system.
 */
static void log_client_trace_puts(const char *str)
{
	struct log_backend *log_backend;
	log_status_t log_status = LOG_STATUS_GENERIC_ERROR;

	/* log message using logging sp */
	log_backend = log_factory_get_backend_instance();

	/* in case logging to sp fails we still want to log it with FFA */
	if (log_backend) {
		log_status = log_backend->interface->puts(log_backend->context, str);
		if (log_status == LOG_STATUS_SUCCESS)
			return;
	}

	/* log message using FFA CONSOLE LOG */
	trace_puts(str);
}

static log_status_t psa_status_to_log_status(psa_status_t psa_status)
{
	log_status_t log_status;
	switch (psa_status) {
		case PSA_SUCCESS:
			log_status = LOG_STATUS_SUCCESS;
			break;
		case PSA_ERROR_INVALID_ARGUMENT:
			log_status = LOG_STATUS_INVALID_PARAMETER;
			break;
		default:
			log_status = LOG_STATUS_GENERIC_ERROR;
			break;
	}
	return log_status;
}

/*
 * Client function for sending string to Logging SP.
 */
log_status_t log_client_puts(void *context, const char *msg)
{
	struct log_client *log_client = (struct log_client *)context;
	uint8_t *request = NULL;
	uint8_t *response = NULL;
	size_t request_length = 0;
	size_t response_length = 0;
	size_t msg_length = 0;
	struct log_request *request_desc = NULL;
	rpc_call_handle handle = 0;
	rpc_status_t rpc_status = TS_RPC_CALL_ACCEPTED;
	log_status_t log_status = LOG_STATUS_GENERIC_ERROR;

	service_status_t service_status = LOG_STATUS_SUCCESS;

	if (log_client->client.session == NULL)
		return LOG_STATUS_GENERIC_ERROR;

	/* Validating input parameters */
	if (msg == NULL)
		return LOG_STATUS_INVALID_PARAMETER;

	msg_length = strlen(msg);

	/* Add one for null termination */
	if (ADD_OVERFLOW(msg_length, 1, &msg_length))
		return LOG_STATUS_INVALID_PARAMETER;

	if (ADD_OVERFLOW(sizeof(*request_desc), msg_length, &request_length))
		return LOG_STATUS_INVALID_PARAMETER;

	/* RPC call */
	handle = rpc_caller_session_begin(log_client->client.session, &request, request_length, 0);
	if (handle) {
		request_desc = (struct log_request *)request;
		memcpy(&request_desc->msg, msg, msg_length);
		request_desc->msg_length = msg_length;

		rpc_status = rpc_caller_session_invoke(handle, TS_LOG_OPCODE_PUTS, &response,
						       &response_length, &service_status);

		if (rpc_status == PSA_SUCCESS)
			log_status = service_status;

		rpc_caller_session_end(handle);
	}

	return log_status;
}

/*
 * Client initialization function.
 */
log_status_t log_client_init(struct log_client *context, struct rpc_caller_session *session)
{
	psa_status_t psa_status = service_client_init(&context->client, session);
	if (psa_status != PSA_SUCCESS)
		return psa_status_to_log_status(psa_status);

	static const struct log_backend_interface interface = { log_client_puts };

	context->backend.context = context;
	context->backend.interface = &interface;

	trace_puts_interface = &log_client_trace_puts;

	return LOG_STATUS_SUCCESS;
}
