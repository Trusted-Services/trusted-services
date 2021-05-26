/*
 * Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstring>
#include <string>
#include <vector>
#include <service/attestation/client/psa/iat_client.h>
#include <service/attestation/client/provision/attest_provision_client.h>
#include <protocols/rpc/common/packed-c/encoding.h>
#include <service_locator.h>
#include <psa/crypto.h>
#include <psa/initial_attestation.h>
#include <provision/attest_provision.h>
#include <qcbor/qcbor_spiffy_decode.h>
#include <t_cose/t_cose_sign1_verify.h>

#define IAK_KEY_BITS    (256)

static bool fetch_and_verify(std::vector<uint8_t> &report, std::string &error_msg);
static bool fetch_iak_public_key(psa_key_handle_t &iak_handle, std::string &error_msg);
static bool verify_token(std::vector<uint8_t> &report, const uint8_t *token, size_t token_len,
    psa_key_handle_t iak_handle, std::string &error_msg);

bool fetch_attest_report(std::vector<uint8_t> &report, std::string &error_msg)
{
    bool success = false;
    rpc_session_handle rpc_session_handle = NULL;
    struct service_context *attest_service_context = NULL;
    int status;

    attest_service_context =
        service_locator_query("sn:trustedfirmware.org:attestation:0", &status);

    if (attest_service_context) {

        struct rpc_caller *caller = NULL;
        rpc_session_handle =
            service_context_open(attest_service_context, TS_RPC_ENCODING_PACKED_C, &caller);

        if (rpc_session_handle) {

            psa_iat_client_init(caller);
            attest_provision_client_init(caller);

            success = fetch_and_verify(report, error_msg);
        }
        else {

            error_msg = "Failed to open RPC session";
        }
    }
    else {

        error_msg = "Failed to discover attestation service provider";
    }

    /* Clean-up context */
    psa_iat_client_deinit();
    attest_provision_client_deinit();
    service_context_close(attest_service_context, rpc_session_handle);
    service_context_relinquish(attest_service_context);

    return success;
}

static bool fetch_and_verify(std::vector<uint8_t> &report, std::string &error_msg)
{
    bool success = false;
    uint8_t token_buf[PSA_INITIAL_ATTEST_MAX_TOKEN_SIZE];
    uint8_t challenge[PSA_INITIAL_ATTEST_CHALLENGE_SIZE_32];
    psa_key_handle_t iak_handle;
    int status;

    if (!fetch_iak_public_key(iak_handle, error_msg)) {

        return false;
    }

    status = psa_generate_random(challenge, sizeof(challenge));

    if (status != PSA_SUCCESS) {

        error_msg = "Failed to generate challenge";
        return false;
    }

    size_t token_size;

    status =
        psa_initial_attest_get_token(challenge, sizeof(challenge),
            token_buf, sizeof(token_buf), &token_size);

    if (status == PSA_SUCCESS) {

        success = verify_token(report, token_buf, token_size, iak_handle, error_msg);
    }
    else {

        error_msg = "Failed to fetch attestation token";
    }

    return success;
}

static bool fetch_iak_public_key(psa_key_handle_t &iak_handle, std::string &error_msg)
{
    size_t iak_pub_key_len = 0;
    uint8_t iak_pub_key_buf[PSA_KEY_EXPORT_MAX_SIZE(
        PSA_KEY_TYPE_PUBLIC_KEY_OF_KEY_PAIR(
            PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_CURVE_SECP256R1)),
            IAK_KEY_BITS)];

    int status = attest_provision_export_iak_public_key(iak_pub_key_buf,
        sizeof(iak_pub_key_buf), &iak_pub_key_len);

    if (status == PSA_SUCCESS) {

        psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

        psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
        psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_VERIFY_HASH);

        psa_set_key_algorithm(&attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
        psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_CURVE_SECP256R1));
        psa_set_key_bits(&attributes, IAK_KEY_BITS);

        status = psa_import_key(&attributes, iak_pub_key_buf, iak_pub_key_len, &iak_handle);

        if (status != PSA_SUCCESS) {

            printf("psa_import_key status: %d\n", status);
            error_msg = "Failed to set-up IAK for verify";
        }

        psa_reset_key_attributes(&attributes);
    }
    else {

        error_msg = "Failed to export IAK public key";
    }

    return (status == PSA_SUCCESS);
}

static bool verify_token(std::vector<uint8_t> &report, const uint8_t *token, size_t token_len,
    psa_key_handle_t iak_handle, std::string &error_msg)
{
    struct t_cose_sign1_verify_ctx verify_ctx;
    struct t_cose_key key_pair;

    key_pair.k.key_handle = iak_handle;
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
    }
    else {

        error_msg = "Attestation token failed to verify";
    }

    return (status == T_COSE_SUCCESS);
}
