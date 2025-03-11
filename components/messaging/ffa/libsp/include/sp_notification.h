/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 */

#ifndef SP_NOTIFICATION_H_
#define SP_NOTIFICATION_H_

#include "sp_api_defines.h"
#include "sp_api_types.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      Notification flags of FFA_NOTIFICATION_BIND API
 */
struct sp_notif_bind_flags {
	/**
	 * true: All notifications in the bitmap are per-vCPU notifications
	 * false: All notifications in the bitmap are global notifications
	 */
	bool per_cpu;
};

/**
 * @brief      Notification flags of FFA_NOTIFICATION_SET API
 */
struct sp_notif_set_flags {
	/**
	 * true: All notifications in the bitmap are per-vCPU notifications
	 * false: All notifications in the bitmap are global notifications
	 */
	bool per_cpu;

	/**
	 * Delay Schedule Receiver interrupt flag
	 */
	bool delay_sched_rec_intr;

	/**
	 * Receiver vCPU ID.
	 */
	uint16_t receiver_vcpu_id;
};

/**
 * @brief 	Requests the partition manager to bind notifications specified
 * 		in the Notification bitmap parameter to the Sender endpoint.
 *
 * @param[in]  sender              Sender endpoint ID
 * @param[in]  receiver            Receiver endpoint ID
 * @param[in]  flags               Notification flags
 * @param[in]  notification_bitmap Bitmap to identify the notifications which the
 *                                 Sender endpoint is allowed to signal.
 * @return                         Error status code
 */
sp_result sp_notification_bind(uint16_t sender, uint16_t receiver,
			       struct sp_notif_bind_flags *flags, uint64_t notification_bitmap);

/**
 * @brief 	Requests the partition manager to unbind notifications specified
 *              in the Notification bitmap parameter to the Sender endpoint.
 *
 * @param[in]  sender              Sender endpoint ID
 * @param[in]  receiver            Receiver endpoint ID
 * @param[in]  notification_bitmap Bitmap to identify the notifications which the
 *                                 sender endpoint is allowed to signal.
 * @return                         Error status code
 */
sp_result sp_notification_unbind(uint16_t sender, uint16_t receiver, uint64_t notification_bitmap);

/**
 * @brief 	Requests the partition manager to signal notifications specified
 *              in the Notification bitmap parameter to the Sender endpoint.
 *
 * @param[in]  sender              Sender endpoint ID
 * @param[in]  receiver            Receiver endpoint ID
 * @param[in]  flags               Notification flags
 * @param[in]  notification_bitmap Bitmap to identify the notifications which the
 *                                 sender endpoint is allowed to signal.
 * @return                         Error status code
 */
sp_result sp_notification_set(uint16_t sender, uint16_t receiver, struct sp_notif_set_flags *flags,
			      uint64_t notification_bitmap);

/**
 * @brief 	Requests the partition manager to request notifications specified
 *              in the Notification bitmap parameter to the Sender endpoint.
 *
 * @param[in]  sender                        Sender endpoint ID
 * @param[in]  receiver                      Receiver endpoint ID
 * @param[out] sp_notification_bitmap        Pending notifications received from SPs.
 * @param[out] vm_notification_bitmap        Pending notifications received from VMs.
 * @param[out] spm_notification_bitmap       Pending notifications received from the SPM.
 * @param[out] hv_notification_bitmap        Pending notifications received from the Hypervisor.
 * @return                                   Error status code
 */
sp_result sp_notification_get(uint16_t sender, uint16_t receiver, uint64_t *sp_notification_bitmap,
			      uint64_t *vm_notification_bitmap, uint32_t *spm_notification_bitmap,
			      uint32_t *hv_notification_bitmap);

#ifdef __cplusplus
}
#endif

#endif /* SP_NOTIFICATION_H_ */
