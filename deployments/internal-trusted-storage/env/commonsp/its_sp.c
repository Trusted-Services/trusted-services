/*
 * Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "components/rpc/common/endpoint/rpc_service_interface.h"
#include "components/rpc/ts_rpc/endpoint/sp/ts_rpc_endpoint_sp.h"
#include "components/service/secure_storage/factory/storage_factory.h"
#include "components/service/secure_storage/frontend/secure_storage_provider/secure_storage_provider.h"
#include "components/service/secure_storage/frontend/secure_storage_provider/secure_storage_uuid.h"
#include "components/service/log/factory/log_factory.h"
#include "sp_api.h"
#include "sp_discovery.h"
#include "sp_messaging.h"
#include "sp_rxtx.h"
#include "trace.h"

static uint8_t tx_buffer[4096] __aligned(4096);
static uint8_t rx_buffer[4096] __aligned(4096);

void sp_main(union ffa_boot_info *boot_info)
{
	sp_result result = SP_RESULT_INTERNAL_ERROR;
	struct rpc_service_interface *secure_storage_iface = NULL;
	struct ts_rpc_endpoint_sp rpc_endpoint = { 0 };
	struct sp_msg req_msg = { 0 };
	struct sp_msg resp_msg = { 0 };
	struct secure_storage_provider secure_storage_provider = { 0 };
	struct storage_backend *storage_backend = NULL;
	uint16_t own_id = 0;
	const struct rpc_uuid service_uuid = { .uuid = TS_PSA_INTERNAL_TRUSTED_STORAGE_UUID };
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;

	/* Boot */
	(void)boot_info;

	result = sp_rxtx_buffer_map(tx_buffer, rx_buffer, sizeof(rx_buffer));
	if (result != SP_RESULT_OK) {
		EMSG("Failed to map RXTX buffers: %d", result);
		goto fatal_error;
	}

	IMSG("Start discovering logging service");
	if (log_factory_create()) {
		IMSG("Logging service discovery successful");
	} else {
		EMSG("Logging service discovery failed, falling back to console log");
	}

	result = sp_discovery_own_id_get(&own_id);
	if (result != SP_RESULT_OK) {
		EMSG("Failed to query own ID: %d", result);
		goto fatal_error;
	}

	storage_backend = storage_factory_create(storage_factory_security_class_INTERNAL_TRUSTED);
	if (!storage_backend) {
		EMSG("Failed to create storage backend");
		goto fatal_error;
	}

	secure_storage_iface = secure_storage_provider_init(&secure_storage_provider,
							    storage_backend, &service_uuid);
	if (!secure_storage_iface) {
		EMSG("Failed to init secure storage provider");
		goto fatal_error;
	}

	rpc_status = ts_rpc_endpoint_sp_init(&rpc_endpoint, 1, 16);
	if (rpc_status != RPC_SUCCESS) {
		EMSG("Failed to initialize RPC endpoint: %d", rpc_status);
		goto fatal_error;
	}

	rpc_status = ts_rpc_endpoint_sp_add_service(&rpc_endpoint, secure_storage_iface);
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
	EMSG("ITS SP error");
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
