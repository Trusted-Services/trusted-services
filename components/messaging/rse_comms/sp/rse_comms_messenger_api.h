/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RSE_COMMS_MESSENGER_API_H__
#define __RSE_COMMS_MESSENGER_API_H__

#include <stddef.h>
#include <stdint.h>

struct rse_comms_messenger {
	void *msg;
	void *platform;
};

int rse_comms_messenger_init(struct rse_comms_messenger *rse_comms);
void rse_comms_messenger_deinit(struct rse_comms_messenger *rse_comms);
int rse_comms_messenger_call_invoke(struct rse_comms_messenger *rse_comms, uint8_t **resp_buf,
				    size_t *resp_len);
int rse_comms_messenger_call_begin(struct rse_comms_messenger *rse_comms, uint8_t **req_buf,
				   size_t req_len);
void rse_comms_messenger_call_end(struct rse_comms_messenger *rse_comms);

#endif /* __RSE_COMMS_MESSENGER_API_H__ */
