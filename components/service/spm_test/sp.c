/*
 * Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <ffa_api.h>
#include <ffa_internal_api.h>
#include <ffa_memory_descriptors.h>
#include <sp_api.h>
#include <sp_discovery.h>
#include <sp_memory_management.h>
#include <sp_rxtx.h>
#include <string.h>
#include <trace.h>

#include "config/interface/config_store.h"
#include "config/loader/sp/sp_config_loader.h"
#include "config/ramstore/config_ramstore.h"
#include "platform/interface/memory_region.h"
#define SP_TEST_OK 0xaa

static volatile uint8_t tx_buffer[4096] __aligned(4096);
static volatile uint8_t rx_buffer[4096] __aligned(4096);
static volatile uint8_t my_buf[4096] __aligned(4096);
static volatile uint8_t *shared_buffer;
static size_t shared_buffer_size;



enum errors {
	ERR_OK,
	ERR_VERSION,
	ERR_ID_GET,
	ERR_FEATURES,
	ERR_SP_COMMUNICATION,
	ERR_RXTX_MAP,
	ERR_PARTITION,
	ERR_RXTX_UNMAP,
	ERR_MEM_INCORRECT_ACCESS,
	ERR_MEM_RETRIEVE,
	ERR_MEM_RELINQUISH,
	ERR_SP_SHARE,
	ERR_SP_SHARE_EXC,
	ERR_TEST_NOT_FOUND
};

enum sp_tests {
	EP_TEST_SP,
	EP_TEST_SP_COMMUNICATION,
	EP_TEST_SP_INCREASE,
	EP_TRY_R_ACCESS,
	EP_TRY_W_ACCESS,
	EP_RETRIEVE,
	EP_RELINQUISH,
	EP_SP_MEM_SHARING,
	EP_SP_MEM_SHARING_MULTI,
	EP_SP_MEM_SHARING_EXC,
	EP_SP_MEM_INCORRECT_ACCESS,
	EP_SP_NOP,
	EP_TEST_SP_COMMUNICATION_RESPONSE,
};

const char* sp_test_str[]= {
	"EP_TEST_SP",
	"EP_TEST_SP_COMMUNICATION",
	"EP_TEST_SP_INCREASE",
	"EP_TRY_R_ACCESS",
	"EP_TRY_W_ACCESS",
	"EP_RETRIEVE",
	"EP_RELINQUISH",
	"EP_SP_MEM_SHARING",
	"EP_SP_MEM_SHARING_MULTI",
	"EP_SP_MEM_SHARING_EXC",
	"EP_SP_MEM_INCORRECT_ACCESS",
	"EP_SP_NOP"
};

static bool test_ffa_version(void)
{
	sp_result result = SP_RESULT_OK;
	uint16_t major = 0;
	uint16_t minor = 0;

	IMSG("Testing ffa_version()\n");

	result = sp_discovery_ffa_version_get(&major, &minor);
	if (result == SP_RESULT_OK) {
		IMSG("ffa_version(): %"PRIu32".%"PRIu32"\n", major, minor);

		return true;
	} else if (result == FFA_NOT_SUPPORTED) {
		IMSG("ffa_version(): not supported\n");
	} else {
		EMSG("ffa_version(): unknown error %"PRId32"\n", result);
	}

	return false;
}

static bool test_ffa_id_get(uint16_t *id)
{
	sp_result result = SP_RESULT_OK;

	IMSG("Testing ffa_id_get()\n");

	result = sp_discovery_own_id_get(id);
	if (result == SP_RESULT_OK) {
		IMSG("ffa_id_get(): 0x%"PRIx16"\n", *id);

		return true;
	} else if (result == FFA_NOT_SUPPORTED) {
		IMSG("ffa_id_get(): not supported\n");
	} else {
		EMSG("ffa_id_get(): unknown error %"PRId32"\n", result);
	}

	return false;
}

static bool test_ffa_features(void)
{
	ffa_result result = FFA_OK;
	struct ffa_interface_properties properties = {0};

	IMSG("Testing ffa_features(FFA_RXTX_MAP)\n");

	result = ffa_features(FFA_RXTX_MAP_32, &properties);
	if (result == FFA_OK) {
		static const char * const sizes[] = {
			"4kB", "64kB", "16kB", "reserved"};
		uint32_t buffer_size = properties.interface_properties[0] &
				       0x3U;

		IMSG("ffa_features(): minimum buffer size=%s\n",
		     sizes[buffer_size]);
		return true;
	} else if (result == FFA_NOT_SUPPORTED) {
		IMSG("ffa_features(): not supported\n");
	} else {
		EMSG("ffa_features(): unknown error %"PRId32"\n", result);
	}
	return false;
}

static bool test_ffa_rxtx_map(void)
{
	sp_result result = SP_RESULT_OK;

	IMSG("Testing ffa_rxtx_map(%p %p, 1)\n", tx_buffer, rx_buffer);

	result = sp_rxtx_buffer_map((void*)tx_buffer,(void*)rx_buffer,
				    sizeof(rx_buffer));
	if (result == FFA_OK) {
		IMSG("ffa_rxtx_map(): success\n");
		return true;
	} else if (result == FFA_NOT_SUPPORTED) {
		IMSG("ffa_rxtx_map(): not supported\n");
	} else {
		EMSG("ffa_rxtx_map(): unknown error %"PRId32"\n", result);
	}

	return false;
}

static bool ffa_partition_info_get_process(sp_result result, uint32_t count,
					struct sp_partition_info *partitions)
{
	uint32_t i = 0;

	if (result != SP_RESULT_OK) {
		if (result == FFA_NOT_SUPPORTED) {
			IMSG("ffa_partition_info_get(): not supported\n");
			return false;
		}
		EMSG("ffa_partition_info_get(): unknown error %"PRId32"\n", result);
		return false;
	}
	IMSG("ffa_partition_info_get(): count=%"PRIu32"\n", count);

	for (i = 0; i < count; i++) {
		IMSG("partition #%u: ID=%u, execution_count=%u  \
		      direct request = %c, send direcy request = %c, \
		      indirect request = %c\n",
		      i, partitions[i].partition_id,
		      partitions[i].execution_context_count,
		      partitions[i].supports_direct_requests ? '1' : '0',
		      partitions[i].can_send_direct_requests ? '1' : '0',
		      partitions[i].supports_indirect_requests ? '1' : '0'
		      );
	}

	IMSG("Testing ffa_rx_release()\n");

	result = ffa_rx_release();
	if (result == FFA_OK) {
		IMSG("ffa_rx_release(): success\n");
		return true;
	} else if (result == FFA_NOT_SUPPORTED) {
		IMSG("ffa_rx_release(): not supported\n");
		return false;
	}

	EMSG("ffa_rx_release(): unknown error %"PRId32"\n", result);
	return false;
}

static bool test_ffa_partition_info_get(void)
{
	sp_result result = SP_RESULT_OK;
	struct sp_partition_info partitions[10] = {0};
	uint32_t count = 10;
	struct sp_uuid uuid = {.uuid = {0x23, 0xeb, 0x01, 0x00, 0xe3, 0x2a,
					0x44, 0x97, 0x90, 0x52, 0x2f, 0x11,
					0xe5, 0x84, 0xaf, 0xa6}};

	IMSG("Testing ffa_partition_info_get(nil)\n");

	result = sp_discovery_partition_info_get_all(partitions, &count);
	if (!ffa_partition_info_get_process(result, count, partitions))
		return false;
	result = sp_discovery_partition_info_get(&uuid,
		   partitions,
		   &count);

	if (!ffa_partition_info_get_process(result, count, partitions))
		return false;
	if (count < 2) {
		EMSG("ffa_partition_info_get(): Returned not enough SPs count=%"PRIu32"\n", count);
		return false;
	}
	return true;
}

static bool test_ffa_rxtx_unmap()
{
	sp_result result  = SP_RESULT_OK;

	result = sp_rxtx_buffer_unmap();
	if (result == SP_RESULT_OK) {
		IMSG("sp_rxtx_buffer_unmap(): success\n");
		return true;
	}
	EMSG("sp_rxtx_buffer_unmap(): unknown error %"PRId32"\n", result);
	return false;
}

static void return_error(uint32_t error, struct ffa_direct_msg *msg)
{
	ffa_msg_send_direct_resp_64(msg->destination_id, msg->source_id, 0xff,
				 error, 0, 0, 0, msg);
}

static void return_ok(struct ffa_direct_msg *msg)
{

	ffa_msg_send_direct_resp_64(msg->destination_id,
				 msg->source_id, SP_TEST_OK, 0, 0, 0, 0, msg);
}

static bool test_read_access(void)
{
	return (shared_buffer[0] != 5);

}

static void test_write_access(void)
{
	shared_buffer[0] = 0xff;
}

static void test_increase(struct ffa_direct_msg *msg)
{
	msg->args.args64[1]++;
	msg->args.args64[2]++;
	msg->args.args64[3]++;
	msg->args.args64[4]++;
	ffa_msg_send_direct_resp_64(msg->destination_id,msg->source_id,
				 SP_TEST_OK, msg->args.args64[1],
				 msg->args.args64[2],msg->args.args64[3],
				 msg->args.args64[4], msg);

}

static void test_communication(struct ffa_direct_msg *msg)
{
	struct ffa_direct_msg sp_msg = {0};
	uint16_t caller = msg->source_id;
	uint16_t src = msg->destination_id;
	uint16_t dst = (uint16_t)msg->args.args64[1];
	ffa_result res = FFA_OK;
	struct ffa_params raw_params = { 0 };

	sp_msg.args.args64[1] =  0x55;
	sp_msg.args.args64[2] =  0xAA;
	sp_msg.args.args64[3] =  0xBB;
	sp_msg.args.args64[4] =  0xCC;

	res = ffa_msg_send_direct_req_64(src, dst,
				      EP_TEST_SP_INCREASE,0x55, 0xAA, 0xBB,
				      0xCC, &sp_msg);
	if (res != FFA_OK) {
		EMSG("error % in %s:%d"PRId32, res, __FILE__, __LINE__);
		goto err;
	}

	if (sp_msg.args.args64[1] != 0x56 || sp_msg.args.args64[2] != 0xAB ||
	    sp_msg.args.args64[3] != 0xBC || sp_msg.args.args64[4] != 0xCD) {
		DMSG("Failed SP communication %lx %lx %lx %lx",
		     sp_msg.args.args64[1], sp_msg.args.args64[2],
		     sp_msg.args.args64[3], sp_msg.args.args64[4]);
		goto err;
	}

	/* Non-null flags (W2) register */
	ffa_svc(FFA_MSG_SEND_DIRECT_REQ_64, (uint32_t)(src << 16 | dst), 1, 0, 0, 0, 0, 0,
		&raw_params);
	if (raw_params.a0 != FFA_ERROR || (uint32_t)raw_params.a2 != FFA_INVALID_PARAMETERS) {
		EMSG("Unexpected error code: %d != %ld", FFA_INVALID_PARAMETERS, raw_params.a2);
		goto err;
	}

	/* Testing non-matching source ID */
	res = ffa_msg_send_direct_req_64(src + 1, dst, 0, 0, 0, 0, 0, &sp_msg);
	if (res != FFA_INVALID_PARAMETERS) {
		EMSG("Unexpected error code: %d != %d", FFA_INVALID_PARAMETERS, res);
		goto err;
	}

	/* Sending message to own ID */
	res = ffa_msg_send_direct_req_64(src, src, 0, 0, 0, 0, 0, &sp_msg);
	if (res != FFA_INVALID_PARAMETERS) {
		EMSG("Unexpected error code: %d != %d", FFA_INVALID_PARAMETERS, res);
		goto err;
	}

	/* Sending message to normal world */
	res = ffa_msg_send_direct_req_64(src, 0, 0, 0, 0, 0, 0, &sp_msg);
	if (res != FFA_NOT_SUPPORTED) {
		EMSG("Unexpected error code: %d != %d", FFA_NOT_SUPPORTED, res);
		goto err;
	}

	/* Sending message for starting direct message response test */
	if (!caller) {
		res = ffa_msg_send_direct_req_64(src, dst, EP_TEST_SP_COMMUNICATION_RESPONSE, 0, 0,
						 0, 0, &sp_msg);
		if (res != FFA_OK) {
			EMSG("Unexpected error code: %d != %d", FFA_OK, res);
			goto err;
		}

		if (sp_msg.args.args64[0] != SP_TEST_OK) {
			EMSG("Unexpected test result: %d != %ld", SP_TEST_OK, sp_msg.args.args64[0]);
			goto err;
		}
	}

	return_ok(msg);
	return;

