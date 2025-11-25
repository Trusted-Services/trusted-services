/*
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CORSTONE1000_EVENT_HANDLING_H
#define CORSTONE1000_EVENT_HANDLING_H

#include <rpc/common/endpoint/rpc_service_interface.h>
#include <service/common/provider/service_provider.h>
#include <service/common/client/service_client.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The event_provider is a service provider that accepts events
 * and delegates them to a suitable backend.
 */
struct event_provider {
	struct service_provider base_provider;
	struct service_client client;
};

/**
 * \brief Initialize an instance of the event handler
 *
 * @param[in] context The instance to initialize
 *
 * \return An rpc_service_interface or NULL on failure
 */
struct rpc_service_interface *corstone1000_event_provider_init(struct event_provider *context);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CORSTONE1000_EVENT_HANDLING_H */
