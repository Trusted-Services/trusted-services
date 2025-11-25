// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 */

#include <CppUTestExt/MockSupport.h>
#include <CppUTest/TestHarness.h>
#include "mock_sp_notification.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

TEST_GROUP(mock_sp_notification)
{
	TEST_TEARDOWN()
	{
		mock().checkExpectations();
		mock().clear();
	}

	static const sp_result result = -1;
	struct sp_notif_bind_flags bind_flags = { .per_cpu = 1 };
	struct sp_notif_set_flags set_flags = { .per_cpu = 1,
						.delay_sched_rec_intr = 0,
						.receiver_vcpu_id = 42 };
};

TEST(mock_sp_notification, sp_notification_bind)
{
	const uint16_t sender = 1;
	const uint16_t receiver = 2;
	const uint64_t notification_bitmap = 0x55555555;

	expect_sp_notification_bind(sender, receiver, &bind_flags, notification_bitmap, result);
	LONGS_EQUAL(result,
		    sp_notification_bind(sender, receiver, &bind_flags, notification_bitmap));
}

TEST(mock_sp_notification, sp_notification_unbind)
{
	const uint16_t sender = 1;
	const uint16_t receiver = 2;
	const uint64_t notification_bitmap = 0x55555555;

	expect_sp_notification_unbind(sender, receiver, notification_bitmap, result);
	LONGS_EQUAL(result, sp_notification_unbind(sender, receiver, notification_bitmap));
}

TEST(mock_sp_notification, sp_notification_set)
{
	const uint16_t sender = 1;
	const uint16_t receiver = 2;
	const uint64_t notification_bitmap = 0x55555555;

	expect_sp_notification_set(sender, receiver, &set_flags, notification_bitmap, result);
	LONGS_EQUAL(result, sp_notification_set(sender, receiver, &set_flags, notification_bitmap));
}

TEST(mock_sp_notification, sp_notification_get)
{
	const uint16_t sender = 1;
	const uint16_t receiver = 2;
	uint64_t expected_notification_bitmap = 0x55555555;
	uint64_t sp_notification_bitmap = 0;
	uint64_t vm_notification_bitmap = 0;
	uint32_t spm_notification_bitmap = 0;
	uint32_t hv_notification_bitmap = 0;

	expect_sp_notification_get(sender, receiver, &expected_notification_bitmap,
				   &expected_notification_bitmap,
				   (uint32_t *)&expected_notification_bitmap,
				   (uint32_t *)&expected_notification_bitmap, result);
	LONGS_EQUAL(result, sp_notification_get(sender, receiver, &sp_notification_bitmap,
						&vm_notification_bitmap, &spm_notification_bitmap,
						&hv_notification_bitmap));
	LONGS_EQUAL(sp_notification_bitmap, expected_notification_bitmap);
	LONGS_EQUAL(vm_notification_bitmap, expected_notification_bitmap);
	LONGS_EQUAL(spm_notification_bitmap, expected_notification_bitmap);
	LONGS_EQUAL(hv_notification_bitmap, expected_notification_bitmap);
}
