/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PSA_FWU_IPC_H
#define PSA_FWU_IPC_H

#include <psa/error.h>
#include "rpc_caller_session.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      Initialize a PSA FWU ipc client
 *
 * A PSA FWU ipc client makes RPC calls to a remote FWU service.
 *
 * @param[in]  rpc_caller RPC caller instance
 *
 * @return     A status indicating the success/failure of the operation
 */
psa_status_t psa_fwu_ipc_init(struct rpc_caller_session *session);

/**
 * @brief      Deinitialize a PSA FWU ipc client
 *
 */
void psa_fwu_ipc_deinit(void);

/**
 * @brief      Return the most recent RPC status
 *
 * May be used to obtain information about an RPC error that resulted
 * in an API operation failure
 *
 * @return     Most recent RPC operation status
 */
int psa_fwu_rpc_status(void);

#ifdef __cplusplus
}
#endif

#endif /* PSA_FWU_IPC_H */

