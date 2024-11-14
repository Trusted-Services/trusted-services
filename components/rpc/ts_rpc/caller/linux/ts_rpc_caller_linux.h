/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TS_RPC_CALLER_LINUX_H
#define TS_RPC_CALLER_LINUX_H

#include "rpc_caller.h"

#ifdef __cplusplus
extern "C" {
#endif

RPC_CALLER_EXPORTED
rpc_status_t ts_rpc_caller_linux_init(struct rpc_caller_interface *rpc_caller);

RPC_CALLER_EXPORTED
rpc_status_t ts_rpc_caller_linux_deinit(struct rpc_caller_interface *rpc_caller);

#ifdef __cplusplus
}
#endif

#endif /* TS_RPC_CALLER_LINUX_H */