err:
	return_error(ERR_SP_COMMUNICATION, msg);
}

static void test_communication_response(struct ffa_direct_msg *msg)
{
	struct ffa_direct_msg sp_msg = {0};
	uint16_t caller = msg->source_id;
	uint16_t src = msg->destination_id;
	ffa_result res = FFA_OK;
	struct ffa_params raw_params = { 0 };

	/* Non-null flags (W2) register */
	ffa_svc(FFA_MSG_SEND_DIRECT_RESP_64, (uint32_t)(src << 16 | 0x1000), 1, 0, 0, 0, 0, 1,
		&raw_params);
	if (raw_params.a0 != FFA_ERROR || (uint32_t)raw_params.a2 != FFA_INVALID_PARAMETERS) {
		EMSG("Unexpected error code: %d != %ld", FFA_INVALID_PARAMETERS, raw_params.a2);
		goto err;
	}

	/* Testing non-matching source ID */
	res = ffa_msg_send_direct_resp_64(src + 1, caller, 0, 0, 0, 0, 2, &sp_msg);
	if (res != FFA_INVALID_PARAMETERS) {
		EMSG("Unexpected error code: %d != %d", FFA_INVALID_PARAMETERS, res);
		goto err;
	}

	/* Sending message to own ID */
	res = ffa_msg_send_direct_resp_64(src, src, 0, 0, 0, 0, 3, &sp_msg);
	if (res != FFA_INVALID_PARAMETERS) {
		EMSG("Unexpected error code: %d != %d", FFA_INVALID_PARAMETERS, res);
		goto err;
	}

	/* Sending message request to caller SP which is busy */
	if (caller) {
		/* Sending message to normal world */
		res = ffa_msg_send_direct_resp_64(src, 0, 0, 0, 0, 0, 4, &sp_msg);
		if (res != FFA_INVALID_PARAMETERS) {
			EMSG("Unexpected error code: %d != %d", FFA_INVALID_PARAMETERS, res);
			goto err;
		}

		/* Sending message to invalid SP */
		res = ffa_msg_send_direct_resp_64(src, 0x1000, 0, 0, 0, 0, 5, &sp_msg);
		if (res != FFA_INVALID_PARAMETERS) {
			EMSG("Unexpected error code: %d != %d", FFA_INVALID_PARAMETERS, res);
			goto err;
		}

		/* Sending message request to caller SP which is busy */
		res = ffa_msg_send_direct_req_64(src, caller, 0, 0, 0, 0, 6, &sp_msg);
		if (res != FFA_BUSY) {
			EMSG("Unexpected error code: %d != %d", FFA_BUSY, res);
			goto err;
		}
	}

	ffa_msg_send_direct_resp_64(src, caller, SP_TEST_OK, 0, 0, 0, 0, msg);
	return;

err:
	ffa_msg_send_direct_resp_64(src, caller, ERR_SP_COMMUNICATION, 0, 0, 0, 0, msg);

}

