/*
 * Copyright (c) 2020-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SERVICE_LOCATOR_H
#define SERVICE_LOCATOR_H

#include "rpc_caller_session.h"

/*
 * The service_locator puplic interface may be exported as a public interface to
 * a shared library.
 */
#ifdef EXPORT_PUBLIC_INTERFACE_SERVICE_LOCATOR
#define SERVICE_LOCATOR_EXPORTED __attribute__((__visibility__("default")))
#else
#define SERVICE_LOCATOR_EXPORTED
#endif


#ifdef __cplusplus
extern "C" {
#endif

/*
 * The service_locator provides an interface for locating trusted service
 * provider instances and establishing RPC sessions for using the service.
 * The service_locator decouples clients from the details of any particular
 * service deployment.  By accessing trusted services using the service_locator,
 * client code may be reused accross different service deployment scenarios.
 */
struct service_location_strategy;

/*
 * Initialises the singleton service locator.  Must be called once
 * but may be called more than once.
 */
SERVICE_LOCATOR_EXPORTED void service_locator_init(void);

/*
 * An environment specific implementation of this function is
 * needed in order to initialise and register the set of
 * concrete service_location_strategy objects to use when
 * building a deployment for a particular environment.
 */
void service_locator_envinit(void);

/*
 * Register a service_location_strategy for locating service
 * instances.  When attempting to locate a service, the set
 * of registered service_location_strategy objects will be tried
 * in the order of registration.
 */
void service_locator_register_strategy(const struct service_location_strategy *strategy);

/*
 * Query to locate a service instance.  If the given service name
 * corresponds to an available service instance, a service_context
 * is returned.  The client should hang onto this until it has
 * finished using the service.  When the service is no longer needed,
 * the client should call the relinquish method.  Returns NULL
 * if no service is located that corresponds to the service name.
 */
SERVICE_LOCATOR_EXPORTED struct service_context *service_locator_query(const char *sn);

/*
 * The service_context struct represents a service instance to a client
 * after having located the service instance using the service locator.  A
 * service_context object allows a client to open and close RPC sessions
 * associated with the service instance, wherever it happens to be deployed.
 */
struct service_context
{
	void *context;

	struct rpc_caller_session *(*open)(void *context);
	void (*close)(void *context, struct rpc_caller_session *session);
	void (*relinquish)(void *context);
};

/*
 * Provides an abstract interface for a strategy that locates services
 * in a particular way.  The set of registered strategies forms a
 * chain of responsibility for resolving a query made by a clisnt.
 */
struct service_location_strategy
{
	struct service_context *(*query)(const char *sn);
};

/*
 * Open an RPC session in order to use the service associated with this
 * service_context.  The parameter encoding scheme that the client
 * intends to use for serializing RPC parameters must be specified.
 */
SERVICE_LOCATOR_EXPORTED struct rpc_caller_session *service_context_open(struct service_context *s);

/*
 * Close an RPC session.
 */
SERVICE_LOCATOR_EXPORTED void service_context_close(struct service_context *s, struct rpc_caller_session *session_handle);

/*
 * Called by a client when it has finished using a service context.
 */
SERVICE_LOCATOR_EXPORTED void service_context_relinquish(struct service_context *context);


#ifdef __cplusplus
}
#endif

#endif /* SERVICE_LOCATOR_H */
