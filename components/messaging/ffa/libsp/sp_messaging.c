// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 */

#include "ffa_api.h"
#include "sp_api_defines.h"
#include "sp_messaging.h"
#if FFA_DIRECT_MSG_ROUTING_EXTENSION
#include "ffa_direct_msg_routing_extension.h"
#endif

#include <string.h>

static void pack_ffa_direct_msg(const struct sp_msg *msg,
				struct ffa_direct_msg *ffa_msg)
{
	ffa_msg->source_id = msg->source_id;
	ffa_msg->destination_id = msg->destination_id;

	if (msg->is_64bit_message) {
		ffa_msg->function_id = FFA_TO_64_BIT_FUNC(0);
		memcpy(ffa_msg->args.args64, msg->args.args64, sizeof(msg->args.args64));
	} else {
		memcpy(ffa_msg->args.args32, msg->args.args32, sizeof(msg->args.args32));
	}
}

static void unpack_ffa_direct_msg(const struct ffa_direct_msg *ffa_msg,
				  struct sp_msg *msg)
{
	if (ffa_msg->function_id == FFA_MSG_SEND_DIRECT_REQ_32 ||
	    ffa_msg->function_id == FFA_MSG_SEND_DIRECT_RESP_32) {
		/*
		 * Handling 32 bit request or response
		 */
		msg->source_id = ffa_msg->source_id;
		msg->destination_id = ffa_msg->destination_id;
		msg->is_64bit_message = false;

		memcpy(msg->args.args32, ffa_msg->args.args32, sizeof(msg->args.args32));
	} else if (ffa_msg->function_id == FFA_MSG_SEND_DIRECT_REQ_64 ||
		   ffa_msg->function_id == FFA_MSG_SEND_DIRECT_RESP_64) {
		/*
		 * Handling 64 bit request or response
		 */
		msg->source_id = ffa_msg->source_id;
		msg->destination_id = ffa_msg->destination_id;
		msg->is_64bit_message = true;

		memcpy(msg->args.args64, ffa_msg->args.args64, sizeof(msg->args.args64));
	} else {
		/* Success has no message parameters */
		*msg = (struct sp_msg){ 0 };
	}
}

sp_result sp_msg_wait(struct sp_msg *msg)
{
	ffa_result ffa_res = FFA_OK;
	struct ffa_direct_msg ffa_msg = { 0 };

	if (!msg)
		return SP_RESULT_INVALID_PARAMETERS;

	ffa_res = ffa_msg_wait(&ffa_msg);
	if (ffa_res != FFA_OK) {
		*msg = (struct sp_msg){ 0 };
		return SP_RESULT_FFA(ffa_res);
	}

#if FFA_DIRECT_MSG_ROUTING_EXTENSION
	ffa_res = ffa_direct_msg_routing_ext_wait_post_hook(&ffa_msg);
	if (ffa_res != FFA_OK) {
		*msg = (struct sp_msg){ 0 };
		return SP_RESULT_FFA(ffa_res);
	}
#endif

	unpack_ffa_direct_msg(&ffa_msg, msg);

	return SP_RESULT_OK;
}

sp_result sp_yield(void)
{
	return SP_RESULT_FFA(ffa_yield());
}

sp_result sp_msg_send_direct_req(const struct sp_msg *req, struct sp_msg *resp)
{
	ffa_result ffa_res = FFA_OK;
	struct ffa_direct_msg ffa_req = { 0 };
	struct ffa_direct_msg ffa_resp = { 0 };

	if (!resp)
		return SP_RESULT_INVALID_PARAMETERS;

	if (!req) {
		*resp = (struct sp_msg){ 0 };
		return SP_RESULT_INVALID_PARAMETERS;
	}

	pack_ffa_direct_msg(req, &ffa_req);

#if FFA_DIRECT_MSG_ROUTING_EXTENSION
	ffa_res = ffa_direct_msg_routing_ext_req_pre_hook(&ffa_req);
	if (ffa_res != FFA_OK) {
		*resp = (struct sp_msg){ 0 };
		return SP_RESULT_INVALID_PARAMETERS;
	}
#endif

	if (req->is_64bit_message)
		ffa_res = ffa_msg_send_direct_req_64(
			ffa_req.source_id, ffa_req.destination_id,
			ffa_req.args.args64[0], ffa_req.args.args64[1],
			ffa_req.args.args64[2], ffa_req.args.args64[3],
			ffa_req.args.args64[4], &ffa_resp);
	else
		ffa_res = ffa_msg_send_direct_req_32(
			ffa_req.source_id, ffa_req.destination_id,
			ffa_req.args.args32[0], ffa_req.args.args32[1],
			ffa_req.args.args32[2], ffa_req.args.args32[3],
			ffa_req.args.args32[4], &ffa_resp);

	if (ffa_res != FFA_OK) {
#if FFA_DIRECT_MSG_ROUTING_EXTENSION
		ffa_direct_msg_routing_ext_req_error_hook();
#endif
		*resp = (struct sp_msg){ 0 };
		return SP_RESULT_FFA(ffa_res);
	}

#if FFA_DIRECT_MSG_ROUTING_EXTENSION
	ffa_res = ffa_direct_msg_routing_ext_req_post_hook(&ffa_resp);
	if (ffa_res != SP_RESULT_OK) {
		*resp = (struct sp_msg){ 0 };
		return SP_RESULT_FFA(ffa_res);
	}
#endif

	unpack_ffa_direct_msg(&ffa_resp, resp);

	return SP_RESULT_OK;
}

