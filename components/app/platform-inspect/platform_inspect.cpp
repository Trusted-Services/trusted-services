/*
 * Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstdint>
#include <cstdio>
#include <service/attestation/reporter/dump/pretty/pretty_report_dump.h>
#include <string>
#include <vector>

#include "attest_report_fetcher.h"
#include "libpsats.h"

int main(void)
{
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	int rval = -1;
	std::string error_msg;
	std::vector<uint8_t> attest_report;

	psa_status = libpsats_init_crypto_context("sn:trustedfirmware.org:crypto:0");
	if (psa_status) {
		printf("libpsats_init_crypto_context failed: %d\n", psa_status);
		goto cleanup;
	}

	psa_status = libpsats_init_attestation_context("sn:trustedfirmware.org:attestation:0");
	if (psa_status) {
		printf("libpsats_init_crypto_context failed: %d\n", psa_status);
		goto cleanup;
	}

	psa_status = psa_crypto_init();
	if (psa_status) {
		printf("psa_crypto_init failed: %d\n", psa_status);
		goto cleanup;
	}

	/* Fetch platform info */
	if (fetch_and_verify(attest_report, error_msg))
		rval = pretty_report_dump(attest_report.data(), attest_report.size());
	else
		printf("%s\n", error_msg.c_str());

cleanup:
	libpsats_deinit_crypto_context();
	libpsats_deinit_attestation_context();

	return rval;
}
