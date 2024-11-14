// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.
 */

#include <assert.h>            // for assert
#include <stddef.h>            // for size_t
#include <stdint.h>            // for uint32_t, uint16_t, uintptr_t, U
#include <string.h>            // for memcpy
#include "ffa_api.h"           // for FFA_OK, ffa_interrupt_handler, ffa_fea...
#include "ffa_api_defines.h"   // for FFA_PARAM_MBZ, FFA_OK, FFA_ERROR, FFA_...
#include "ffa_api_types.h"     // for ffa_result, ffa_direct_msg, ffa_uuid
#include "ffa_internal_api.h"  // for ffa_params, ffa_svc
#include "util.h"              // for GENMASK_32, SHIFT_U32, BIT

/*
 * Unpacks the error code from the FFA_ERROR message. It is a signed 32 bit
 * value in an unsigned 64 bit field so proper casting must be used to avoid
 * compiler dependent behavior.
 */
static inline ffa_result ffa_get_errorcode(struct ffa_params *result)
{
	uint32_t raw_value = result->a2;

	return *(ffa_result *)(&raw_value);
}

/*
 * Unpacks the content of the SVC result into an ffa_direct_msg structure.
 */
static inline void ffa_unpack_direct_msg(struct ffa_params *svc_result,
					 struct ffa_direct_msg *msg)
{
	msg->function_id = svc_result->a0;
	msg->source_id = (svc_result->a1 >> 16);
	msg->destination_id = svc_result->a1;

	if (FFA_IS_32_BIT_FUNC(msg->function_id)) {
		msg->args.args32[0] = svc_result->a3;
		msg->args.args32[1] = svc_result->a4;
		msg->args.args32[2] = svc_result->a5;
		msg->args.args32[3] = svc_result->a6;
		msg->args.args32[4] = svc_result->a7;
	} else {
		msg->args.args64[0] = svc_result->a3;
		msg->args.args64[1] = svc_result->a4;
		msg->args.args64[2] = svc_result->a5;
		msg->args.args64[3] = svc_result->a6;
		msg->args.args64[4] = svc_result->a7;
	}
}

/*
 * The end of the interrupt handler is indicated by an FFA_MSG_WAIT call.
 */
static inline void ffa_return_from_interrupt(struct ffa_params *result)
{
	ffa_svc(FFA_MSG_WAIT, FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		result);
}

static inline void ffa_uuid_to_abi_format(const struct ffa_uuid *uuid,
					  uint32_t *result)
{
	size_t i = 0;

	for (i = 0; i < 4; i++) {
		result[i] = uuid->uuid[4 * i];
		result[i] |= SHIFT_U32(uuid->uuid[4 * i + 1], 8);
		result[i] |= SHIFT_U32(uuid->uuid[4 * i + 2], 16);
		result[i] |= SHIFT_U32(uuid->uuid[4 * i + 3], 24);
	}
}

ffa_result ffa_version(uint32_t *version)
{
	struct ffa_params result = {0};
	uint32_t self_version = 0;

	self_version = (FFA_VERSION_MAJOR << FFA_VERSION_MAJOR_SHIFT) |
		       (FFA_VERSION_MINOR << FFA_VERSION_MINOR_SHIFT);

	ffa_svc(FFA_VERSION, self_version, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		&result);

	if (result.a0 & BIT(31)) {
		uint32_t raw_error = result.a0;

		*version = 0;

		return *(ffa_result *)(&raw_error);
	}

	*version = result.a0;
	return FFA_OK;
}

ffa_result ffa_features(uint32_t ffa_function_id,
			struct ffa_interface_properties *interface_properties)
{
	struct ffa_params result = {0};

	ffa_svc(FFA_FEATURES, ffa_function_id, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		&result);

	if (result.a0 == FFA_ERROR) {
		interface_properties->interface_properties[0] = 0;
		interface_properties->interface_properties[1] = 0;
		return ffa_get_errorcode(&result);
	}

