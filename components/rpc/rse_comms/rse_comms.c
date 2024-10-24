/*
 * Copyright (c) 2022-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <string.h>

#include "protocols/rpc/common/packed-c/status.h"
#include "psa/client.h"
#include "rse_comms_caller.h"
#include "rse_comms_protocol.h"
#include "trace.h"

/* This value should be set according to the implemented channels
 * in the MHU. As only the Embed protocol is supported at the moment,
 * it is set big enough to fit even the largest messages.
 */
#ifndef COMMS_MHU_MSG_SIZE
#define COMMS_MHU_MSG_SIZE 0x2200
#endif

static uint8_t select_protocol_version(const struct psa_invec *in_vec, size_t in_len,
				       const struct psa_outvec *out_vec, size_t out_len)
{
	size_t comms_embed_msg_min_size;
	size_t comms_embed_reply_min_size;
	size_t in_size_total = 0;
	size_t out_size_total = 0;
	size_t i;

	for (i = 0U; i < in_len; ++i) {
		in_size_total += in_vec[i].len;
	}
	for (i = 0U; i < out_len; ++i) {
		out_size_total += out_vec[i].len;
	}

	comms_embed_msg_min_size = sizeof(struct serialized_rse_comms_header_t) +
				   sizeof(struct rse_embed_msg_t) - PLAT_RSE_COMMS_PAYLOAD_MAX_SIZE;

	comms_embed_reply_min_size = sizeof(struct serialized_rse_comms_header_t) +
				     sizeof(struct rse_embed_reply_t) -
				     PLAT_RSE_COMMS_PAYLOAD_MAX_SIZE;

	/* Use embed if we can pack into one message and reply, else use
	 * pointer_access. The underlying MHU transport protocol uses a
	 * single uint32_t to track the length, so the amount of data that
	 * can be in a message is 4 bytes less than mhu_get_max_message_size
	 * reports.
	 *
	 * TODO tune this with real performance numbers, it's possible a
	 * pointer_access message is less performant than multiple embed
	 * messages due to ATU configuration costs to allow access to the
	 * pointers.
	 */
	if ((comms_embed_msg_min_size + in_size_total >
	     COMMS_MHU_MSG_SIZE - sizeof(uint32_t)) ||
	    (comms_embed_reply_min_size + out_size_total >
	     COMMS_MHU_MSG_SIZE - sizeof(uint32_t))) {
		return RSE_COMMS_PROTOCOL_POINTER_ACCESS;
	} else {
		return RSE_COMMS_PROTOCOL_EMBED;
	}
}

psa_status_t __psa_call(struct rpc_caller_interface *caller, psa_handle_t handle, int32_t client_id,
			int32_t type, const struct psa_invec *in_vec, size_t in_len,
			struct psa_outvec *out_vec, size_t out_len)
{
	static uint8_t seq_num = 1U;
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	psa_status_t psa_status = PSA_ERROR_INVALID_ARGUMENT;
	psa_status_t return_val = PSA_ERROR_INVALID_ARGUMENT;
	rse_comms_call_handle rpc_handle = (rse_comms_call_handle)caller;
	size_t req_len = 0;
	size_t resp_len = 0;
	struct serialized_rse_comms_msg_t *req = NULL;
	struct serialized_rse_comms_reply_t *reply = NULL;
	uint8_t protocol_ver = 0;

	protocol_ver = select_protocol_version(in_vec, in_len, out_vec, out_len);

	psa_status = rse_protocol_calculate_msg_len(handle, protocol_ver, in_vec, in_len, &req_len);
	if (psa_status != PSA_SUCCESS) {
		EMSG("Message size calculation failed: %d", psa_status);
		return psa_status;
	}

	rpc_handle = rse_comms_caller_begin(caller, (uint8_t **)&req, req_len);
	if (!rpc_handle) {
		EMSG("Could not get handle");
		return PSA_ERROR_GENERIC_ERROR;
	}

	req->header.seq_num = seq_num;
	/* No need to distinguish callers (currently concurrent calls are not supported). */
	req->header.client_id = client_id;
	req->header.protocol_ver = protocol_ver;

	psa_status = rse_protocol_serialize_msg(handle, type, in_vec, in_len, out_vec, out_len, req,
						&req_len);
	if (psa_status != PSA_SUCCESS) {
		EMSG("Serialize msg failed: %d", psa_status);
		return psa_status;
	}

	DMSG("Sending rse_comms message");
	DMSG("protocol_ver=%u", req->header.protocol_ver);
	DMSG("seq_num=%u", req->header.seq_num);
	DMSG("client_id=%u", req->header.client_id);

	resp_len = sizeof(*reply);

	rpc_status = rse_comms_caller_invoke(rpc_handle, 0, (uint8_t **)&reply, &resp_len);
	if (rpc_status != RPC_SUCCESS) {
		EMSG("Invoke failed: %d", rpc_status);
		return PSA_ERROR_GENERIC_ERROR;
	}

	DMSG("Received rse_comms reply");
	DMSG("protocol_ver=%u", reply->header.protocol_ver);
	DMSG("seq_num=%u", reply->header.seq_num);
	DMSG("client_id=%u", reply->header.client_id);
	DMSG("resp_len=%lu", resp_len);

	psa_status = rse_protocol_deserialize_reply(out_vec, out_len, &return_val, reply, resp_len);
	if (psa_status != PSA_SUCCESS) {
		EMSG("Protocol deserialize reply failed: %d", psa_status);
		return psa_status;
	}

	DMSG("Return_val=%d", return_val);

	rse_comms_caller_end(rpc_handle);

	seq_num++;

	return return_val;
}

psa_status_t psa_call_client_id(struct rpc_caller_interface *caller, psa_handle_t psa_handle,
				int32_t client_id, int32_t type, const struct psa_invec *in_vec,
				size_t in_len, struct psa_outvec *out_vec, size_t out_len)
{
	return __psa_call(caller, psa_handle, client_id, type, in_vec, in_len, out_vec, out_len);
}

psa_status_t psa_call(struct rpc_caller_interface *caller, psa_handle_t psa_handle, int32_t type,
		      const struct psa_invec *in_vec, size_t in_len, struct psa_outvec *out_vec,
		      size_t out_len)
{
	return __psa_call(caller, psa_handle, 0, type, in_vec, in_len, out_vec, out_len);
}
