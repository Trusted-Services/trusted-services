// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 */

#include "sp_notification.h"
#include "sp_api.h"
#include <assert.h>
#include <string.h>

static uint32_t build_notif_bind_flags(struct sp_notif_bind_flags *flags)
{
	uint32_t res = 0;

	if (flags->per_cpu)
		res |= FFA_NOTIF_BIND_FLAGS_PER_VCPU_NOTIFICATIONS;

	return res;
}

static uint32_t build_notif_set_flags(struct sp_notif_set_flags *flags)
{
	uint32_t res = 0;

	if (flags->per_cpu) {
		res |= FFA_NOTIF_SET_FLAGS_PER_VCPU_NOTIFICATIONS;
		res |= SHIFT_U32(flags->receiver_vcpu_id, FFA_NOTIF_SET_FLAGS_RECEIVER_VCPU_SHIFT);
	}

	if (flags->delay_sched_rec_intr)
		res |= FFA_NOTIF_SET_FLAGS_DELAY_SCHEDULE_RECEIVER;

	return res;
}

sp_result sp_notification_bind(uint16_t sender, uint16_t receiver,
			       struct sp_notif_bind_flags *flags, uint64_t notification_bitmap)
{
	return SP_RESULT_FFA(ffa_notification_bind(sender, receiver, build_notif_bind_flags(flags),
						   notification_bitmap));
}

sp_result sp_notification_unbind(uint16_t sender, uint16_t receiver, uint64_t notification_bitmap)
{
	return SP_RESULT_FFA(ffa_notification_unbind(sender, receiver, notification_bitmap));
}

sp_result sp_notification_set(uint16_t sender, uint16_t receiver, struct sp_notif_set_flags *flags,
			      uint64_t notification_bitmap)
{
	if (!flags->per_cpu && flags->receiver_vcpu_id)
		return SP_RESULT_INVALID_PARAMETERS;

	return SP_RESULT_FFA(ffa_notification_set(sender, receiver, build_notif_set_flags(flags),
						  notification_bitmap));
}

sp_result sp_notification_get(uint16_t sender, uint16_t receiver, uint64_t *sp_notification_bitmap,
			      uint64_t *vm_notification_bitmap, uint32_t *spm_notification_bitmap,
			      uint32_t *hv_notification_bitmap)
{
	uint32_t flags = 0;
	uint64_t buff_framework_notification_bitmap = 0;
	uint64_t buff_sp_notification_bitmap = 0;
	uint64_t buff_vm_notification_bitmap = 0;
	sp_result result = 0;

	if (sp_notification_bitmap)
		flags |= FFA_NOTIF_GET_FLAGS_PENDING_SP_NOTIF;
	if (vm_notification_bitmap)
		flags |= FFA_NOTIF_GET_FLAGS_PENDING_VM_NOTIF;
	if (spm_notification_bitmap)
		flags |= FFA_NOTIF_GET_FLAGS_PENDING_SPM_NOTIF;
	if (hv_notification_bitmap)
		flags |= FFA_NOTIF_GET_FLAGS_PENDING_HV_NOTIF;

	result = SP_RESULT_FFA(ffa_notification_get(sender, receiver, flags, &buff_sp_notification_bitmap,
						    &buff_vm_notification_bitmap,
						    &buff_framework_notification_bitmap));
	if (sp_notification_bitmap)
		*sp_notification_bitmap = buff_sp_notification_bitmap;
	if (vm_notification_bitmap)
		*vm_notification_bitmap = buff_vm_notification_bitmap;
	if (spm_notification_bitmap)
		*spm_notification_bitmap = buff_framework_notification_bitmap & 0xFFFFFFFF;
	if (hv_notification_bitmap)
		*hv_notification_bitmap = buff_framework_notification_bitmap >> 32 & 0xFFFFFFFF;

	return result;
}
