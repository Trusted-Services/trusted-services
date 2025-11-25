/*
 * Copyright (c) 2021-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * A attestation reporter for psa ipc
 */

#include <stddef.h>
#include <psa/error.h>
#include <service/attestation/reporter/attest_report.h>
#include <psa/initial_attestation.h>
#include <trace.h>
#include <stdlib.h>

int attest_report_create(int32_t client_id, const uint8_t *auth_challenge_data,
			 size_t auth_challenge_len, const uint8_t **report,
			 size_t *report_len)
{
	psa_status_t ret;
	size_t token_buf_size = 0;
	size_t token_size = 0;
	uint8_t *token_buf = NULL;

	ret = psa_initial_attest_get_token_size(auth_challenge_len, &token_buf_size);
	if (ret != PSA_SUCCESS) {
		EMSG("psa_initial_attest_get_token_size() returned %i", ret);
		goto err;
	}

	token_buf = malloc(token_buf_size);
	if (!token_buf) {
		EMSG("Memory allocation failed.");
		ret = PSA_ERROR_INSUFFICIENT_MEMORY;
		goto err;
	}

	ret = psa_initial_attest_get_token(auth_challenge_data,
					   auth_challenge_len, token_buf,
					   token_buf_size, &token_size);
	if (ret != PSA_SUCCESS) {
		EMSG("psa_initial_attest_get_token() returned %i", ret);
		goto err_get_token;
	}

	*report = token_buf;
	*report_len = token_size;

	return PSA_SUCCESS;

err_get_token:
	free(token_buf);
err:
	*report = NULL;
	*report_len = 0;
	return ret;
}

void attest_report_destroy(const uint8_t *report)
{
	free((void *)report);
}