static void test_internal_sp(struct ffa_direct_msg *msg)
{
	enum errors err = ERR_OK;
	uint16_t id = 0;

	if (test_ffa_version()) {
		if (!test_ffa_id_get(&id))
			err = ERR_ID_GET;

		if (!err && !test_ffa_features())
			err = ERR_VERSION;

		if (!err && !test_ffa_rxtx_unmap(id))
			err = ERR_RXTX_UNMAP;

		if (!err && !test_ffa_rxtx_map())
			err = ERR_RXTX_MAP;

		if (!err && !test_ffa_partition_info_get())
			err = ERR_PARTITION;

	} else {
		err = ERR_VERSION;
	}

	if (err != ERR_OK) {
		DMSG("Failed at SP test %x", err);
		return_error((uint32_t)err, msg);
	}

	return_ok(msg);
}

static void set_rxtx_buf(struct ffa_mem_transaction_buffer *t_buf,
			 struct ffa_mem_transaction_buffer *r_buf)
{
	if (t_buf) {
		t_buf->buffer = (void*)tx_buffer;
		t_buf->length = 4096;
		t_buf->used = false;
	}
	if (r_buf) {
		r_buf->buffer = (void*)rx_buffer;
		r_buf->length = 4096;
		r_buf->used = false;
	}
}

