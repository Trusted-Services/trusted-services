/*
 * Copyright (c) 2026, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _MOCK_RPMB_FRONTEND_H_
#define _MOCK_RPMB_FRONTEND_H_

#include "components/service/common/include/psa/error.h"
#include <stddef.h>
#include <stdint.h>

struct rpmb_frontend;

void expect_rpmb_frontend_init(struct rpmb_frontend *context, psa_status_t result);
void expect_rpmb_frontend_create(struct rpmb_frontend *context, uint32_t dev_id,
				 psa_status_t result);
void expect_rpmb_frontend_destroy(struct rpmb_frontend *context);

void expect_rpmb_frontend_block_size(struct rpmb_frontend *context, size_t block_size,
				     psa_status_t result);
void expect_rpmb_frontend_block_count(struct rpmb_frontend *context, size_t block_count,
				      psa_status_t result);

void expect_rpmb_frontend_write(struct rpmb_frontend *context, uint16_t block_index,
				const uint8_t *data, size_t data_len, size_t block_count,
				psa_status_t result);

void expect_rpmb_frontend_read(struct rpmb_frontend *context, uint16_t block_index,
			       uint8_t *data, size_t data_len, size_t block_count,
			       psa_status_t result);

#endif
