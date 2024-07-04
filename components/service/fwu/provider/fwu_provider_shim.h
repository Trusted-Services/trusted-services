/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FWU_PROVIDER_SHIM_H
#define FWU_PROVIDER_SHIM_H

#include "rpc/common/endpoint/rpc_service_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief PSA FWU provider shim layer
 *
 * The PSA FWU forwards the function ID and the service status in the beginning of the shared
 * memory. This layer handles the extraction of the function ID and the insertion of service status,
 * so individual function handlers don't have to do that.
 */

struct fwu_provider_shim {
	struct rpc_service_interface shim_rpc_interface;
};

struct rpc_service_interface *fwu_provider_shim_init(
	struct fwu_provider_shim *shim, struct rpc_service_interface *fwu_rpc_interface);

void fwu_provider_shim_deinit(struct fwu_provider_shim *shim);

#ifdef __cplusplus
}
#endif

#endif /* FWU_PROVIDER_SHIM_H */
