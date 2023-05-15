/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "../psa_api_test_common.h"
#include "libpsa.h"
#include "trace.h"

psa_status_t test_setup(const char *service_name_crypto, const char *service_name_iat,
			const char *service_name_ps, const char *service_name_its)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;

	if (!service_name_its)
		service_name_its = "sn:trustedfirmware.org:internal-trusted-storage:0";

	psa_status = libpsa_init_its_context(service_name_its);
	if (psa_status) {
		EMSG("libpsa_init_its_context failed: %d\n", psa_status);
		return psa_status;
	}

	return psa_status;
}

void test_teardown(void)
{
	libpsa_deinit_its_context();
}