	assert(result.a0 == FFA_SUCCESS_32);
	interface_properties->interface_properties[0] = result.a2;
	interface_properties->interface_properties[1] = result.a3;
	return FFA_OK;
}

ffa_result ffa_rx_release(void)
{
	struct ffa_params result = {0};

	ffa_svc(FFA_RX_RELEASE, FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		&result);

	if (result.a0 == FFA_ERROR)
		return ffa_get_errorcode(&result);

	assert(result.a0 == FFA_SUCCESS_32);
	return FFA_OK;
}

ffa_result ffa_rxtx_map(const void *tx_buffer, const void *rx_buffer,
			uint32_t page_count)
{
	struct ffa_params result = {0};

	assert(page_count <= FFA_RXTX_MAP_PAGE_COUNT_MAX);

	page_count = SHIFT_U32(page_count & FFA_RXTX_MAP_PAGE_COUNT_MASK,
			       FFA_RXTX_MAP_PAGE_COUNT_SHIFT);

	ffa_svc(FFA_RXTX_MAP_64, (uintptr_t)tx_buffer, (uintptr_t)rx_buffer,
		page_count, FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		FFA_PARAM_MBZ, &result);

	if (result.a0 == FFA_ERROR)
		return ffa_get_errorcode(&result);

	/*
	 * There are no 64-bit parameters returned with FFA_SUCCESS, the SPMC
	 * will use the default 32-bit version.
	 */
	assert(result.a0 == FFA_SUCCESS_32);
	return FFA_OK;
}

ffa_result ffa_rxtx_unmap(uint16_t id)
{
	struct ffa_params result = {0};

	ffa_svc(FFA_RXTX_UNMAP, SHIFT_U32(id, FFA_RXTX_UNMAP_ID_SHIFT),
		FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		FFA_PARAM_MBZ, FFA_PARAM_MBZ, &result);

	if (result.a0 == FFA_ERROR)
		return ffa_get_errorcode(&result);

	assert(result.a0 == FFA_SUCCESS_32);
	return FFA_OK;
}

#if CFG_FFA_VERSION == FFA_VERSION_1_0
ffa_result ffa_partition_info_get(const struct ffa_uuid *uuid, uint32_t *count)
{
	struct ffa_params result = {0};
	uint32_t abi_uuid[4] = {0};

	ffa_uuid_to_abi_format(uuid, abi_uuid);

	ffa_svc(FFA_PARTITION_INFO_GET, abi_uuid[0], abi_uuid[1], abi_uuid[2],
		abi_uuid[3], FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		&result);

	if (result.a0 == FFA_ERROR) {
		*count = UINT32_C(0);
		return ffa_get_errorcode(&result);
	}

	assert(result.a0 == FFA_SUCCESS_32);
	*count = result.a2;
	return FFA_OK;
}
#elif CFG_FFA_VERSION >= FFA_VERSION_1_1
ffa_result ffa_partition_info_get(const struct ffa_uuid *uuid, uint32_t flags, uint32_t *count,
				  uint32_t *size)
{
	struct ffa_params result = {0};
	uint32_t abi_uuid[4] = {0};

	ffa_uuid_to_abi_format(uuid, abi_uuid);

	ffa_svc(FFA_PARTITION_INFO_GET, abi_uuid[0], abi_uuid[1], abi_uuid[2],
		abi_uuid[3], flags, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		&result);

	if (result.a0 == FFA_ERROR) {
		*count = UINT32_C(0);
		*size = UINT32_C(0);
		return ffa_get_errorcode(&result);
	}

	assert(result.a0 == FFA_SUCCESS_32);
	*count = result.a2;
	*size = result.a3;
	return FFA_OK;
}
#endif /* CFG_FFA_VERSION */

