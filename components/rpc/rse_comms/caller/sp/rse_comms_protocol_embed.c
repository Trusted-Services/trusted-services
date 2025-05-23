/*
 * Copyright (c) 2022-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <assert.h>
#include <string.h>

#include "rse_comms_protocol_common.h"
#include "rse_comms_protocol_embed.h"

psa_status_t rse_protocol_embed_serialize_msg(psa_handle_t handle,
					      int16_t type,
					      const struct psa_invec *in_vec,
					      uint8_t in_len,
					      const struct psa_outvec *out_vec,
					      uint8_t out_len,
					      struct rse_embed_msg_t *msg,
					      size_t *msg_len)
{
	uint32_t payload_size = 0;
	uint32_t i;

	assert(msg != NULL);
	assert(msg_len != NULL);
	assert(in_vec != NULL);

	msg->ctrl_param = PARAM_PACK(type, in_len, out_len);
	msg->handle = handle;

	/* Fill msg iovec lengths */
	for (i = 0U; i < in_len; ++i) {
		msg->io_size[i] = in_vec[i].len;
	}
	for (i = 0U; i < out_len; ++i) {
		msg->io_size[in_len + i] = out_vec[i].len;
	}

	for (i = 0U; i < in_len; ++i) {
		if (in_vec[i].len > sizeof(msg->trailer) - payload_size) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}
		memcpy(msg->trailer + payload_size,
		       psa_u32_to_ptr(in_vec[i].base),
		       in_vec[i].len);
		payload_size += in_vec[i].len;
	}

	/* Output the actual size of the message, to optimize sending */
	*msg_len = sizeof(*msg) - sizeof(msg->trailer) + payload_size;

	return PSA_SUCCESS;
}

psa_status_t rse_protocol_embed_deserialize_reply(struct psa_outvec *out_vec,
						  uint8_t out_len,
						  psa_status_t *return_val,
						  const struct rse_embed_reply_t *reply,
						  size_t reply_size)
{
	uint32_t payload_offset = 0;
	uint32_t i;

	assert(reply != NULL);
	assert(return_val != NULL);

	for (i = 0U; i < out_len; ++i) {
		if ((sizeof(*reply) - sizeof(reply->trailer) + payload_offset)
		    > reply_size) {
			return PSA_ERROR_INVALID_ARGUMENT;
		}

		memcpy(psa_u32_to_ptr(out_vec[i].base),
		       reply->trailer + payload_offset,
		       reply->out_size[i]);
		out_vec[i].len = reply->out_size[i];
		payload_offset += reply->out_size[i];
	}

	*return_val = reply->return_val;

	return PSA_SUCCESS;
}

psa_status_t rse_protocol_embed_calculate_msg_len(psa_handle_t handle,
						  const struct psa_invec *in_vec,
						  uint8_t in_len,
						  size_t *msg_len)
{
	uint32_t payload_size = 0;
	uint32_t i = 0;

	assert(in_vec != NULL);
	assert(msg_len != NULL);

	for (i = 0U; i < in_len; ++i)
		payload_size += in_vec[i].len;

	/* Output the actual size of the message, to optimize sending */
	*msg_len = offsetof(struct rse_embed_msg_t, trailer) + payload_size;

	return PSA_SUCCESS;
}
