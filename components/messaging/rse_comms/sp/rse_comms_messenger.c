/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <trace.h>

#include "protocols/rpc/common/packed-c/status.h"
#include "rse_comms_messenger_api.h"
#include "rse_comms_platform_api.h"

struct rse_comms_msg {
	uint8_t *req_buf;
	size_t req_len;
	uint8_t *resp_buf;
};

int rse_comms_messenger_init(struct rse_comms_messenger *rse_comms)
{
	int ret = 0;

	if (!rse_comms || rse_comms->msg || rse_comms->platform)
		return -1;

	rse_comms->msg = calloc(1, sizeof(struct rse_comms_msg));
	if (!rse_comms->msg)
		return -1;

	rse_comms->platform = rse_comms_platform_init();
	if (!rse_comms->platform) {
		EMSG("Platform init failed");
		free(rse_comms->msg);
		rse_comms->msg = NULL;
		return ret;
	}

	return 0;
}

void rse_comms_messenger_deinit(struct rse_comms_messenger *rse_comms)
{
	struct rse_comms_msg *msg = NULL;

	if (!rse_comms)
		return;

	if (rse_comms->msg) {
		msg = (struct rse_comms_msg *)rse_comms->msg;

		if (msg->req_buf)
			free(msg->req_buf);
		if (msg->resp_buf)
			free(msg->resp_buf);

		free(msg);
		rse_comms->msg = NULL;
	}

	rse_comms_platform_deinit(rse_comms->platform);
	rse_comms->platform = NULL;
}

int rse_comms_messenger_call_invoke(struct rse_comms_messenger *rse_comms, uint8_t **resp_buf,
				    size_t *resp_len)
{
	struct rse_comms_msg *msg = NULL;
	int ret = 0;

	if (!rse_comms || !resp_buf || !resp_len) {
		EMSG("Invalid arguments");
		return -1;
	}

	if (!rse_comms->msg) {
		EMSG("msg is null");
		return -1;
	}

	msg = (struct rse_comms_msg *)rse_comms->msg;
	*resp_buf = calloc(1, *resp_len);

	if (!(*resp_buf))
		return -1;

	ret = rse_comms_platform_invoke(rse_comms->platform, *resp_buf, msg->req_buf, resp_len,
					msg->req_len);

	if (ret < 0) {
		free(*resp_buf);
		*resp_buf = NULL;
		return ret;
	}

	msg->resp_buf = *resp_buf;

	return 0;
}

int rse_comms_messenger_call_begin(struct rse_comms_messenger *rse_comms, uint8_t **req_buf,
				   size_t req_len)
{
	int ret = 0;
	struct rse_comms_msg *msg = NULL;

	if (!rse_comms || !req_buf || !rse_comms->msg)
		return -1;

	if (req_len > UINT32_MAX || req_len == 0) {
		EMSG("req_len invalid: %lu", req_len);
		return -1;
	}

	msg = (struct rse_comms_msg *)rse_comms->msg;

	if (msg->req_buf)
		return -1;

	msg->req_buf = calloc(1, req_len);

	if (!msg->req_buf)
		return -1;

	*req_buf = msg->req_buf;
	msg->req_len = req_len;

	ret = rse_comms_platform_begin(rse_comms->platform, *req_buf, req_len);

	return ret;
}

void rse_comms_messenger_call_end(struct rse_comms_messenger *rse_comms)
{
	int ret = 0;
	struct rse_comms_msg *msg = NULL;

	if (!rse_comms || !rse_comms->msg)
		return;

	msg = (struct rse_comms_msg *)rse_comms->msg;

	if (msg->req_buf)
		free(msg->req_buf);

	if (msg->resp_buf)
		free(msg->resp_buf);

	msg->req_len = 0;
	msg->req_buf = NULL;
	msg->resp_buf = NULL;

	ret = rse_comms_platform_end(rse_comms->platform);

	if (ret < 0) {
		EMSG("Platform end failed: %d", ret);
		return;
	}
}
