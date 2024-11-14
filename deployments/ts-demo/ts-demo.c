/*
 * Copyright (c) 2020-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <app/ts-demo/ts-demo.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "libpsats.h"

int main(int argc, char *argv[])
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	const char *service_name = "sn:trustedfirmware.org:crypto:0";

	if (argc > 1)
		service_name = argv[1];

	psa_status = libpsats_init_crypto_context(service_name);

	if (psa_status) {
		printf("libpsats_init_crypto_context failed: %d\n", psa_status);
		return psa_status;
	}

	psa_status = run_ts_demo(true);

	libpsats_deinit_its_context();

	if (psa_status)
		printf("\n*** ts-demo failed ***\n\n");
	else
		printf("\n*** ts-demo was successful ***\n\n");

	return psa_status;
}
