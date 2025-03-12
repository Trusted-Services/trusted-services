/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 */

#ifndef LIBSP_MOCK_MOCK_SP_NOTIFICATION_H_
#define LIBSP_MOCK_MOCK_SP_NOTIFICATION_H_

#include "../include/sp_notification.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void expect_sp_notification_bind(uint16_t sender, uint16_t receiver,
				 struct sp_notif_bind_flags *flags, uint64_t notification_bitmap,
				 sp_result result);
void expect_sp_notification_unbind(uint16_t sender, uint16_t receiver, uint64_t notification_bitmap,
				   sp_result result);
void expect_sp_notification_set(uint16_t sender, uint16_t receiver,
				struct sp_notif_set_flags *flags, uint64_t notification_bitmap,
				sp_result result);
void expect_sp_notification_get(uint16_t sender, uint16_t receiver,
				uint64_t *sp_notification_bitmap, uint64_t *vm_notification_bitmap,
				uint32_t *spm_notification_bitmap, uint32_t *hv_notification_bitmap,
				sp_result result);

#ifdef __cplusplus
}
#endif

#endif /* LIBSP_MOCK_MOCK_SP_NOTIFICATION_H_ */
