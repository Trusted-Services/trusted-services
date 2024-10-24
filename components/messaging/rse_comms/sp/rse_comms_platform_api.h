/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RSE_COMMS_PLATFORM_API_H__
#define __RSE_COMMS_PLATFORM_API_H__

#include <stdint.h>

struct rse_comms_platform;

struct rse_comms_platform *rse_comms_platform_init(void);
int rse_comms_platform_deinit(struct rse_comms_platform *rse_comms_plat);
int rse_comms_platform_invoke(struct rse_comms_platform *rse_comms_plat, uint8_t *resp_buf,
			      uint8_t *req_buf, size_t *resp_len, size_t req_len);
int rse_comms_platform_begin(struct rse_comms_platform *rse_comms_plat, uint8_t *req_buf,
			     size_t req_len);
int rse_comms_platform_end(struct rse_comms_platform *rse_comms_plat);

#endif /* __RSE_COMMS_PLATFORM_API_H__ */
