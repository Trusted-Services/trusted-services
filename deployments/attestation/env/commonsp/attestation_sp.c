/*
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "components/rpc/ts_rpc/endpoint/sp/ts_rpc_endpoint_sp.h"
#include "components/rpc/ts_rpc/caller/sp/ts_rpc_caller_sp.h"
#include "components/rpc/common/caller/rpc_caller_session.h"
#include "protocols/rpc/common/packed-c/status.h"
#include "config/ramstore/config_ramstore.h"
#include "config/loader/sp/sp_config_loader.h"
#include "service/attestation/provider/attest_provider.h"
#include "service/attestation/provider/serializer/packed-c/packedc_attest_provider_serializer.h"
#include "service/attestation/claims/claims_register.h"
#include "service/attestation/claims/sources/event_log/event_log_claim_source.h"
#include "service/attestation/claims/sources/boot_seed_generator/boot_seed_generator.h"
#include "service/attestation/claims/sources/null_lifecycle/null_lifecycle_claim_source.h"
#include "service/attestation/claims/sources/instance_id/instance_id_claim_source.h"
#include "service/attestation/claims/sources/implementation_id/implementation_id_claim_source.h"
#include "service/attestation/key_mngr/local/local_attest_key_mngr.h"
#include "service/crypto/client/psa/psa_crypto_client.h"
#include "service/log/factory/log_factory.h"
#include "service_locator.h"
#include "psa/crypto.h"
#include "sp_api.h"
#include "sp_discovery.h"
#include "sp_rxtx.h"
#include "trace.h"

static bool sp_init(uint16_t *own_sp_id);
static bool locate_crypto_service(void);

void __noreturn sp_main(union ffa_boot_info *boot_info)
{
	/* Service provider objects */
	struct attest_provider attest_provider = { 0 };
	struct rpc_service_interface *attest_iface = NULL;
	struct ts_rpc_endpoint_sp rpc_endpoint = { 0 };
	struct sp_msg req_msg = { 0 };
	struct sp_msg resp_msg = { 0 };
	uint16_t own_id = 0;
	sp_result result = SP_RESULT_INTERNAL_ERROR;
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;

	/* Claim source objects */
	struct claim_source *claim_source = NULL;
	struct event_log_claim_source event_log_claim_source = { 0 };
	struct boot_seed_generator boot_seed_claim_source = { 0 };
	struct null_lifecycle_claim_source lifecycle_claim_source = { 0 };
	struct instance_id_claim_source instance_id_claim_source = { 0 };
	struct implementation_id_claim_source implementation_id_claim_source = { 0 };

	/*********************************************************
	 * Boot phase
	 *********************************************************/
	if (!sp_init(&own_id)) {
		EMSG("Failed to init SP");
		goto fatal_error;
	}

	config_ramstore_init();

	if (!sp_config_load(boot_info)) {
		EMSG("Failed to load SP config");
		goto fatal_error;
	}

	/**
	 * Locate crypto service endpoint and establish RPC session
	 */
	if (!locate_crypto_service()) {
		EMSG("Failed to locate crypto service");
		goto fatal_error;
	}

	/**
	 * Register claim sources for deployment
	 */
	claims_register_init();

	/* Boot measurement claim source */
	claim_source = event_log_claim_source_init_from_config(&event_log_claim_source);
	if (!claim_source) {
		EMSG("Failed to claim event log source from config");
		goto fatal_error;
	}
	claims_register_add_claim_source(CLAIM_CATEGORY_BOOT_MEASUREMENT, claim_source);

	/* Boot seed claim source */
	claim_source = boot_seed_generator_init(&boot_seed_claim_source);
	if (!claim_source) {
		EMSG("Failed to initialize boot seed generator");
		goto fatal_error;
	}
	claims_register_add_claim_source(CLAIM_CATEGORY_DEVICE, claim_source);

	/* Lifecycle state claim source */
	claim_source = null_lifecycle_claim_source_init(&lifecycle_claim_source);
	if (!claim_source) {
		EMSG("Failed to initialize lifecycle state claim source");
		goto fatal_error;
	}
	claims_register_add_claim_source(CLAIM_CATEGORY_DEVICE, claim_source);

	/* Instance ID claim source */
	claim_source = instance_id_claim_source_init(&instance_id_claim_source);
	if (!claim_source) {
		EMSG("Failed to initialize instance ID claim source");
		goto fatal_error;
	}
	claims_register_add_claim_source(CLAIM_CATEGORY_DEVICE, claim_source);

	/* Implementation ID claim source */
	claim_source = implementation_id_claim_source_init(&implementation_id_claim_source,
		"trustedfirmware.org.ts.attestation_sp");
	if (!claim_source) {
		EMSG("Failed to initialize implementation ID claim source");
		goto fatal_error;
	}
	claims_register_add_claim_source(CLAIM_CATEGORY_DEVICE, claim_source);

	/**
	 * Initialize the service provider
	 */
	local_attest_key_mngr_init(LOCAL_ATTEST_KEY_MNGR_VOLATILE_IAK);
	attest_iface = attest_provider_init(&attest_provider);
	if (!attest_iface) {
		EMSG("Failed to initialize attestation provider");
		goto fatal_error;
	}

	attest_provider_register_serializer(&attest_provider,
					    packedc_attest_provider_serializer_instance());

	rpc_status = ts_rpc_endpoint_sp_init(&rpc_endpoint, 1, 16);
	if (rpc_status != RPC_SUCCESS) {
		EMSG("Failed to initialize RPC endpoint: %d", rpc_status);
		goto fatal_error;
	}

	rpc_status = ts_rpc_endpoint_sp_add_service(&rpc_endpoint, attest_iface);
	if (rpc_status != RPC_SUCCESS) {
		EMSG("Failed to add service to RPC endpoint: %d", rpc_status);
		goto fatal_error;
	}

	/*********************************************************
	 * End of boot phase
	 *********************************************************/
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
	EMSG("Attestation SP error");
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

bool locate_crypto_service(void)
{
	struct rpc_caller_session *session = NULL;
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;

	service_locator_init();

	/* todo - add option to use configurable crypto service location */
	struct service_context *crypto_service_context =
		service_locator_query("sn:ffa:d9df52d5-16a2-4bb2-9aa4-d26d3b84e8c0:0");

	if (!crypto_service_context) {
		EMSG("Service locator query failed");
		return false;
	}

	session = service_context_open(crypto_service_context);
	if (!session) {
		EMSG("Failed to open crypto service context");
		return false;
	}

	psa_status = psa_crypto_client_init(session);
	if (psa_status != PSA_SUCCESS) {
		EMSG("Failed to init PSA crypto client: %d", psa_status);
		return false;
	}

	psa_status = psa_crypto_init();
	if (psa_status != PSA_SUCCESS) {
		EMSG("Failed to init PSA crypto: %d", psa_status);
		return false;
	}

	return true;
}
