/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <protocols/rpc/common/packed-c/encoding.h>
#include <psa/initial_attestation.h>
#include <service/attestation/client/provision/attest_provision_client.h>
#include <service/attestation/client/psa/iat_client.h>
#include <service_locator.h>
#include <stdio.h>

#include "libpsats.h"
#include "trace.h"

static struct rpc_caller_session *rpc_session;
static struct service_context *attestation_service_context;

LIBPSATS_EXPORTED psa_status_t libpsats_init_attestation_context(const char *service_name)
{
	psa_status_t result = PSA_ERROR_GENERIC_ERROR;
	psa_status_t provision_result = PSA_ERROR_GENERIC_ERROR;

	if (rpc_session || attestation_service_context) {
		EMSG("The client is already initialized\n");
		return result;
	}

	service_locator_init();

	attestation_service_context = service_locator_query(service_name);

	if (!attestation_service_context) {
		EMSG("Failed to discover service\n");
		return result;
	}

	rpc_session = service_context_open(attestation_service_context);

	if (!rpc_session) {
		EMSG("Failed to open rpc session\n");
		libpsats_deinit_attestation_context();
		return result;
	}

	result = psa_iat_client_init(rpc_session);

	if (result) {
		EMSG("psa_iat_client_init failed\n");
		return result;
	}

	provision_result = attest_provision_client_init(rpc_session);

	/* If external IAK is used this call can fail */
	if (provision_result)
		EMSG(
			"attest_provision_client_init failed. Are you using external IAK key?\n");

	return result;
}

LIBPSATS_EXPORTED void libpsats_deinit_attestation_context(void)
{
	psa_iat_client_deinit();
	attest_provision_client_deinit();

	if (attestation_service_context && rpc_session) {
		service_context_close(attestation_service_context, rpc_session);
		rpc_session = NULL;
	}

	if (attestation_service_context) {
		service_context_relinquish(attestation_service_context);
		attestation_service_context = NULL;
	}
}
