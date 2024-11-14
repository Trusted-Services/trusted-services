// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 */

#include "common/crc32/crc32.h"
#include "rpc/ts_rpc/endpoint/sp/ts_rpc_endpoint_sp.h"
#include "protocols/rpc/common/packed-c/status.h"
#include "config/ramstore/config_ramstore.h"
#include "config/loader/sp/sp_config_loader.h"
#include "service/block_storage/provider/block_storage_provider.h"
#include "service/block_storage/provider/serializer/packed-c/packedc_block_storage_serializer.h"
#include "service/block_storage/factory/block_store_factory.h"
#include "service/log/factory/log_factory.h"
#include "sp_api.h"
#include "sp_discovery.h"
#include "sp_messaging.h"
#include "sp_rxtx.h"
#include "trace.h"


static bool sp_init(uint16_t *own_sp_id);

void __noreturn sp_main(union ffa_boot_info *boot_info)
{
	struct ts_rpc_endpoint_sp rpc_endpoint = { 0 };
	struct block_storage_provider service_provider = { 0 };
	struct block_store *backend = NULL;
	struct rpc_service_interface *service_iface = NULL;
	struct sp_msg req_msg = { 0 };
	struct sp_msg resp_msg = { 0 };
	uint16_t own_id = 0;
	sp_result result = SP_RESULT_INTERNAL_ERROR;
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;

	/* Boot phase */
	if (!sp_init(&own_id)) {
		EMSG("Failed to init SP");
		goto fatal_error;
	}

	config_ramstore_init();

	if (!sp_config_load(boot_info)) {
		EMSG("Failed to load SP config");
		goto fatal_error;
	}

	crc32_init();

	/* Initialise the service provider and backend block store */
	backend = block_store_factory_create();
	if (!backend) {
		EMSG("Failed to create block store");
		goto fatal_error;
	}

	service_iface = block_storage_provider_init(
		&service_provider,
		backend);
	if (!service_iface) {
		EMSG("Failed to init service provider");
		goto fatal_error;
	}

	block_storage_provider_register_serializer(&service_provider,
						   packedc_block_storage_serializer_instance());

	/* Associate service interface with FFA call endpoint */
	rpc_status = ts_rpc_endpoint_sp_init(&rpc_endpoint, 1, 16);
	if (rpc_status != RPC_SUCCESS) {
		EMSG("Failed to initialize RPC endpoint: %d", rpc_status);
		goto fatal_error;
	}

	rpc_status = ts_rpc_endpoint_sp_add_service(&rpc_endpoint, service_iface);
	if (rpc_status != RPC_SUCCESS) {
		EMSG("Failed to add service to RPC endpoint: %d", rpc_status);
		goto fatal_error;
	}

	/* End of boot phase */
	result = sp_msg_wait(&req_msg);
	if (result != SP_RESULT_OK) {
		EMSG("Failed to send message wait %d", result);
		goto fatal_error;
	}

	while (1) {
		ts_rpc_endpoint_sp_receive(&rpc_endpoint, &req_msg, &resp_msg);

		result = sp_msg_send_direct_resp(&resp_msg, &req_msg);
		if (result != SP_RESULT_OK) {
			EMSG("Failed to send direct response %d", result);
			result = sp_msg_wait(&req_msg);
			if (result != SP_RESULT_OK) {
				EMSG("Failed to send message wait %d", result);
				goto fatal_error;
			}
		}
	}

fatal_error:
	/* SP is not viable */
	EMSG("Block storage SP error");
	while (1) {}
}

void sp_interrupt_handler(uint32_t interrupt_id)
{
	(void)interrupt_id;
}

ffa_result ffa_vm_created_handler(uint16_t vm_id, uint64_t handle)
{
	(void)vm_id;
	(void)handle;

	return FFA_OK;
}

ffa_result ffa_vm_destroyed_handler(uint16_t vm_id, uint64_t handle)
{
	(void)vm_id;
	(void)handle;

	return FFA_OK;
}

static bool sp_init(uint16_t *own_id)
{
	sp_result sp_res = SP_RESULT_INTERNAL_ERROR;
	static uint8_t tx_buffer[4096] __aligned(4096);
	static uint8_t rx_buffer[4096] __aligned(4096);

	sp_res = sp_rxtx_buffer_map(tx_buffer, rx_buffer, sizeof(rx_buffer));
	if (sp_res != SP_RESULT_OK) {
		EMSG("Failed to map RXTX buffers: %d", sp_res);
		return false;
	}

	IMSG("Start discovering logging service");
	if (log_factory_create()) {
		IMSG("Logging service discovery successful");
	} else {
		EMSG("Logging service discovery failed, falling back to console log");
	}

	sp_res = sp_discovery_own_id_get(own_id);
	if (sp_res != SP_RESULT_OK) {
		EMSG("Failed to query own ID: %d", sp_res);
		return false;
	}

	return true;
}
