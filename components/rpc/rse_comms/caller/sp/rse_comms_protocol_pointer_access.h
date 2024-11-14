/*
 * Copyright (c) 2022-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __RSE_COMMS_PROTOCOL_POINTER_ACCESS_H__
#define __RSE_COMMS_PROTOCOL_POINTER_ACCESS_H__


#include <psa/client.h>
#include <sys/cdefs.h>

struct __packed rse_pointer_access_msg_t {
	psa_handle_t handle;
	uint32_t ctrl_param;
	uint32_t io_sizes[PSA_MAX_IOVEC];
	uint64_t host_ptrs[PSA_MAX_IOVEC];
};

struct __packed rse_pointer_access_reply_t {
	int32_t return_val;
	uint32_t out_sizes[PSA_MAX_IOVEC];
};

psa_status_t rse_protocol_pointer_access_serialize_msg(psa_handle_t handle,
						       int16_t type,
						       const struct psa_invec *in_vec,
						       uint8_t in_len,
						       const struct psa_outvec *out_vec,
						       uint8_t out_len,
						       struct rse_pointer_access_msg_t *msg,
						       size_t *msg_len);

psa_status_t rse_protocol_pointer_access_deserialize_reply(struct psa_outvec *out_vec,
							   uint8_t out_len,
							   psa_status_t *return_val,
							   const struct rse_pointer_access_reply_t *reply,
							   size_t reply_size);

#endif /* __RSE_COMMS_PROTOCOL_POINTER_ACCESS_H__ */
