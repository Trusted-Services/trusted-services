// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 */

#include "common/trace/include/trace.h"
#include "config/ramstore/config_ramstore.h"
#include "config/interface/config_store.h"
#include "config/loader/sp/sp_config_loader.h"
#include "platform/interface/device_region.h"
#include "rpc/common/endpoint/rpc_service_interface.h"
#include "rpc/tpm_crb_ffa/endpoint/sp/tpm_crb_ffa_endpoint.h"
#include "service/crypto/backend/mbedcrypto/trng_adapter/trng_adapter.h"
#include "service/log/factory/log_factory.h"
#include "service/secure_storage/factory/storage_factory.h"
#include "service/secure_storage/frontend/psa/ps/ps_frontend.h"
#include "service/tpm/backend/ms_tpm/ms_tpm_backend.h"
#include "service/tpm/provider/tpm_crb_provider.h"
#include "service_locator.h"
#include "sp_api.h"
#include "sp_discovery.h"
#include "sp_messaging.h"
#include "sp_rxtx.h"

#include "ftpm_sp.h"

#define CONFIG_NAME_TPM_CRB_NS_REGION "tpm-crb-ns"
#define CONFIG_NAME_TPM_CRB_S_REGION "tpm-crb-s"

static uint8_t tx_buffer[4096] __aligned(4096);
static uint8_t rx_buffer[4096] __aligned(4096);

void __noreturn sp_main(union ffa_boot_info *boot_info)
{
	struct tpm_crb_provider service_provider = { 0 };
	struct device_region tpm_crb_ns_region = { 0 };
	struct device_region tpm_crb_s_region = { 0 };
	struct tpm_crb_ffa_ep rpc_endpoint = { 0 };
	struct rpc_service_interface *service_iface = NULL;
	struct storage_backend *storage_backend = NULL;
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;
	sp_result sp_res = SP_RESULT_INTERNAL_ERROR;
	struct sp_msg resp_msg = { 0 };
	struct sp_msg req_msg = { 0 };
	uint16_t own_id = 0;

	/* Boot phase */
	sp_res = sp_rxtx_buffer_map(tx_buffer, rx_buffer, sizeof(rx_buffer));
	if (sp_res != SP_RESULT_OK) {
		EMSG("Failed to map RXTX buffers: %d", sp_res);
		goto fatal_error;
	}

	IMSG("Start discovering logging service");
	if (log_factory_create()) {
		IMSG("Logging service discovery successful");
	} else {
		EMSG("Logging service discovery failed, falling back to console log");
	}

	sp_res = sp_discovery_own_id_get(&own_id);
	if (sp_res != SP_RESULT_OK) {
		EMSG("Failed to query own ID: %d", sp_res);
		goto fatal_error;
	}

	config_ramstore_init();

	if (!sp_config_load(boot_info)) {
		EMSG("Failed to load SP config");
		goto fatal_error;
	}

	if (!config_store_query(CONFIG_CLASSIFIER_DEVICE_REGION, CONFIG_NAME_TPM_CRB_NS_REGION, 0,
				&tpm_crb_ns_region, sizeof(tpm_crb_ns_region))) {
		EMSG(CONFIG_NAME_TPM_CRB_NS_REGION " is not set in SP configuration");
		goto fatal_error;
	}

	DMSG("Found TPM CRB NS careveout with address: 0x%lx, size: %ld",
		tpm_crb_ns_region.base_addr, tpm_crb_ns_region.io_region_size);

	if (!config_store_query(CONFIG_CLASSIFIER_DEVICE_REGION, CONFIG_NAME_TPM_CRB_S_REGION, 0,
				&tpm_crb_s_region, sizeof(tpm_crb_s_region))) {
		EMSG(CONFIG_NAME_TPM_CRB_S_REGION " is not set in SP configuration");
		goto fatal_error;
	}

	DMSG("Found TPM CRB S careveout with address: 0x%lx, size: %ld",
		tpm_crb_s_region.base_addr, tpm_crb_s_region.io_region_size);

	/* Create a storage backend for persistent key storage - prefer PS */
	storage_backend = storage_factory_create(storage_factory_security_class_PROTECTED);
	if (!storage_backend) {
		EMSG("Failed to create storage factory");
		goto fatal_error;
	}

	status = psa_ps_frontend_init(storage_backend);
	if (status != PSA_SUCCESS) {
		EMSG("Failed to init protected storage frontend: %d", status);
		goto fatal_error;
	}

	/* Initialize TRNG */
	status = trng_adapter_init(0);
	if (status != PSA_SUCCESS) {
		EMSG("Failed to init TRNG adapter: %d", status);
		goto fatal_error;
	}

	if (!ms_tpm_backend_init()) {
		EMSG("ms_tpm backend init failed");
		goto fatal_error;
	}

	service_iface = tpm_provider_init(&service_provider,
					  (uint8_t *)tpm_crb_ns_region.base_addr,
					  tpm_crb_ns_region.io_region_size,
					  (uint8_t *)tpm_crb_s_region.base_addr,
					  tpm_crb_s_region.io_region_size);
	if (!service_iface) {
		EMSG("Failed to init service provider");
		goto fatal_error;
	}

	if (!tpm_crb_ffa_endpoint_init(&rpc_endpoint)) {
		EMSG("Failed to initialize RPC endpoint");
		goto fatal_error;
	}

	if (!tpm_crb_ffa_endpoint_add_service(&rpc_endpoint, service_iface)) {
		EMSG("Failed to add service to RPC endpoint");
		goto fatal_error;
	}

	/* End of boot phase */

	sp_res = sp_msg_wait(&req_msg);
	if (sp_res != SP_RESULT_OK) {
		EMSG("Failed to send message wait %d", sp_res);
		goto fatal_error;
	}

	while (1) {
		tpm_crb_ffa_endpoint_receive(&rpc_endpoint, &req_msg, &resp_msg);

		sp_res = sp_msg_send_direct_resp(&resp_msg, &req_msg);
		if (sp_res != SP_RESULT_OK) {
			EMSG("Failed to send direct response %d", sp_res);
			sp_res = sp_msg_wait(&req_msg);
			if (sp_res != SP_RESULT_OK) {
				EMSG("Failed to send message wait %d", sp_res);
				goto fatal_error;
			}
		}
	}

fatal_error:
	EMSG("fTPM SP error");
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