static void test_mem_retrieve(struct ffa_direct_msg *msg)
{
	ffa_result res = FFA_OK;
	struct sp_memory_descriptor descriptor = {0};
	struct sp_memory_region regions[1] = {0};
	struct sp_memory_access_descriptor acc_desc = {0};
	uint64_t handle = 0;
	uint32_t out_region_count = 1;
	uint16_t own_id = 0;

	ffa_id_get(&own_id);

	handle = (uint64_t)msg->args.args64[1] |
		 (((uint64_t)msg->args.args64[2]) << 32);
	descriptor.tag = 0;
	descriptor.sender_id = msg->args.args64[3] & 0xffff;
	acc_desc.receiver_id = own_id;
	acc_desc.data_access = sp_data_access_read_write;
	res = sp_memory_retrieve(&descriptor, &acc_desc, regions, 1,
				 &out_region_count, handle);

	if (res) {
		DMSG("Failed retrieving shared memory");
		return_error((uint32_t)ERR_MEM_RETRIEVE, msg);
		return;
	}

	if (descriptor.flags.transaction_type != sp_memory_transaction_type_share) {
		EMSG("Invalid transaction type");
		return_error((uint32_t)ERR_MEM_RETRIEVE, msg);
		return;
	}

	shared_buffer = regions[0].address;
	shared_buffer_size = regions[0].page_count * 4096;

	return_ok(msg);
}

static void test_mem_relinquish(struct ffa_direct_msg *msg)
{
	ffa_result res = FFA_OK;
	uint64_t handle = 0;
	uint16_t endpoint_id = 0;
	struct sp_memory_transaction_flags flags = {
		.zero_memory = false,
		.operation_time_slicing = false,
	};

	if (msg->args.args64[3] == 1)
		flags.zero_memory = true;

	ffa_id_get(&endpoint_id);
	handle = (uint64_t)msg->args.args64[1] |
		 (((uint64_t)msg->args.args64[2]) << 32);

	res = sp_memory_relinquish(handle, &endpoint_id, 1, &flags);
	if (res) {
		DMSG("Failed to relinquish share");
		return_error((uint32_t)ERR_MEM_RELINQUISH, msg);
	}

	return_ok(msg);
}

