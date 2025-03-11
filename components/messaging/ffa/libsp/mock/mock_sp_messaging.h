/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 */

#ifndef LIBSP_MOCK_MOCK_SP_MESSAGING_H_
#define LIBSP_MOCK_MOCK_SP_MESSAGING_H_

#include "../include/sp_messaging.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void expect_sp_msg_wait(const struct sp_msg *msg, sp_result result);

void expect_sp_yield(sp_result result);

void expect_sp_msg_send_direct_req(const struct sp_msg *req,
				   const struct sp_msg *resp,
				   sp_result result);


void expect_sp_msg_send_direct_resp(const struct sp_msg *resp,
				    const struct sp_msg *req,
				    sp_result result);

#if FFA_DIRECT_MSG_ROUTING_EXTENSION
void expect_sp_msg_send_rc_req(const struct sp_msg *req,
			       const struct sp_msg *resp,
			       sp_result result);
#endif /* FFA_DIRECT_MSG_ROUTING_EXTENSION */

#ifdef __cplusplus
}
#endif

#endif /* LIBSP_MOCK_MOCK_SP_MESSAGING_H_ */