ffa_result ffa_id_get(uint16_t *id)
{
	struct ffa_params result = {0};

	ffa_svc(FFA_ID_GET, FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		&result);

	if (result.a0 == FFA_ERROR) {
		*id = FFA_ID_GET_ID_MASK;
		return ffa_get_errorcode(&result);
	}

	assert(result.a0 == FFA_SUCCESS_32);
	*id = (result.a2 >> FFA_ID_GET_ID_SHIFT) & FFA_ID_GET_ID_MASK;
	return FFA_OK;
}

static void handle_framework_msg(struct ffa_params *result)
{
	if (result->a0 == FFA_INTERRUPT) {
		ffa_interrupt_handler(result->a2);
		ffa_return_from_interrupt(result);
	} else if (result->a0 == FFA_MSG_SEND_DIRECT_REQ_32 && FFA_IS_FRAMEWORK_MSG(result->a2)) {
		ffa_result res = FFA_OK;
		uint16_t src_id = result->a1 >> 16;
		uint16_t dst_id = result->a1;
		uint64_t handle = reg_pair_to_64(result->a4, result->a3);
		uint16_t vm_id = result->a5;

		switch (result->a2 & FFA_FRAMEWORK_MSG_TYPE_MASK) {
		case FFA_FRAMEWORK_MSG_VM_CREATED:
			res = ffa_vm_created_handler(vm_id, handle);
			ffa_svc(FFA_MSG_SEND_DIRECT_RESP_32, ((uint32_t)dst_id << 16) | src_id,
				FFA_MSG_FLAG_FRAMEWORK | FFA_FRAMEWORK_MSG_VM_CREATED_ACK,
				(uint64_t)res, FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
				FFA_PARAM_MBZ, result);
			break;
		case FFA_FRAMEWORK_MSG_VM_DESTROYED:
			res = ffa_vm_destroyed_handler(vm_id, handle);
			ffa_svc(FFA_MSG_SEND_DIRECT_RESP_32, ((uint32_t)dst_id << 16) | src_id,
				FFA_MSG_FLAG_FRAMEWORK | FFA_FRAMEWORK_MSG_VM_DESTROYED_ACK,
				(uint64_t)res, FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
				FFA_PARAM_MBZ, result);
			break;
		default:
			ffa_svc(FFA_ERROR, FFA_PARAM_MBZ, FFA_INVALID_PARAMETERS, FFA_PARAM_MBZ,
				FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ, result);
			break;
		}
	}
}

ffa_result ffa_msg_wait(struct ffa_direct_msg *msg)
{
	struct ffa_params result = {0};

	ffa_svc(FFA_MSG_WAIT, FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		&result);

	/* FF-A framework messages are handled outside of the main partition message handler loop */
	while ((result.a0 == FFA_INTERRUPT) ||
		(result.a0 == FFA_MSG_SEND_DIRECT_REQ_32 && FFA_IS_FRAMEWORK_MSG(result.a2))) {
		handle_framework_msg(&result);
	}

	if (result.a0 == FFA_ERROR) {
		return ffa_get_errorcode(&result);
	} else if (FFA_TO_32_BIT_FUNC(result.a0) == FFA_MSG_SEND_DIRECT_REQ_32) {
		ffa_unpack_direct_msg(&result, msg);
	} else {
		assert(result.a0 == FFA_SUCCESS_32);
		*msg = (struct ffa_direct_msg){.function_id = result.a0};
	}

	return FFA_OK;
}

static ffa_result ffa_msg_send_direct_req(uint32_t function_id, uint32_t resp_id,
					  uint16_t source, uint16_t dest,
					  uint64_t a0, uint64_t a1, uint64_t a2,
					  uint64_t a3, uint64_t a4,
					  struct ffa_direct_msg *msg)
{
	struct ffa_params result = {0};

	ffa_svc(function_id,
		SHIFT_U32(source, FFA_MSG_SEND_DIRECT_REQ_SOURCE_ID_SHIFT) |
		dest, FFA_PARAM_MBZ, a0, a1, a2, a3, a4, &result);

