/*
 * Copyright (c) 2022-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <assert.h>

#include "rse_comms_protocol.h"

psa_status_t rse_protocol_serialize_msg(struct rpc_caller_interface *caller,
					psa_handle_t handle,
					int16_t type,
					const struct psa_invec *in_vec,
					uint8_t in_len,
					const struct psa_outvec *out_vec,
					uint8_t out_len,
					struct serialized_rse_comms_msg_t *msg,
					size_t *msg_len)
{
	psa_status_t status = PSA_SUCCESS;

	assert(msg != NULL);
	assert(msg_len != NULL);
	assert(in_vec != NULL);

	switch (msg->header.protocol_ver) {
	case RSE_COMMS_PROTOCOL_EMBED:
		status = rse_protocol_embed_serialize_msg(handle, type, in_vec, in_len, out_vec,
							  out_len, &msg->msg.embed, msg_len);
		if (status != PSA_SUCCESS) {
			return status;
		}
		break;
	case RSE_COMMS_PROTOCOL_POINTER_ACCESS:
		status = rse_protocol_pointer_access_serialize_msg(caller, handle, type, in_vec, in_len,
								   out_vec, out_len,
								   &msg->msg.pointer_access,
								   msg_len);
		if (status != PSA_SUCCESS) {
			return status;
		}
		break;
	default:
		return PSA_ERROR_NOT_SUPPORTED;
	}

	*msg_len += sizeof(struct serialized_rse_comms_header_t);

	return PSA_SUCCESS;
}

psa_status_t rse_protocol_deserialize_reply(struct rpc_caller_interface *caller,
					    struct psa_outvec *out_vec,
					    uint8_t out_len,
					    psa_status_t *return_val,
					    const struct serialized_rse_comms_reply_t *reply,
					    size_t reply_size)
{
	assert(reply != NULL);
	assert(return_val != NULL);

	switch (reply->header.protocol_ver) {
	case RSE_COMMS_PROTOCOL_EMBED:
		return rse_protocol_embed_deserialize_reply(out_vec, out_len, return_val,
							    &reply->reply.embed, reply_size);
	case RSE_COMMS_PROTOCOL_POINTER_ACCESS:
		return rse_protocol_pointer_access_deserialize_reply(caller, out_vec, out_len, return_val,
								     &reply->reply.pointer_access,
								     reply_size);
	default:
		return PSA_ERROR_NOT_SUPPORTED;
	}

	return PSA_SUCCESS;
}

psa_status_t rse_protocol_calculate_msg_len(psa_handle_t handle,
					    uint8_t protocol_ver,
					    const struct psa_invec *in_vec,
					    uint8_t in_len,
					    size_t *msg_len)
{
	psa_status_t status = PSA_SUCCESS;

	assert(msg_len != NULL);
	assert(in_vec != NULL);

	switch (protocol_ver) {
	case RSE_COMMS_PROTOCOL_EMBED:
		status = rse_protocol_embed_calculate_msg_len(handle, in_vec, in_len, msg_len);
		if (status != PSA_SUCCESS)
			return status;

		break;
	case RSE_COMMS_PROTOCOL_POINTER_ACCESS:
		status = rse_protocol_pointer_access_calculate_msg_len(handle, in_vec, in_len, msg_len);
		if (status != PSA_SUCCESS)
			return status;
		break;
	default:
		return PSA_ERROR_NOT_SUPPORTED;
	}

	*msg_len += sizeof(struct serialized_rse_comms_header_t);

	return PSA_SUCCESS;
}
