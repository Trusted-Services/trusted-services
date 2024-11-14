/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RSE_COMMS_CALLER_H__
#define __RSE_COMMS_CALLER_H__

#include "rpc_caller.h"
#include "rse_comms_messenger_api.h"

typedef void *rse_comms_call_handle;

rpc_status_t rse_comms_caller_init(struct rpc_caller_interface *rpc_caller);
rpc_status_t rse_comms_caller_deinit(struct rpc_caller_interface *rpc_caller);

rse_comms_call_handle rse_comms_caller_begin(struct rpc_caller_interface *caller,
					     uint8_t **request_buffer, size_t request_length);

rpc_status_t rse_comms_caller_invoke(rse_comms_call_handle handle, uint32_t opcode,
				     uint8_t **response_buffer, size_t *response_length);

rpc_status_t rse_comms_caller_end(rse_comms_call_handle handle);

#endif /* __RSE_COMMS_CALLER_H__ */
