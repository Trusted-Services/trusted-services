/*
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FWU_PROVIDER_H
#define FWU_PROVIDER_H

#include "protocols/rpc/common/packed-c/encoding.h"
#include "rpc/common/endpoint/rpc_service_interface.h"
#include "service/common/provider/service_provider.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Interface dependencies
 */
struct fwu_provider_serializer;
struct update_agent;

/**
 * \brief fwu_provider instance structure
 *
 * An instance of the fwu_provider presents the service level interface for
 * remote access to the fwu service. In addition to handling incoming call
 * requests, the fwu_provider is responsible for access control, call parameter
 * serialization/deserialization and parameter sanitation. Request are delegated
 * to the associated update_agent.
 */
struct fwu_provider {
	struct service_provider base_provider;
	const struct fwu_provider_serializer *serializers[TS_RPC_ENCODING_LIMIT];
	struct update_agent *update_agent;
};

/**
 * \brief Initialise a fwu_provider
 *
 * \param[in] context         The subject fwu_provider context
 * \param[in] update_agent    The associated update_agent
 *
 * \return A pointer to the exposed rpc_interface or NULL on failure
 */
struct rpc_service_interface *fwu_provider_init(struct fwu_provider *context,
						struct update_agent *update_agent);

/**
 * \brief De-initialise a fwu_provider
 *
 * \param[in] context    The subject fwu_provider context
 */
void fwu_provider_deinit(struct fwu_provider *context);

/**
 * \brief Register a serializer
 *
 * \param[in] context    The subject fwu_provider context
 * \param[in] encoding   The encoding scheme
 * \param[in] serializer The serializer
 */
void fwu_provider_register_serializer(struct fwu_provider *context, unsigned int encoding,
				      const struct fwu_provider_serializer *serializer);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* FWU_PROVIDER_H */