sp_result sp_msg_send_direct_resp(const struct sp_msg *resp, struct sp_msg *req)
{
	ffa_result ffa_res = FFA_OK;
	struct ffa_direct_msg ffa_resp = { 0 };
	struct ffa_direct_msg ffa_req = { 0 };

	if (!req)
		return SP_RESULT_INVALID_PARAMETERS;

	if (!resp) {
		*req = (struct sp_msg){ 0 };
		return SP_RESULT_INVALID_PARAMETERS;
	}

	pack_ffa_direct_msg(resp, &ffa_resp);

#if FFA_DIRECT_MSG_ROUTING_EXTENSION
	ffa_res = ffa_direct_msg_routing_ext_resp_pre_hook(&ffa_resp);
	if (ffa_res != FFA_OK) {
		*req = (struct sp_msg){ 0 };
		return SP_RESULT_INVALID_PARAMETERS;
	}
#endif

	if (resp->is_64bit_message)
		ffa_res = ffa_msg_send_direct_resp_64(
			ffa_resp.source_id, ffa_resp.destination_id,
			ffa_resp.args.args64[0], ffa_resp.args.args64[1],
			ffa_resp.args.args64[2], ffa_resp.args.args64[3],
			ffa_resp.args.args64[4], &ffa_req);
	else
		ffa_res = ffa_msg_send_direct_resp_32(
			ffa_resp.source_id, ffa_resp.destination_id,
			ffa_resp.args.args32[0], ffa_resp.args.args32[1],
			ffa_resp.args.args32[2], ffa_resp.args.args32[3],
			ffa_resp.args.args32[4], &ffa_req);

	if (ffa_res != FFA_OK) {
#if FFA_DIRECT_MSG_ROUTING_EXTENSION
		ffa_direct_msg_routing_ext_resp_error_hook();
#endif
		*req = (struct sp_msg){ 0 };
		return SP_RESULT_FFA(ffa_res);
	}

#if FFA_DIRECT_MSG_ROUTING_EXTENSION
	ffa_res = ffa_direct_msg_routing_ext_resp_post_hook(&ffa_req);
	if (ffa_res != SP_RESULT_OK) {
		*req = (struct sp_msg){ 0 };
		return SP_RESULT_FFA(ffa_res);
	}
#endif

	unpack_ffa_direct_msg(&ffa_req, req);

	return SP_RESULT_OK;
}

#if FFA_DIRECT_MSG_ROUTING_EXTENSION
sp_result sp_msg_send_rc_req(const struct sp_msg *req, struct sp_msg *resp)
{
	ffa_result ffa_res = FFA_OK;
	struct ffa_direct_msg ffa_req = { 0 };
	struct ffa_direct_msg ffa_resp = { 0 };

	if (!resp)
		return SP_RESULT_INVALID_PARAMETERS;

	if (!req) {
		*resp = (struct sp_msg){ 0 };
		return SP_RESULT_INVALID_PARAMETERS;
	}

	pack_ffa_direct_msg(req, &ffa_req);

	ffa_res = ffa_direct_msg_routing_ext_rc_req_pre_hook(&ffa_req);
	if (ffa_res != FFA_OK) {
		*resp = (struct sp_msg){ 0 };
		return SP_RESULT_INVALID_PARAMETERS;
	}

	ffa_res = ffa_msg_send_direct_resp_32(ffa_req.source_id,
					   ffa_req.destination_id,
					   ffa_req.args.args32[0], ffa_req.args.args32[1],
					   ffa_req.args.args32[2], ffa_req.args.args32[3],
					   ffa_req.args.args32[4], &ffa_resp);

	if (ffa_res != FFA_OK) {
		ffa_direct_msg_routing_ext_rc_req_error_hook();
		*resp = (struct sp_msg){ 0 };
		return SP_RESULT_FFA(ffa_res);
	}

	ffa_res = ffa_direct_msg_routing_ext_rc_req_post_hook(&ffa_resp);
	if (ffa_res != SP_RESULT_OK) {
		*resp = (struct sp_msg){ 0 };
		return SP_RESULT_FFA(ffa_res);
	}

	unpack_ffa_direct_msg(&ffa_resp, resp);

	return SP_RESULT_OK;
}
#endif