	while (result.a0 == FFA_INTERRUPT) {
		ffa_interrupt_handler(result.a2);
		ffa_return_from_interrupt(&result);
	}

	if (result.a0 == FFA_ERROR) {
		return ffa_get_errorcode(&result);
	} else if (result.a0 == resp_id) {
		ffa_unpack_direct_msg(&result, msg);
	} else {
		assert(result.a0 == FFA_SUCCESS_32);
		*msg = (struct ffa_direct_msg){.function_id = result.a0};
	}

	return FFA_OK;
}

ffa_result ffa_msg_send_direct_req_32(uint16_t source, uint16_t dest,
				      uint32_t a0, uint32_t a1, uint32_t a2,
				      uint32_t a3, uint32_t a4,
				      struct ffa_direct_msg *msg)
{
	return ffa_msg_send_direct_req(FFA_MSG_SEND_DIRECT_REQ_32,
				       FFA_MSG_SEND_DIRECT_RESP_32,
				       source, dest, a0, a1, a2, a3, a4, msg);
}

ffa_result ffa_msg_send_direct_req_64(uint16_t source, uint16_t dest,
				      uint64_t a0, uint64_t a1, uint64_t a2,
				      uint64_t a3, uint64_t a4,
				      struct ffa_direct_msg *msg)
{
	return ffa_msg_send_direct_req(FFA_MSG_SEND_DIRECT_REQ_64,
				       FFA_MSG_SEND_DIRECT_RESP_64,
				       source, dest, a0, a1, a2, a3, a4, msg);
}

static ffa_result ffa_msg_send_direct_resp(uint32_t function_id,
					   uint16_t source, uint16_t dest,
					   uint64_t a0, uint64_t a1,
					   uint64_t a2, uint64_t a3,
					   uint64_t a4,
					   struct ffa_direct_msg *msg)
{
	struct ffa_params result = {0};

	ffa_svc(function_id,
		SHIFT_U32(source, FFA_MSG_SEND_DIRECT_RESP_SOURCE_ID_SHIFT) |
		dest, FFA_PARAM_MBZ, a0, a1, a2, a3, a4, &result);

	/* FF-A framework messages are handled outside of the main partition message handler loop */
	while ((result.a0 == FFA_INTERRUPT) ||
		(result.a0 == FFA_MSG_SEND_DIRECT_REQ_32 && FFA_IS_FRAMEWORK_MSG(result.a2))) {
		handle_framework_msg(&result);
	}

	if (result.a0 == FFA_ERROR) {
		return ffa_get_errorcode(&result);
	} else if (FFA_TO_32_BIT_FUNC(result.a0) == FFA_MSG_SEND_DIRECT_REQ_32) {
		ffa_unpack_direct_msg(&result, msg);
	} else {
		assert(result.a0 == FFA_SUCCESS_32);
		*msg = (struct ffa_direct_msg){.function_id = result.a0};
	}

	return FFA_OK;
}

ffa_result ffa_msg_send_direct_resp_32(uint16_t source, uint16_t dest,
				       uint32_t a0, uint32_t a1, uint32_t a2,
				       uint32_t a3, uint32_t a4,
				       struct ffa_direct_msg *msg)
{
	return ffa_msg_send_direct_resp(FFA_MSG_SEND_DIRECT_RESP_32, source,
					dest, a0, a1, a2, a3, a4, msg);
}

ffa_result ffa_msg_send_direct_resp_64(uint16_t source, uint16_t dest,
				      uint64_t a0, uint64_t a1, uint64_t a2,
				      uint64_t a3, uint64_t a4,
				      struct ffa_direct_msg *msg)
{
	return ffa_msg_send_direct_resp(FFA_MSG_SEND_DIRECT_RESP_64, source,
					dest, a0, a1, a2, a3, a4, msg);
}

