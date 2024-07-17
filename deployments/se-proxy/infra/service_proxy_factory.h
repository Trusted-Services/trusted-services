/*
 * Copyright (c) 2021-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SERVICE_PROXY_FACTORY_H
#define SERVICE_PROXY_FACTORY_H

#include "components/rpc/common/endpoint/rpc_service_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

struct rpc_service_interface *attest_proxy_create(void);
struct rpc_service_interface *crypto_proxy_create(void);
struct rpc_service_interface *ps_proxy_create(void);
struct rpc_service_interface *its_proxy_create(void);
struct rpc_service_interface *fwu_proxy_create(void);

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_PROXY_FACTORY_H */
