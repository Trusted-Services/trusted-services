/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
 */

#ifndef LIBSP_INCLUDE_SP_MESSAGING_H_
#define LIBSP_INCLUDE_SP_MESSAGING_H_

#include "sp_api_defines.h"
#include "sp_api_types.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SP_MSG_ARG_COUNT (4)

/**
 * @brief      SP message type
 */
struct sp_msg {
	uint16_t source_id;
	uint16_t destination_id;
	uint32_t args[SP_MSG_ARG_COUNT];
};

/**
 * @brief      Wait for a message and returns it.
 * @param[out] msg   The received message
 *
 * @return     The SP API result
 */
sp_result sp_msg_wait(struct sp_msg *msg);

/**
 * @brief      Sends a request message and waits for the response message
 *             which it returns then.
 *
 * @param[in]  req   The request message
 * @param[out] resp  The response message
 *
 * @return     The SP API result
 */
sp_result sp_msg_send_direct_req(const struct sp_msg *req, struct sp_msg *resp);

/**
 * @brief      Sends a response message and waits for a new request which it
 *             returns then.
 *
 * @param[in]  resp  The response message
 * @param[out] req   The request message
 *
 * @return     The SP API result
 */
sp_result sp_msg_send_direct_resp(const struct sp_msg *resp,
				  struct sp_msg *req);

#ifdef __cplusplus
}
#endif

#endif /* LIBSP_INCLUDE_SP_MESSAGING_H_ */