static void test_mem_sharing(uint16_t service_ep_id, struct ffa_direct_msg *msg)
{
	ffa_result res = FFA_OK;
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_region region = { 0 };
	uint64_t handle = 0;
	struct ffa_mem_transaction_buffer t_buf = {0};
	uint16_t own_id = 0;
	uint16_t src_id = msg->source_id;
	struct sp_memory_access_descriptor acc_desc = { };

	my_buf[0] = 0xa;
	set_rxtx_buf(&t_buf, NULL);
	ffa_id_get(&own_id);

	region.address = (void*) my_buf;
	region.page_count = 1;
	desc.sender_id = own_id;
	desc.memory_type = sp_memory_type_normal_memory;
	desc.mem_region_attr.normal_memory.cacheability =
		sp_cacheability_write_back;

	desc.mem_region_attr.normal_memory.shareability =
		sp_shareability_inner_shareable;

	acc_desc.data_access = sp_data_access_read_write;
	acc_desc.instruction_access = sp_instruction_access_not_executable;
	acc_desc.receiver_id = service_ep_id;

	res = sp_memory_share(&desc, &acc_desc, 1, &region, 1, &handle);
	if (res != FFA_OK) {
		EMSG("Failed to share memory: %"PRId32, res);
		return_error((uint32_t)ERR_SP_SHARE, msg);
		return;
	}

	res = ffa_msg_send_direct_req_64(own_id, service_ep_id,
				      EP_RETRIEVE, handle & 0xffffffff,
				      handle >> 32, own_id, 0, msg);

	if (res != FFA_OK) {
		EMSG("Failed to send retrieve command: %"PRId32, res);
		return_error((uint32_t)ERR_SP_SHARE, msg);
		return;
	}

	res = ffa_msg_send_direct_req_64(own_id, service_ep_id,
				EP_TRY_W_ACCESS, 0,
				0, 0, 0, msg);

	if (res != FFA_OK) {
		EMSG("Failed to send TRY_W_ACCESS command: %"PRId32, res);
		return_error((uint32_t)ERR_SP_SHARE, msg);
		return;
	}

	res = ffa_msg_send_direct_req_64(own_id, service_ep_id,
				EP_RELINQUISH, handle & 0xffffffff,
				handle >> 32, 0, 0, msg);
	if (res != FFA_OK) {
		EMSG("Failed to send relinquish command: %"PRId32, res);
		return_error((uint32_t)ERR_SP_SHARE, msg);
		return;
	}

	res = ffa_mem_reclaim(handle, 0);

	if (res != FFA_OK) {
		EMSG("Failed to reclaim memory: %"PRId32, res);
		return_error((uint32_t)ERR_SP_SHARE, msg);
		return;
	}
	msg->destination_id = own_id;
	msg->source_id = src_id;

	return_ok(msg);
}

static void test_mem_multi_sharing(struct ffa_direct_msg *msg)
{
	ffa_result res = FFA_OK;
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_region region = { 0 };
	uint64_t handle = 0;
	struct ffa_mem_transaction_buffer t_buf = {0};
	uint16_t own_id = 0;
	uint16_t src_id = msg->source_id;
	struct sp_memory_access_descriptor acc_desc[2] = { };
	uint32_t err = 0;
	uint16_t endpoint2 = msg->args.args64[1];
	uint16_t endpoint3 = msg->args.args64[2];

	my_buf[0] = 0xa;
	set_rxtx_buf(&t_buf, NULL);
	ffa_id_get(&own_id);

	region.address = (void*) my_buf;
	region.page_count = 1;
	desc.sender_id = own_id;
	desc.memory_type = sp_memory_type_normal_memory;
	desc.mem_region_attr.normal_memory.cacheability =
		sp_cacheability_write_back;

	desc.mem_region_attr.normal_memory.shareability =
		sp_shareability_inner_shareable;

	acc_desc[0].data_access = sp_data_access_read_write;
	acc_desc[0].instruction_access = sp_instruction_access_not_executable;
	acc_desc[0].receiver_id = endpoint2;

	acc_desc[1].data_access = sp_data_access_read_write;
	acc_desc[1].instruction_access = sp_instruction_access_not_executable;
	acc_desc[1].receiver_id = endpoint3;

	res = sp_memory_share(&desc, acc_desc, 2, &region, 1, &handle);
	if (res != FFA_OK) {
		EMSG("Failed to share memory: %"PRId32, res);
		err = (uint32_t)ERR_SP_SHARE;
		goto err;
	}
	/* test SP2*/
	res = ffa_msg_send_direct_req_64(own_id, endpoint2,
				      EP_RETRIEVE, handle & 0xffffffff,
				      handle >> 32, own_id, 0, msg);

	if (res != FFA_OK) {
		EMSG("Failed to send retrieve command: %"PRId32, res);
		return_error((uint32_t)ERR_SP_SHARE, msg);
		return;
	}

	res = ffa_msg_send_direct_req_64(own_id, endpoint2,
				EP_TRY_W_ACCESS, 0,
				0, 0, 0, msg);

	if (res != FFA_OK) {
		EMSG("Failed to send TRY_W_ACCESS command: %"PRId32, res);
		return_error((uint32_t)ERR_SP_SHARE, msg);
		return;
	}

	if (my_buf[0] != 0xff) {
		EMSG("SP2 didn't change the value of the buffer");
		err = (uint32_t)ERR_SP_SHARE;
		goto err;
	}

	res = ffa_msg_send_direct_req_64(own_id, endpoint2,
				      EP_RELINQUISH, handle & 0xffffffff,
				      handle >> 32, 0, 0, msg);

	if (res != FFA_OK) {
		EMSG("Failed to send relinquish command: %"PRId32, res);
		return_error((uint32_t)ERR_SP_SHARE, msg);
		return;
	}
	my_buf[0] = 0xa;
	/* test SP3*/
	res = ffa_msg_send_direct_req_64(own_id, endpoint3,
				      EP_RETRIEVE, handle & 0xffffffff,
				      handle >> 32, own_id, 0, msg);

	if (res != FFA_OK) {
		EMSG("Failed to send retrieve command: %"PRId32, res);
		return_error((uint32_t)ERR_SP_SHARE, msg);
		return;
	}

	res = ffa_msg_send_direct_req_64(own_id, endpoint3,
				EP_TRY_W_ACCESS, 0,
				0, 0, 0, msg);

	if (res != FFA_OK) {
		EMSG("Failed to send TRY_W_ACCESS command: %"PRId32, res);
		return_error((uint32_t)ERR_SP_SHARE, msg);
		return;
	}

	if (my_buf[0] != 0xff) {
		EMSG("SP3 didn't change the value of the buffer");
		err = (uint32_t)ERR_SP_SHARE;
		goto err;
	}

	if (ffa_mem_reclaim(handle, 0) == FFA_OK) {
		EMSG("SP3 didn't relinquish memory yet!");
		err = (uint32_t)ERR_SP_SHARE;
		goto err;
	}

	res = ffa_msg_send_direct_req_64(own_id, endpoint3,
				EP_RELINQUISH, handle & 0xffffffff,
				handle >> 32, 0, 0, msg);

	if (res != FFA_OK) {
		EMSG("Failed to send relinquish command: %"PRId32, res);
		return_error((uint32_t)ERR_SP_SHARE, msg);
		return;
	}

	if (ffa_mem_reclaim(handle, 0) != FFA_OK) {
		EMSG("Failed to reclaim memory: %"PRId32, res);
		err = (uint32_t)ERR_SP_SHARE;
		goto err;
	}

	msg->destination_id = own_id;
	msg->source_id = src_id;
	return_ok(msg);
	return;
err:
	msg->destination_id = own_id;
	msg->source_id = src_id;
	return_error(err, msg);
}

