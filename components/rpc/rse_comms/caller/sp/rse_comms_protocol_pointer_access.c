/*
 * Copyright (c) 2022-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <assert.h>
#include <string.h>

#include "platform/interface/device_region.h"
#include "rse_comms_caller.h"
#include "rse_comms_protocol.h"
#include "rse_comms_protocol_common.h"
#include "rse_comms_protocol_pointer_access.h"

/*
 * For the pointer access protocol, the pointers sent in the RSE message must be physical addresses.
 * But the base addresses in the invec and outvec are virtual.
 *
 * A carveout memory region is used for exchanging the invec and outvec data with the RSE:
 *  - The invec payload data is copied to the carveout region, the physical address of the buffer
 *    in the carveout region is sent to the RSE.
 *  - The space for outvec payload data is reserved in the carveout region. The physical address of
 *    the reserved buffer is sent to the RSE. When a reply is received from the RSE, data has been
 *    written to the buffer. Copy the data from the carveout region to the outvec buffer.
 *
 * The physical address and virtual address of the region are available:
 *  - The carveout region is reserved via device tree
 *  - The physical address and size of the region is configurable
 *  - The virtual address of the region is obtained from SP input parameters
 *
 * Invec/Outvec buffer handling for RSE message:
 *  - Calculate the offset of the payload buffer in the carveout region
 *  - buffer_physical_address = carveout_physical_address + offset
 *  - buffer_virtual_address = carveout_virtual_address + offset
 *  - For invec buffer:
 *     - Copy the invec data to buffer_virtual_address
 *  - For outvec buffer:
 *     - Reserve the space from buffer_virtual_address in the carveout region to receive RSE output
 *
 * Outvec buffer handling for RSE reply:
 *  - Calculate the offset of the outvec payload in the carveout region
 *  - buffer_virtual_address = carveout_virtual_address + offset
 *  - Copy the payload from buffer_virtual_address to the outvec buffer
 *
 * Here is an example layout of the carveout region for an RSE message that has 2 invec buffers
 * (0x100 and 0x200 in size) and 2 outvec buffers (0x300 and 0x400 in size).
 *
 * `#` area is invec data to RSE
 * `.` area is reserved for RSE output
 *
 *                      Virtual Address | Physical Address
 *  +---------------+ <--   0x4004C000   |   0xFFFFC000
 *  |###############|
 *  |## in_vec[0] ##|
 *  |###############|
 *  +---------------+ <--   0x4004C100   |   0xFFFFC100
 *  |###############|
 *  |## in_vec[1] ##|
 *  |###############|
 *  +---------------+ <--   0x4004C300   |   0xFFFFC300
 *  |...............|
 *  |.. out_vec[0] .|
 *  |...............|
 *  +---------------+ <--   0x4004C600   |   0xFFFFC600
 *  |...............|
 *  |.. out_vec[1] .|
 *  |...............|
 *  +---------------+ <--   0x4004CA00   |   0xFFFFCA00
 *
 */