ffa_result ffa_mem_donate(uint32_t total_length, uint32_t fragment_length,
			  void *buffer_address, uint32_t page_count,
			  uint64_t *handle)
{
	struct ffa_params result = {0};

	ffa_svc((buffer_address) ? FFA_MEM_DONATE_64 : FFA_MEM_DONATE_32,
		total_length, fragment_length, (uintptr_t)buffer_address,
		page_count, FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		&result);

	if (result.a0 == FFA_ERROR) {
		*handle = 0U;
		return ffa_get_errorcode(&result);
	}

	/*
	 * There are no 64-bit parameters returned with FFA_SUCCESS, the SPMC
	 * will use the default 32-bit version.
	 */
	assert(result.a0 == FFA_SUCCESS_32);
	*handle = reg_pair_to_64(result.a3, result.a2);
	return FFA_OK;
}

ffa_result ffa_mem_donate_rxtx(uint32_t total_length, uint32_t fragment_length,
			       uint64_t *handle)
{
	return ffa_mem_donate(total_length, fragment_length, NULL, 0, handle);
}

ffa_result ffa_mem_lend(uint32_t total_length, uint32_t fragment_length,
			void *buffer_address, uint32_t page_count,
			uint64_t *handle)
{
	struct ffa_params result = {0};

	ffa_svc((buffer_address) ? FFA_MEM_LEND_64 : FFA_MEM_LEND_32,
		total_length, fragment_length, (uintptr_t)buffer_address,
		page_count, FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		&result);

	if (result.a0 == FFA_ERROR) {
		*handle = 0U;
		return ffa_get_errorcode(&result);
	}

	/*
	 * There are no 64-bit parameters returned with FFA_SUCCESS, the SPMC
	 * will use the default 32-bit version.
	 */
	assert(result.a0 == FFA_SUCCESS_32);
	*handle = reg_pair_to_64(result.a3, result.a2);
	return FFA_OK;
}

ffa_result ffa_mem_lend_rxtx(uint32_t total_length, uint32_t fragment_length,
			     uint64_t *handle)
{
	return ffa_mem_lend(total_length, fragment_length, NULL, 0, handle);
}

ffa_result ffa_mem_share(uint32_t total_length, uint32_t fragment_length,
			 void *buffer_address, uint32_t page_count,
			 uint64_t *handle)
{
	struct ffa_params result = {0};

	ffa_svc((buffer_address) ? FFA_MEM_SHARE_64 : FFA_MEM_SHARE_32,
		total_length, fragment_length, (uintptr_t)buffer_address,
		page_count, FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		&result);

	if (result.a0 == FFA_ERROR) {
		*handle = 0U;
		return ffa_get_errorcode(&result);
	}

	/*
	 * There are no 64-bit parameters returned with FFA_SUCCESS, the SPMC
	 * will use the default 32-bit version.
	 */
	assert(result.a0 == FFA_SUCCESS_32);
	*handle = reg_pair_to_64(result.a3, result.a2);
	return FFA_OK;
}

ffa_result ffa_mem_share_rxtx(uint32_t total_length, uint32_t fragment_length,
			      uint64_t *handle)
{
	return ffa_mem_share(total_length, fragment_length, NULL, 0, handle);
}

ffa_result ffa_mem_retrieve_req(uint32_t total_length, uint32_t fragment_length,
				void *buffer_address, uint32_t page_count,
				uint32_t *resp_total_length,
				uint32_t *resp_fragment_length)
{
	struct ffa_params result = {0};

	ffa_svc((buffer_address) ? FFA_MEM_RETRIEVE_REQ_64 : FFA_MEM_RETRIEVE_REQ_32,
		total_length, fragment_length, (uintptr_t)buffer_address,
		page_count, FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		&result);

	if (result.a0 == FFA_ERROR) {
		*resp_total_length = 0U;
		*resp_fragment_length = 0U;
		return ffa_get_errorcode(&result);
	}

	assert(result.a0 == FFA_MEM_RETRIEVE_RESP);
	*resp_total_length = result.a1;
	*resp_fragment_length = result.a2;
	return FFA_OK;
}

