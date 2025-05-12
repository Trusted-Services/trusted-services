/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <config/interface/config_store.h>
#include <platform/interface/device_region.h>
#include <platform/interface/mhu_interface.h>
#include <stdlib.h>

#include "mhu.h"
#include "mhu_v3_x.h"

static int mhu_adapter_send(void *context, unsigned char *send_buffer, size_t size)
{
	struct mhu_v3_x_dev_t *dev = NULL;

	if (!context || !send_buffer || size == 0)
		return -1;

	dev = (struct mhu_v3_x_dev_t *)context;

	if (mhu_send_data(dev, (uint8_t *)send_buffer, size) != MHU_ERR_NONE)
		return -1;

	return 0;
}

static int mhu_adapter_receive(void *context, unsigned char *receive_buffer, size_t *size)
{
	struct mhu_v3_x_dev_t *dev = NULL;

	if (!context || !receive_buffer || !size)
		return -1;

	dev = (struct mhu_v3_x_dev_t *)context;

	if (mhu_receive_data(dev, (uint8_t *)receive_buffer, size) != MHU_ERR_NONE)
		return -1;

	return 0;
}

static int mhu_adapter_wait_data(void *context)
{
	struct mhu_v3_x_dev_t *dev = NULL;

	if (!context)
		return -1;

	dev = (struct mhu_v3_x_dev_t *)context;

	if (mhu_wait_data(dev) != MHU_ERR_NONE)
		return -1;

	return 0;
}

static int mhu_adapter_signal_and_wait_for_clear(void *context, uint32_t value)
{
	struct mhu_v3_x_dev_t *dev = NULL;

	if (!context)
		return -1;

	dev = (struct mhu_v3_x_dev_t *)context;

	if (signal_and_wait_for_clear(dev, value) != MHU_ERR_NONE)
		return -1;

	return 0;
}

static int mhu_adapter_wait_for_signal_and_clear(void *context, uint32_t value)
{
	struct mhu_v3_x_dev_t *dev = NULL;

	if (!context)
		return -1;

	dev = (struct mhu_v3_x_dev_t *)context;

	if (wait_for_signal_and_clear(dev, value) == MHU_ERR_NONE)
		return -1;

	return 0;
}

int platform_mhu_create(struct platform_mhu_driver *driver, const char *object_name,
			bool is_receiver)
{
	struct mhu_v3_x_dev_t *new_instance = NULL;
	enum mhu_error_t status = MHU_ERR_NONE;
	struct device_region device_region = { 0 };
	static const struct platform_mhu_iface iface = {
		.send = mhu_adapter_send,
		.receive = mhu_adapter_receive,
		.wait_data = mhu_adapter_wait_data,
		.signal_and_wait_for_clear = mhu_adapter_signal_and_wait_for_clear,
		.wait_for_signal_and_clear = mhu_adapter_wait_for_signal_and_clear,
	};

	if (!driver || !object_name)
		return -1;

	/* Default to leaving the driver in a safe but inoperable state. */
	driver->iface = &iface;
	driver->context = NULL;

	if (!config_store_query(CONFIG_CLASSIFIER_DEVICE_REGION, object_name, 0, &device_region,
				sizeof(device_region)))
		return -1;

	new_instance = calloc(1, sizeof(struct mhu_v3_x_dev_t));
	if (!new_instance)
		return -1;

	new_instance->base = device_region.base_addr;

	if (is_receiver) {
		new_instance->frame = MHU_V3_X_MBX_FRAME;
		status = mhu_init_receiver(new_instance);
	} else {
		new_instance->frame = MHU_V3_X_PBX_FRAME;
		status = mhu_init_sender(new_instance);
	}

	if (status != MHU_ERR_NONE) {
		free(new_instance);
		return -1;
	}

	driver->context = new_instance;

	return 0;
}

void platform_mhu_destroy(struct platform_mhu_driver *driver)
{
	if (!driver || !driver->context)
		return;

	free(driver->context);
	driver->context = NULL;
}
