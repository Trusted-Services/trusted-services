/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <trace.h>

#include "platform/interface/mhu_interface.h"
#include "rse_comms_messenger_api.h"
#include "rse_comms_platform_api.h"

struct rse_comms_platform {
	struct platform_mhu_driver rx_dev;
	struct platform_mhu_driver tx_dev;
};

struct rse_comms_platform *rse_comms_platform_init(void)
{
	struct rse_comms_platform *rse_comms_plat = NULL;
	int ret = 0;

	rse_comms_plat = calloc(1, sizeof(*rse_comms_plat));
	if (!rse_comms_plat) {
		EMSG("rse_comms calloc dev failed");
		return NULL;
	}

	ret = platform_mhu_create(&rse_comms_plat->rx_dev, "mhu-receiver", true);
	if (ret < 0)
		goto free_plat;

	ret = platform_mhu_create(&rse_comms_plat->tx_dev, "mhu-sender", false);
	if (ret < 0)
		goto free_rx_dev;

	return rse_comms_plat;

free_rx_dev:
	platform_mhu_destroy(&rse_comms_plat->rx_dev);
free_plat:
	free(rse_comms_plat);

	return NULL;
}

int rse_comms_platform_deinit(struct rse_comms_platform *rse_comms_plat)
{
	if (!rse_comms_plat)
		return -1;

	platform_mhu_destroy(&rse_comms_plat->rx_dev);
	platform_mhu_destroy(&rse_comms_plat->tx_dev);

	free(rse_comms_plat);

	return 0;
}

int rse_comms_platform_invoke(struct rse_comms_platform *rse_comms_plat, uint8_t *resp_buf,
			      uint8_t *req_buf, size_t *resp_len, size_t req_len)
{
	struct platform_mhu_driver *rx_dev = NULL;
	struct platform_mhu_driver *tx_dev = NULL;
	int err = 0;

	if (!rse_comms_plat || !resp_buf || !req_buf)
		return -1;

	rx_dev = &rse_comms_plat->rx_dev;
	tx_dev = &rse_comms_plat->tx_dev;

	if (!tx_dev->iface || !tx_dev->iface->send)
		return -1;

	err = tx_dev->iface->send(tx_dev->context, req_buf, req_len);
	if (err != 0) {
		EMSG("mhu send data failed!");
		return -1;
	}

	if (!rx_dev->iface || !rx_dev->iface->wait_data || !rx_dev->iface->receive)
		return -1;

	err = rx_dev->iface->wait_data(rx_dev->context);
	if (err != 0) {
		EMSG("mhu wait for signal failed!");
		return -1;
	}

	err = rx_dev->iface->receive(rx_dev->context, resp_buf, resp_len);
	if (err != 0) {
		EMSG("mhu receive data failed!");
		return -1;
	}

	return 0;
}

int rse_comms_platform_begin(struct rse_comms_platform *rse_comms_plat, uint8_t *req_buf,
			     size_t req_len)
{
	return 0;
}

int rse_comms_platform_end(struct rse_comms_platform *rse_comms_plat)
{
	return 0;
}
