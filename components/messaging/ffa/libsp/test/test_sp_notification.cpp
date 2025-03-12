// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 */

#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include <stdint.h>
#include <string.h>
#include "mock_ffa_api.h"
#include "../include/sp_notification.h"

/* Picked functions */
extern "C" {
uint32_t build_notif_bind_flags(struct sp_notif_bind_flags *flags);
uint32_t build_notif_set_flags(struct sp_notif_set_flags *flags);
}

TEST_GROUP(sp_notification)
{
	TEST_TEARDOWN()
	{
		mock().checkExpectations();
		mock().clear();
	}

	const uint16_t sender = 1;
	const uint16_t receiver = 2;
	struct sp_notif_bind_flags bind_flags_percpu = { .per_cpu = 1 };
	struct sp_notif_bind_flags bind_flags_global = { .per_cpu = 0 };
	struct sp_notif_set_flags set_flags = { .per_cpu = 1,
						.delay_sched_rec_intr = 0,
						.receiver_vcpu_id = 42 };
	struct sp_notif_set_flags set_flags_invalid = { .per_cpu = 0,
							.delay_sched_rec_intr = 1,
							.receiver_vcpu_id = 42 };
	uint64_t test_sp_notification_bitmap = 0xAAAAAAAABBBBBBBB;
	uint64_t test_vm_notification_bitmap = 0xCCCCCCCCDDDDDDDD;
	uint64_t test_fw_notification_bitmap = 0xEEEEEEEEFFFFFFFF;
};

TEST(sp_notification, build_notif_bind_flags_global)
{
	uint32_t result = 0;

	result = build_notif_bind_flags(&bind_flags_global);
	UNSIGNED_LONGS_EQUAL(0, result);
}

TEST(sp_notification, build_notif_bind_flags_percpu)
{
	uint32_t expected_result = bind_flags_global.per_cpu;
	uint32_t result = 0;

	result = build_notif_bind_flags(&bind_flags_global);
	UNSIGNED_LONGS_EQUAL(expected_result, result);
}

TEST(sp_notification, build_notif_set_flags)
{
	uint32_t expected_result = set_flags.receiver_vcpu_id
					   << FFA_NOTIF_SET_FLAGS_RECEIVER_VCPU_SHIFT |
				   FFA_NOTIF_SET_FLAGS_PER_VCPU_NOTIFICATIONS;
	uint32_t result = 0;

	result = build_notif_set_flags(&set_flags);
	UNSIGNED_LONGS_EQUAL(expected_result, result);
}

TEST(sp_notification, build_notif_set_flags_invalid)
{
	uint32_t expected_result = FFA_NOTIF_SET_FLAGS_DELAY_SCHEDULE_RECEIVER;
	uint32_t result = 0;

	result = build_notif_set_flags(&set_flags_invalid);
	UNSIGNED_LONGS_EQUAL(expected_result, result);
}

TEST(sp_notification, sp_notification_bind_error)
{
	ffa_result result = FFA_ABORTED;
	expect_ffa_notification_bind(sender, receiver, build_notif_bind_flags(&bind_flags_percpu),
				     test_sp_notification_bitmap, result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_notification_bind(sender, receiver, &bind_flags_percpu,
					 test_sp_notification_bitmap));
}

TEST(sp_notification, sp_notification_bind)
{
	ffa_result result = FFA_OK;
	expect_ffa_notification_bind(sender, receiver, build_notif_bind_flags(&bind_flags_percpu),
				     test_sp_notification_bitmap, result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_notification_bind(sender, receiver, &bind_flags_percpu,
					 test_sp_notification_bitmap));
}

TEST(sp_notification, sp_notification_unbind_error)
{
	ffa_result result = FFA_ABORTED;
	expect_ffa_notification_unbind(sender, receiver, test_sp_notification_bitmap, result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_notification_unbind(sender, receiver, test_sp_notification_bitmap));
}

TEST(sp_notification, sp_notification_unbind)
{
	ffa_result result = FFA_OK;
	expect_ffa_notification_unbind(sender, receiver, test_sp_notification_bitmap, result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_notification_unbind(sender, receiver, test_sp_notification_bitmap));
}

TEST(sp_notification, sp_notification_set_error)
{
	ffa_result result = FFA_ABORTED;
	expect_ffa_notification_set(sender, receiver, build_notif_set_flags(&set_flags),
				    test_sp_notification_bitmap, result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_notification_set(sender, receiver, &set_flags, test_sp_notification_bitmap));
}

TEST(sp_notification, sp_notification_set_invalid_flag)
{
	LONGS_EQUAL(SP_RESULT_INVALID_PARAMETERS,
		    sp_notification_set(sender, receiver, &set_flags_invalid,
					test_sp_notification_bitmap));
}

TEST(sp_notification, sp_notification_set)
{
	ffa_result result = FFA_OK;
	expect_ffa_notification_set(sender, receiver, build_notif_set_flags(&set_flags),
				    test_sp_notification_bitmap, result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_notification_set(sender, receiver, &set_flags, test_sp_notification_bitmap));
}

TEST(sp_notification, sp_notification_get_error)
{
	ffa_result result = FFA_ABORTED;
	uint64_t sp_notification_bitmap = 0;
	uint64_t vm_notification_bitmap = 0;
	uint32_t spm_notification_bitmap = 0;
	uint32_t hv_notification_bitmap = 0;

	expect_ffa_notification_get(sender, receiver, 0xF, &test_sp_notification_bitmap,
				    &test_vm_notification_bitmap, &test_fw_notification_bitmap,
				    result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_notification_get(sender, receiver, &sp_notification_bitmap,
					&vm_notification_bitmap, &spm_notification_bitmap,
					&hv_notification_bitmap));
}

TEST(sp_notification, sp_notification_get_null_param)
{
	ffa_result result = FFA_OK;
	uint64_t dummy = 0;

	expect_ffa_notification_get(sender, receiver, 0, &dummy, &dummy, &dummy, result);

	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_notification_get(sender, receiver, NULL, NULL, NULL, NULL));
}

TEST(sp_notification, sp_notification_get)
{
	ffa_result result = FFA_OK;
	uint64_t sp_notification_bitmap = 0;
	uint64_t vm_notification_bitmap = 0;
	uint32_t spm_notification_bitmap = 0;
	uint32_t hv_notification_bitmap = 0;

	expect_ffa_notification_get(sender, receiver, 0xF, &test_sp_notification_bitmap,
				    &test_vm_notification_bitmap, &test_fw_notification_bitmap,
				    result);
	LONGS_EQUAL(SP_RESULT_FFA(result),
		    sp_notification_get(sender, receiver, &sp_notification_bitmap,
					&vm_notification_bitmap, &spm_notification_bitmap,
					&hv_notification_bitmap));
	LONGS_EQUAL(sp_notification_bitmap, test_sp_notification_bitmap);
	LONGS_EQUAL(vm_notification_bitmap, test_vm_notification_bitmap);
	LONGS_EQUAL(spm_notification_bitmap, test_fw_notification_bitmap & 0xFFFFFFFF);
	LONGS_EQUAL(hv_notification_bitmap, test_fw_notification_bitmap >> 32 & 0xFFFFFFFF);
}
