/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2023-2026, Arm Limited and Contributors. All rights reserved.
 */

#ifndef LOG_CLIENT_H
#define LOG_CLIENT_H

#include <service/common/client/service_client.h>
#include <stdbool.h>

#include "components/service/log/backend/log_backend.h"
#include "components/service/log/common/log_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief	Log client instance
 */
struct log_client {
	struct log_backend backend;
	struct service_client client;
};

/**
 * @brief	Initialize a log client
 *
 * A log client is a log backend that makes RPC calls
 * to a remote log provider.
 *
 * @param[in]  context	Instance data
 * @param[in]  rpc_caller RPC caller instance
 *
 * @return	The log error status code
 */
log_status_t log_client_init(struct log_client *context, struct rpc_caller_session *session);

/**
 * @brief	  Deinitialize a log client
 *
 * @param[in]  context   Instance data
 */
void log_client_deinit(struct log_client *context);

#ifdef __cplusplus
}
#endif

#endif /* LOG_CLIENT_H */
