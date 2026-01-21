/*
 * Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "attestation_service_context.h"
#include <service/attestation/provider/serializer/packed-c/packedc_attest_provider_serializer.h>
#include <service/attestation/claims/claims_register.h>
#include <service/attestation/claims/sources/event_log/event_log_claim_source.h>
#include <service/attestation/claims/sources/event_log/mock/mock_event_log.h>
#include <service/attestation/key_mngr/local/local_attest_key_mngr.h>
#include <config/ramstore/config_ramstore.h>
#include <config/interface/config_store.h>
#include <config/interface/config_blob.h>
#include <service/crypto/client/psa/psa_crypto_client.h>
#include <psa/crypto.h>

attestation_service_context::attestation_service_context(const char *sn) :
	standalone_service_context(sn),
	m_attest_provider(),
	m_event_log_claim_source(),
	m_boot_seed_claim_source(),
	m_lifecycle_claim_source(),
	m_instance_id_claim_source(),
	m_implementation_id_claim_source(),
	m_crypto_service_context(NULL),
	m_crypto_session(NULL)
{

}

attestation_service_context::~attestation_service_context()
{

}

void attestation_service_context::do_init()
{
	struct claim_source *claim_source;
	struct config_blob event_log_blob;
	const struct mock_event_log_info *event_log_info = mock_event_log_list_get_default();


	/* The crypto service is used for token signing */
	open_crypto_session();

	/**
	 * Initialize the config_store and load dynamic parameters.  For
	 * the attestation service provider, the TPM event log is expected
	 * to be loaded as a dynamic parameter.
	 */
	config_ramstore_init();

	event_log_blob.data = event_log_info->data;
	event_log_blob.data_len = event_log_info->data_length;

	config_store_add(CONFIG_CLASSIFIER_BLOB,
		"EVENT_LOG", 0, &event_log_blob, sizeof(event_log_blob));

	/**
	 * Initialize and register claims sources to define the view of
	 * the device reflected by the attestation service.  On a real
	 * device, the set of claim sources will be deployment specific
	 * to accommodate specific device architecture and product
	 * variations.
	 */
	claims_register_init();

	/* Boot measurement claim source */
	claim_source = event_log_claim_source_init_from_config(&m_event_log_claim_source);
	claims_register_add_claim_source(CLAIM_CATEGORY_BOOT_MEASUREMENT, claim_source);

	/* Boot seed claim source */
	claim_source = boot_seed_generator_init(&m_boot_seed_claim_source);
	claims_register_add_claim_source(CLAIM_CATEGORY_DEVICE, claim_source);

	/* Lifecycle state claim source */
	claim_source = null_lifecycle_claim_source_init(&m_lifecycle_claim_source);
	claims_register_add_claim_source(CLAIM_CATEGORY_DEVICE, claim_source);

	/* Instance ID claim source */
	claim_source = instance_id_claim_source_init(&m_instance_id_claim_source);
	claims_register_add_claim_source(CLAIM_CATEGORY_DEVICE, claim_source);

	/* Implementation ID claim source */
	claim_source = implementation_id_claim_source_init(&m_implementation_id_claim_source,
		"trustedfirmware.org.ts.standalone");
	claims_register_add_claim_source(CLAIM_CATEGORY_DEVICE, claim_source);

	/* Initialize the attestation service provider */
	local_attest_key_mngr_init(LOCAL_ATTEST_KEY_MNGR_VOLATILE_IAK);
	struct rpc_service_interface *attest_ep = attest_provider_init(&m_attest_provider);

	attest_provider_register_serializer(&m_attest_provider, packedc_attest_provider_serializer_instance());

	standalone_service_context::set_rpc_interface(attest_ep);
}

void attestation_service_context::do_deinit()
{
	attest_provider_deinit(&m_attest_provider);
	claims_register_deinit();
	config_ramstore_deinit();
	local_attest_key_mngr_deinit();
	close_crypto_session();
}

void attestation_service_context::open_crypto_session()
{
	m_crypto_service_context = service_locator_query("sn:trustedfirmware.org:crypto:0");
	if (m_crypto_service_context) {
		m_crypto_session = service_context_open(m_crypto_service_context);
		if (m_crypto_session) {
			psa_crypto_client_init(m_crypto_session);
			psa_crypto_init();
		}
	}
}

void attestation_service_context::close_crypto_session()
{
	psa_crypto_client_deinit();

	if (m_crypto_service_context && m_crypto_session) {
		service_context_close(m_crypto_service_context, m_crypto_session);
		m_crypto_session = NULL;

		service_context_relinquish(m_crypto_service_context);
		m_crypto_service_context = NULL;
	}
}
