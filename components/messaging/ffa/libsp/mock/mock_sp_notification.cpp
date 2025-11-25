// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2025, Arm Limited. All rights reserved.
 */

#include <CppUTestExt/MockSupport.h>
#include "mock_sp_notification.h"

void expect_sp_notification_bind(uint16_t sender, uint16_t receiver,
				 struct sp_notif_bind_flags *flags, uint64_t notification_bitmap,
				 sp_result result)
{
	mock().expectOneCall("sp_notification_bind")
		.withUnsignedIntParameter("sender", sender)
		.withUnsignedIntParameter("receiver", receiver)
		.withMemoryBufferParameter("flags", (const uint8_t *)flags, sizeof(*flags))
		.withUnsignedIntParameter("notification_bitmap", notification_bitmap)
		.andReturnValue(result);
}

sp_result sp_notification_bind(uint16_t sender, uint16_t receiver,
			       struct sp_notif_bind_flags *flags, uint64_t notification_bitmap)
{
	return mock()
		.actualCall("sp_notification_bind")
		.withUnsignedIntParameter("sender", sender)
		.withUnsignedIntParameter("receiver", receiver)
		.withMemoryBufferParameter("flags", (const uint8_t *)flags, sizeof(*flags))
		.withUnsignedIntParameter("notification_bitmap", notification_bitmap)
		.returnIntValue();
}

void expect_sp_notification_unbind(uint16_t sender, uint16_t receiver, uint64_t notification_bitmap,
				   sp_result result)
{
	mock().expectOneCall("sp_notification_unbind")
		.withUnsignedIntParameter("sender", sender)
		.withUnsignedIntParameter("receiver", receiver)
		.withUnsignedIntParameter("notification_bitmap", notification_bitmap)
		.andReturnValue(result);
}

sp_result sp_notification_unbind(uint16_t sender, uint16_t receiver, uint64_t notification_bitmap)
{
	return mock()
		.actualCall("sp_notification_unbind")
		.withUnsignedIntParameter("sender", sender)
		.withUnsignedIntParameter("receiver", receiver)
		.withUnsignedIntParameter("notification_bitmap", notification_bitmap)
		.returnIntValue();
}

void expect_sp_notification_set(uint16_t sender, uint16_t receiver,
				struct sp_notif_set_flags *flags, uint64_t notification_bitmap,
				sp_result result)
{
	mock().expectOneCall("sp_notification_set")
		.withUnsignedIntParameter("sender", sender)
		.withUnsignedIntParameter("receiver", receiver)
		.withMemoryBufferParameter("flags", (const uint8_t *)flags, sizeof(*flags))
		.withUnsignedIntParameter("notification_bitmap", notification_bitmap)
		.andReturnValue(result);
}

sp_result sp_notification_set(uint16_t sender, uint16_t receiver, struct sp_notif_set_flags *flags,
			      uint64_t notification_bitmap)
{
	return mock()
		.actualCall("sp_notification_set")
		.withUnsignedIntParameter("sender", sender)
		.withUnsignedIntParameter("receiver", receiver)
		.withMemoryBufferParameter("flags", (const uint8_t *)flags, sizeof(*flags))
		.withUnsignedIntParameter("notification_bitmap", notification_bitmap)
		.returnIntValue();
}

void expect_sp_notification_get(uint16_t sender, uint16_t receiver,
				uint64_t *sp_notification_bitmap, uint64_t *vm_notification_bitmap,
				uint32_t *spm_notification_bitmap, uint32_t *hv_notification_bitmap,
				sp_result result)
{
	mock().expectOneCall("sp_notification_get")
		.withUnsignedIntParameter("sender", sender)
		.withUnsignedIntParameter("receiver", receiver)
		.withOutputParameterReturning("sp_notification_bitmap", sp_notification_bitmap,
					      sizeof(*sp_notification_bitmap))
		.withOutputParameterReturning("vm_notification_bitmap", vm_notification_bitmap,
					      sizeof(*vm_notification_bitmap))
		.withOutputParameterReturning("spm_notification_bitmap", spm_notification_bitmap,
					      sizeof(*spm_notification_bitmap))
		.withOutputParameterReturning("hv_notification_bitmap", hv_notification_bitmap,
					      sizeof(*hv_notification_bitmap))
		.andReturnValue(result);
}

sp_result sp_notification_get(uint16_t sender, uint16_t receiver, uint64_t *sp_notification_bitmap,
			      uint64_t *vm_notification_bitmap, uint32_t *spm_notification_bitmap,
			      uint32_t *hv_notification_bitmap)
{
	return mock()
		.actualCall("sp_notification_get")
		.withUnsignedIntParameter("sender", sender)
		.withUnsignedIntParameter("receiver", receiver)
		.withOutputParameter("sp_notification_bitmap", sp_notification_bitmap)
		.withOutputParameter("vm_notification_bitmap", vm_notification_bitmap)
		.withOutputParameter("spm_notification_bitmap", spm_notification_bitmap)
		.withOutputParameter("hv_notification_bitmap", hv_notification_bitmap)
		.returnIntValue();
}