#define TEST_FFA_MEM_SHARE(len, handle, expected) \
do { \
	ffa_result res = FFA_OK; \
	res = ffa_mem_share_rxtx(len, len, handle); \
	if (res != expected) { \
		EMSG("Invalid FFA_MEM_SHARE result: expected = %d, actual = %d", \
		     expected, res); \
		return -1; \
	} \
} while (0)

static int test_mem_sharing_invalid(uint16_t service_ep_id)
{
	uint64_t handle = 0;
	uint16_t own_id = 0;
	size_t len = 0;
	struct ffa_mem_transaction_desc *transaction = NULL;
	struct ffa_mem_access_desc *acc_desc = NULL;
	struct ffa_composite_mem_region_desc *comp_desc = NULL;
	struct ffa_constituent_mem_region_desc *region = NULL;

	memset((void *)tx_buffer, 0x00, sizeof(tx_buffer));

	transaction = (struct ffa_mem_transaction_desc *)tx_buffer;

	ffa_id_get(&own_id);

	transaction->sender_id = own_id;
	transaction->mem_region_attr = 0x24;
	transaction->flags = 0;
	transaction->handle = 0;
	transaction->tag = 0;

	len = sizeof(*transaction);

	/* Zero offset, size and count */
	TEST_FFA_MEM_SHARE(len, &handle, FFA_INVALID_PARAMETERS);

#if CFG_FFA_VERSION >= FFA_VERSION_1_1
	/* Zero count */
	transaction->mem_access_desc_size = sizeof(struct ffa_mem_access_desc);
	transaction->mem_access_desc_offset = sizeof(*transaction);
#endif /* CFG_FFA_VERSION */

	/* Too many mem access desc */
	transaction->mem_access_desc_count = sizeof(tx_buffer);
	TEST_FFA_MEM_SHARE(len, &handle, FFA_INVALID_PARAMETERS);

	transaction->mem_access_desc_count = 1;

#if CFG_FFA_VERSION >= FFA_VERSION_1_1
	/* Invalid offset */
	transaction->mem_access_desc_offset = sizeof(tx_buffer);
	TEST_FFA_MEM_SHARE(len, &handle, FFA_INVALID_PARAMETERS);

	transaction->mem_access_desc_offset = sizeof(*transaction);
#endif /* CFG_FFA_VERSION */

	acc_desc = (struct ffa_mem_access_desc *)(tx_buffer + len);
	len +=  sizeof(*acc_desc);

	acc_desc->mem_access_perm_desc.endpoint_id = service_ep_id;
	acc_desc->mem_access_perm_desc.mem_access_permissions = 0x06; /* RWnX */

	/* Too large memory region descriptor offset */
	acc_desc->composite_mem_region_desc_offset = sizeof(tx_buffer);
	TEST_FFA_MEM_SHARE(len, &handle, FFA_INVALID_PARAMETERS);

	acc_desc->composite_mem_region_desc_offset = len;
	comp_desc = (struct ffa_composite_mem_region_desc *)(tx_buffer + len);
	len += sizeof(*comp_desc);

	/* Zero pages and address ranges */
	TEST_FFA_MEM_SHARE(len, &handle, FFA_INVALID_PARAMETERS);

	region = (struct ffa_constituent_mem_region_desc *)(tx_buffer + len);
	len += sizeof(*region);

	/* One region with zero pages */
	region->address = (uint64_t)shared_buffer;
	comp_desc->address_range_count = 1;
	TEST_FFA_MEM_SHARE(len, &handle, FFA_INVALID_PARAMETERS);

	/* One region with not matching sum pages */
	region->address = (uint64_t)shared_buffer;
	comp_desc->address_range_count = 1;
	comp_desc->total_page_count = 2;
	region->page_count = 1;
	TEST_FFA_MEM_SHARE(len, &handle, FFA_INVALID_PARAMETERS);

	/* Too many regions */
	comp_desc->total_page_count = sizeof(tx_buffer);
	comp_desc->address_range_count = sizeof(tx_buffer);
	TEST_FFA_MEM_SHARE(len, &handle, FFA_INVALID_PARAMETERS);

	return 0;
}

