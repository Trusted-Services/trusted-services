/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "../psa_api_test_common.h"
#include "libpsats.h"
#include "trace.h"

psa_status_t test_setup(const char *service_name_crypto, const char *service_name_iat,
			const char *service_name_ps, const char *service_name_its)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;

	if (!service_name_crypto)
		service_name_crypto = "sn:trustedfirmware.org:crypto:0";

	if (!service_name_iat)
		service_name_iat = "sn:trustedfirmware.org:attestation:0";

	psa_status = libpsats_init_crypto_context(service_name_crypto);
	if (psa_status) {
		EMSG("libpsats_init_crypto_context failed: %d\n", psa_status);
		return psa_status;
	}

	psa_status = libpsats_init_attestation_context(service_name_iat);
	if (psa_status) {
		EMSG("libpsats_init_attestation_context failed: %d\n", psa_status);
		return psa_status;
	}

	psa_status = psa_crypto_init();
	if (psa_status) {
		EMSG("psa_crypto_init failed: %d\n", psa_status);
		return psa_status;
	}

	return PSA_SUCCESS;
}

void test_teardown(void)
{
	libpsats_deinit_crypto_context();
	libpsats_deinit_attestation_context();
}
