/*
 * Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstring>
#include <provision/attest_provision.h>
#include <qcbor/qcbor_spiffy_decode.h>
#include <string>
#include <t_cose/t_cose_sign1_verify.h>
#include <vector>

#include "libpsats.h"

static bool fetch_iak_public_key(psa_key_id_t &iak_id, std::string &error_msg);
static bool verify_token(std::vector<uint8_t> &report, const uint8_t *token, size_t token_len,
			 psa_key_id_t iak_id, std::string &error_msg);

bool fetch_and_verify(std::vector<uint8_t> &report, std::string &error_msg)
{
	bool result = true;
	uint8_t token_buf[PSA_INITIAL_ATTEST_MAX_TOKEN_SIZE];
	uint8_t challenge[PSA_INITIAL_ATTEST_CHALLENGE_SIZE_32];
	psa_key_id_t iak_id;
	int status;

	if (!fetch_iak_public_key(iak_id, error_msg)) {
		return false;
	}

	status = psa_generate_random(challenge, sizeof(challenge));

	if (status != PSA_SUCCESS) {
		error_msg = "Failed to generate challenge";
		result = false;
	}

	if (result) {
		size_t token_size;

		status = psa_initial_attest_get_token(challenge, sizeof(challenge), token_buf,
						      sizeof(token_buf), &token_size);

		if (status == PSA_SUCCESS) {
			result = verify_token(report, token_buf, token_size, iak_id, error_msg);
		} else {
			error_msg = "Failed to fetch attestation token";
		}
	}

	psa_destroy_key(iak_id);

	return result;
}

static bool fetch_iak_public_key(psa_key_id_t &iak_id, std::string &error_msg)
{
	size_t iak_pub_key_len = 0;
	uint8_t iak_pub_key_buf[PSA_EXPORT_PUBLIC_KEY_MAX_SIZE];

	int status = attest_provision_export_iak_public_key(
		iak_pub_key_buf, sizeof(iak_pub_key_buf), &iak_pub_key_len);

	if (status == PSA_SUCCESS) {
		psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

		psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
		psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_VERIFY_HASH);

		psa_set_key_algorithm(&attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
		psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
		psa_set_key_bits(&attributes, 256);

		status = psa_import_key(&attributes, iak_pub_key_buf, iak_pub_key_len, &iak_id);

		if (status != PSA_SUCCESS) {
			printf("psa_import_key status: %d\n", status);
			error_msg = "Failed to set-up IAK for verify";
		}

		psa_reset_key_attributes(&attributes);
	} else {
		error_msg = "Failed to export IAK public key";
	}

	return (status == PSA_SUCCESS);
}

static bool verify_token(std::vector<uint8_t> &report, const uint8_t *token, size_t token_len,
			 psa_key_id_t iak_id, std::string &error_msg)
{
	struct t_cose_sign1_verify_ctx verify_ctx;
	struct t_cose_key key_pair;

	key_pair.k.key_handle = iak_id;
	key_pair.crypto_lib = T_COSE_CRYPTO_LIB_PSA;
	UsefulBufC signed_cose;
	UsefulBufC report_body;

	signed_cose.ptr = token;
	signed_cose.len = token_len;

	report_body.ptr = NULL;
	report_body.len = 0;

	t_cose_sign1_verify_init(&verify_ctx, 0);
	t_cose_sign1_set_verification_key(&verify_ctx, key_pair);

	int status = t_cose_sign1_verify(&verify_ctx, signed_cose, &report_body, NULL);

	if (status == T_COSE_SUCCESS) {
		report.resize(report_body.len);
		memcpy(report.data(), report_body.ptr, report_body.len);
	} else {
		error_msg = "Attestation token failed to verify";
	}

	return (status == T_COSE_SUCCESS);
}