static void test_mem_sharing_inccorrect_access(uint16_t service_ep_id,
					struct ffa_direct_msg *msg)
{
	ffa_result res = FFA_OK;
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_region region = { 0 };
	uint64_t handle = 0;
	struct ffa_mem_transaction_buffer t_buf = {0};
	uint16_t own_id = 0;
	uint16_t src_id = msg->source_id;
	struct sp_memory_access_descriptor acc_desc = { };

	set_rxtx_buf(&t_buf, NULL);
	ffa_id_get(&own_id);

	region.address = (void*) my_buf;
	region.page_count = 1;
	desc.sender_id = own_id;
	desc.memory_type = sp_memory_type_normal_memory;
	desc.mem_region_attr.normal_memory.cacheability =
		sp_cacheability_write_back;

	desc.mem_region_attr.normal_memory.shareability =
		sp_shareability_inner_shareable;

	acc_desc.data_access = sp_data_access_read_write;
	acc_desc.instruction_access = sp_instruction_access_executable;
	acc_desc.receiver_id = service_ep_id;

	res = sp_memory_share(&desc, &acc_desc, 1, &region, 1, &handle);
	if (res == FFA_OK) {
		EMSG("ffa_memory_share(): error %"PRId32, res);
		return_error((uint32_t)ERR_SP_SHARE, msg);
		return;
	}

	if (test_mem_sharing_invalid(service_ep_id)) {
		return_error((uint32_t)ERR_SP_SHARE, msg);
		return;
	}

	msg->destination_id = own_id;
	msg->source_id = src_id;
	return_ok(msg);
}

static void test_mem_sharing_exc(uint16_t service_ep_id,
				 struct ffa_direct_msg *msg)
{
	ffa_result res = FFA_OK;
	struct sp_memory_descriptor desc = { 0 };
	struct sp_memory_region region = { 0 };
	uint64_t handle = 0;
	uint64_t handle2 = 0;
	struct ffa_mem_transaction_buffer t_buf = {0};
	uint16_t own_id = 0;
	uint16_t src_id = msg->source_id;
	struct sp_memory_access_descriptor acc_desc = { };
	uint32_t err = 0;

	set_rxtx_buf(&t_buf, NULL);
	ffa_id_get(&own_id);

	region.address = (void*) my_buf;
	region.page_count = 1;
	desc.sender_id = own_id;
	desc.memory_type = sp_memory_type_normal_memory;
	desc.mem_region_attr.normal_memory.cacheability =
		sp_cacheability_write_back;

	desc.mem_region_attr.normal_memory.shareability =
		sp_shareability_inner_shareable;

	acc_desc.data_access = sp_data_access_read_write;
	acc_desc.instruction_access = sp_instruction_access_not_executable;
	acc_desc.receiver_id = service_ep_id;

	res = sp_memory_share(&desc, &acc_desc, 1, &region, 1, &handle);
	if (res != FFA_OK) {
		EMSG("test_mem_sharing_exc(): error %"PRId32, res);
		err = (uint32_t)ERR_SP_SHARE_EXC;
		goto err;
	}

	/*
	 * Try it again, it should fail as we don't have acclusive access
	 * anymore
	 */
	res = sp_memory_share(&desc, &acc_desc, 1, &region, 1, &handle2);
	if (res == FFA_OK) {
		EMSG("test_mem_sharing_exc(): error %"PRId32, res);
		err = (uint32_t)ERR_SP_SHARE_EXC;
		goto err;
	}

	res = ffa_mem_reclaim(handle, 0);

	if (res != FFA_OK) {
		EMSG("ffa_memory_share(): error % in %s:%d"PRId32, res,
							          __FILE__,
							          __LINE__);
		return_error((uint32_t)ERR_SP_SHARE, msg);
		return;
	}

	msg->destination_id = own_id;
	msg->source_id = src_id;
	return_ok(msg);
	return;
err:
	msg->destination_id = own_id;
	msg->source_id = src_id;
	return_error(err, msg);
}

