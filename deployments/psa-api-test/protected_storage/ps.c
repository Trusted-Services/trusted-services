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

	if (!service_name_ps)
		service_name_ps = "sn:trustedfirmware.org:protected-storage:0";

	psa_status = libpsats_init_ps_context(service_name_ps);
	if (psa_status) {
		EMSG("libpsats_init_ps_context failed: %d\n", psa_status);
		return psa_status;
	}

	return psa_status;
}

void test_teardown(void)
{
	libpsats_deinit_ps_context();
}