ffa_result ffa_mem_retrieve_req_rxtx(uint32_t total_length,
				     uint32_t fragment_length,
				     uint32_t *resp_total_length,
				     uint32_t *resp_fragment_length)
{
	return ffa_mem_retrieve_req(total_length, fragment_length, NULL, 0,
				    resp_total_length, resp_fragment_length);
}

ffa_result ffa_mem_relinquish(void)
{
	struct ffa_params result = {0};

	ffa_svc(FFA_MEM_RELINQUISH, FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		&result);

	if (result.a0 == FFA_ERROR)
		return ffa_get_errorcode(&result);

	assert(result.a0 == FFA_SUCCESS_32);
	return FFA_OK;
}

ffa_result ffa_mem_reclaim(uint64_t handle, uint32_t flags)
{
	struct ffa_params result = {0};
	uint32_t handle_hi = 0;
	uint32_t handle_lo = 0;

	reg_pair_from_64(handle, &handle_hi, &handle_lo);

	ffa_svc(FFA_MEM_RECLAIM, handle_lo, handle_hi, flags, FFA_PARAM_MBZ,
		FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ, &result);

	if (result.a0 == FFA_ERROR)
		return ffa_get_errorcode(&result);

	assert(result.a0 == FFA_SUCCESS_32);
	return FFA_OK;
}

ffa_result ffa_mem_perm_get(const void *base_address, uint32_t *mem_perm)
{
	struct ffa_params result = {0};

	ffa_svc(FFA_MEM_PERM_GET, (uintptr_t)base_address, FFA_PARAM_MBZ,
		FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		FFA_PARAM_MBZ, &result);

	if (result.a0 == FFA_ERROR)
		return ffa_get_errorcode(&result);

	assert(result.a0 == FFA_SUCCESS_32);
	*mem_perm = result.a2;
	return FFA_OK;
}

ffa_result ffa_mem_perm_set(const void *base_address, uint32_t page_count,
			    uint32_t mem_perm)
{
	struct ffa_params result = {0};

	assert((mem_perm & FFA_MEM_PERM_RESERVED_MASK) == 0);

	ffa_svc(FFA_MEM_PERM_SET, (uintptr_t)base_address, page_count, mem_perm,
		FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ, FFA_PARAM_MBZ,
		&result);

	if (result.a0 == FFA_ERROR)
		return ffa_get_errorcode(&result);

	assert(result.a0 == FFA_SUCCESS_32);
	return FFA_OK;
}

ffa_result ffa_console_log_32(const char *message, size_t length)
{
	struct ffa_params result = {0};
	uint32_t char_lists[6] = {0};

	assert(length > 0 && length <= sizeof(char_lists));

	memcpy(char_lists, message, MIN(length, sizeof(char_lists)));

	ffa_svc(FFA_CONSOLE_LOG_32, length, char_lists[0], char_lists[1],
		char_lists[2], char_lists[3], char_lists[4], char_lists[5],
		&result);

	if (result.a0 == FFA_ERROR)
		return ffa_get_errorcode(&result);

	assert(result.a0 == FFA_SUCCESS_32);
	return FFA_OK;
}

ffa_result ffa_console_log_64(const char *message, size_t length)
{
	struct ffa_params result = {0};
	uint64_t char_lists[6] = {0};

	assert(length > 0 && length <= sizeof(char_lists));

	memcpy(char_lists, message, MIN(length, sizeof(char_lists)));

	ffa_svc(FFA_CONSOLE_LOG_64, length, char_lists[0], char_lists[1],
		char_lists[2], char_lists[3], char_lists[4], char_lists[5],
		&result);

	if (result.a0 == FFA_ERROR)
		return ffa_get_errorcode(&result);

	assert(result.a0 == FFA_SUCCESS_32);
	return FFA_OK;
}