psa_status_t rse_protocol_pointer_access_serialize_msg(struct rpc_caller_interface *caller,
						       psa_handle_t handle,
						       int16_t type,
						       const struct psa_invec *in_vec,
						       uint8_t in_len,
						       const struct psa_outvec *out_vec,
						       uint8_t out_len,
						       struct rse_pointer_access_msg_t *msg,
						       size_t *msg_len)
{
	struct rse_comms_caller_context *context =
		(struct rse_comms_caller_context *)caller->context;
	struct rse_comms_messenger *rse_comms = &context->rse_comms;
	struct device_region *carveout_region =
		(struct device_region *)rse_comms->protocol;
	uint64_t carveout_phys_addr;
	uint64_t carveout_virt_addr;
	uint64_t offset_in_carveout = 0;
	uint64_t virt_addr_in_carveout;
	unsigned int i;

	assert(msg != NULL);
	assert(msg_len != NULL);
	assert(in_vec != NULL);
	assert(carveout_region != NULL);

	carveout_phys_addr = carveout_region->phys_addr;
	carveout_virt_addr = carveout_region->base_addr;
	msg->ctrl_param = PARAM_PACK(type, in_len, out_len);
	msg->handle = handle;

	/* Check if all the data can fit in the carveout region*/
	for (i = 0U; i < in_len; ++i) {
		offset_in_carveout += in_vec[i].len;
	}
	for (i = 0U; i < out_len; ++i) {
		offset_in_carveout += out_vec[i].len;
	}
	if (offset_in_carveout > carveout_region->io_region_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	/* Fill msg iovec lengths */
	offset_in_carveout = 0;
	for (i = 0U; i < in_len; ++i) {
		msg->host_ptrs[i] = carveout_phys_addr + offset_in_carveout;
		msg->io_sizes[i] = in_vec[i].len;
		virt_addr_in_carveout = carveout_virt_addr + offset_in_carveout;
		memcpy((void *)virt_addr_in_carveout, psa_u32_to_ptr(in_vec[i].base),
		       in_vec[i].len);
		offset_in_carveout += in_vec[i].len;
	}
	for (i = 0U; i < out_len; ++i) {
		msg->host_ptrs[in_len + i] =
			carveout_phys_addr + offset_in_carveout;
		msg->io_sizes[in_len + i] = out_vec[i].len;
		offset_in_carveout += out_vec[i].len;
	}

	*msg_len = sizeof(*msg);

	return PSA_SUCCESS;
}

psa_status_t rse_protocol_pointer_access_deserialize_reply(struct rpc_caller_interface *caller,
						struct psa_outvec *out_vec,
						uint8_t out_len,
						psa_status_t *return_val,
						const struct rse_pointer_access_reply_t *reply,
						size_t reply_size)
{
	struct rse_comms_caller_context *context =
		(struct rse_comms_caller_context *)caller->context;
	struct rse_comms_messenger *rse_comms = &context->rse_comms;
	struct device_region *carveout_region =
		(struct device_region *)rse_comms->protocol;
	struct rse_comms_msg *rse_comms_msg =
		(struct rse_comms_msg *)rse_comms->msg;
	struct serialized_rse_comms_msg_t *serialized_rse_comms_msg =
		(struct serialized_rse_comms_msg_t *)rse_comms_msg->req_buf;
	struct rse_pointer_access_msg_t *pointer_access_msg =
		(struct rse_pointer_access_msg_t *)&serialized_rse_comms_msg->msg;
	uint8_t in_len = PARAM_UNPACK_IN_LEN(pointer_access_msg->ctrl_param);
	uint64_t carveout_phys_addr;
	uint64_t carveout_virt_addr;
	uint64_t offset_in_carveout = 0;
	uint64_t virt_addr_in_carveout;
	unsigned int i;

	assert(reply != NULL);
	assert(return_val != NULL);
	assert(carveout_region != NULL);

	carveout_phys_addr = carveout_region->phys_addr;
	carveout_virt_addr = carveout_region->base_addr;

	/* Check if the reply data is still in the carveout boundary */
	for (i = 0U; i < in_len; ++i) {
		offset_in_carveout += pointer_access_msg->io_sizes[i];
	}
	for (i = 0U; i < out_len; ++i) {
		offset_in_carveout += reply->out_sizes[i];
	}
	if (offset_in_carveout > carveout_region->io_region_size) {
		return PSA_ERROR_BUFFER_TOO_SMALL;
	}

	if (reply->return_val == PSA_SUCCESS) {
		for (i = 0U; i < out_len; ++i) {
			out_vec[i].len = reply->out_sizes[i];
			virt_addr_in_carveout =
				pointer_access_msg->host_ptrs[in_len + i] -
				carveout_phys_addr +
				carveout_virt_addr;
			memcpy(psa_u32_to_ptr(out_vec[i].base),
			       (void *)virt_addr_in_carveout,
			       out_vec[i].len);
		}
	}

	*return_val = reply->return_val;

	return PSA_SUCCESS;
}

psa_status_t rse_protocol_pointer_access_calculate_msg_len(psa_handle_t handle,
							   const struct psa_invec *in_vec,
							   uint8_t in_len,
							   size_t *msg_len)
{
	(void)handle;
	(void)in_vec;
	(void)in_len;

	assert(msg_len != NULL);

	*msg_len = sizeof(struct rse_pointer_access_msg_t);

	return PSA_SUCCESS;
}