void test_mem_get_set(union ffa_boot_info *boot_info)
{
	void *addr = NULL;
	ffa_result res = FFA_OK;
	struct memory_region buffer_region = { 0 };
	uint32_t mem_perm = 0;
	const uint32_t original_perm = FFA_MEM_PERM_INSTRUCTION_ACCESS_PERM_NX |
				       FFA_MEM_PERM_DATA_ACCESS_PERM_RW;
	const uint32_t ro_perm = FFA_MEM_PERM_INSTRUCTION_ACCESS_PERM_NX |
				 FFA_MEM_PERM_DATA_ACCESS_PERM_RO;

	DMSG("Testing FFA_MEM_PERM_GET/SET");
	config_ramstore_init();

	if (!sp_config_load(boot_info)) {
		EMSG("Failed to load SP config");
		goto err;
	}

	/* Only run the test if we have the test-region enabled */
	if (!config_store_query(CONFIG_CLASSIFIER_MEMORY_REGION, "test-region",
				0, &buffer_region, sizeof(buffer_region)))
		return;

	addr = (void *)buffer_region.base_addr;
	/* Check original permissions */
	res = ffa_mem_perm_get(addr, &mem_perm);
	if (res)
		goto err;

	if (mem_perm != original_perm) {
		EMSG("Incorrect permision got 0x%x expected 0x%x", mem_perm, original_perm);
		res = FFA_INVALID_PARAMETERS;
		goto err;
	}

	/* Remove write permission */
	res = ffa_mem_perm_set(addr, 1, ro_perm);
	if (res)
		goto err;

	/* Check if write permission is removed */
	res = ffa_mem_perm_get(addr, &mem_perm);
	if (res)
		goto err;

	if (mem_perm != ro_perm) {
		EMSG("Incorrect permision got 0x%x expected 0x%x", mem_perm, original_perm);
		res = FFA_INVALID_PARAMETERS;
		goto err;
	}
	/* Set write permission back */
	res = ffa_mem_perm_set(addr, 1, original_perm);
	if (res)
		goto err;

	/* Check original permissions */
	res = ffa_mem_perm_get(addr, &mem_perm);
	if (res)
		goto err;

	if (mem_perm != original_perm) {
		EMSG("Incorrect permision got 0x%x expected 0x%x", mem_perm, original_perm);
		res = FFA_INVALID_PARAMETERS;
		goto err;
	}

	return;
err:
	EMSG("GET/SET_MEM failed (0x%x)", res);
}

void __noreturn sp_main(union ffa_boot_info *boot_info) {
	struct ffa_direct_msg msg = {0};
	uint16_t own_id = 0;

	/* Boot phase */
	if (sp_discovery_own_id_get(&own_id) != SP_RESULT_OK) {
		EMSG("Couldn't get own_id!!");
	}

	test_ffa_rxtx_map();
	test_mem_get_set(boot_info);
	/* End of boot phase */
	test_ffa_partition_info_get();
	ffa_msg_wait(&msg);

	while (1) {
		enum sp_tests test_case = (enum sp_tests)msg.args.args64[0];

		DMSG("SP:%x Starting test %s", own_id, sp_test_str[test_case]);
		switch (test_case) {
		case EP_TEST_SP:
			test_internal_sp(&msg);
			break;
		case EP_TEST_SP_COMMUNICATION:
			test_communication(&msg);
			break;
		case EP_TEST_SP_COMMUNICATION_RESPONSE:
			test_communication_response(&msg);
			break;
		case EP_TEST_SP_INCREASE:
			test_increase(&msg);
			break;
		case EP_TRY_R_ACCESS:
			test_read_access();
			return_ok(&msg);
			break;
		case EP_TRY_W_ACCESS:
			test_write_access();
			return_ok(&msg);
			break;
		case EP_RETRIEVE:
			test_mem_retrieve(&msg);
			break;
		case EP_RELINQUISH:
			test_mem_relinquish(&msg);
			break;
		case EP_SP_MEM_SHARING:
			test_mem_sharing((uint16_t)msg.args.args64[1], &msg);
			break;
		case EP_SP_MEM_SHARING_MULTI:
			test_mem_multi_sharing(&msg);
			break;
		case EP_SP_MEM_SHARING_EXC:
			test_mem_sharing_exc((uint16_t)msg.args.args64[1],
					     &msg);
			break;
		case EP_SP_MEM_INCORRECT_ACCESS:
			test_mem_sharing_inccorrect_access(
				(uint16_t)msg.args.args64[1], &msg);
			break;
		case EP_SP_NOP:
			return_ok(&msg);
			break;

		default:
			return_error((uint32_t)ERR_TEST_NOT_FOUND, &msg);
			break;
		}
	}
}

void sp_interrupt_handler(uint32_t interrupt_id)
{
	(void)interrupt_id;
	DMSG("Got interrupt %x", interrupt_id);
}

ffa_result ffa_vm_created_handler(uint16_t vm_id, uint64_t handle)
{
	(void)handle;
	DMSG("VM with ID %d created", vm_id);

	return FFA_OK;
}

ffa_result ffa_vm_destroyed_handler(uint16_t vm_id, uint64_t handle)
{
	(void)handle;
	DMSG("VM with ID %d destroyed", vm_id);

	return FFA_OK;
}
