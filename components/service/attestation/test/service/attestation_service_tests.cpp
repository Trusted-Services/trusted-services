/*
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <limits.h>
#include <string.h>
#include <service/attestation/client/psa/iat_client.h>
#include <protocols/rpc/common/packed-c/encoding.h>
#include <service_locator.h>
#include <psa/initial_attestation.h>
#include <service/crypto/client/psa/psa_crypto_client.h>
#include <CppUTest/TestHarness.h>

/*
 * Service-level tests for the attestation service.
 */
TEST_GROUP(AttestationServiceTests)
{
    void setup()
    {
        m_rpc_session = NULL;
        m_attest_service_context = NULL;

        service_locator_init();

        m_attest_service_context =
            service_locator_query("sn:trustedfirmware.org:attestation:0");
        CHECK_TRUE(m_attest_service_context);

        m_rpc_session =
            service_context_open(m_attest_service_context);
        CHECK_TRUE(m_rpc_session);

        psa_iat_client_init(m_rpc_session);

	open_crypto_session();
    }

    void teardown()
    {
        psa_iat_client_deinit();

	if (m_attest_service_context) {
		if (m_rpc_session) {
			service_context_close(m_attest_service_context, m_rpc_session);
			m_rpc_session = NULL;
		}

		service_context_relinquish(m_attest_service_context);
		m_attest_service_context = NULL;
	}
	close_crypto_session();
    }

    void open_crypto_session()
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

    void close_crypto_session()
    {
        psa_crypto_client_deinit();

        if (m_crypto_service_context && m_crypto_session) {
            service_context_close(m_crypto_service_context, m_crypto_session);
            m_crypto_session = NULL;

            service_context_relinquish(m_crypto_service_context);
            m_crypto_service_context = NULL;
        }
    }

    struct rpc_caller_session *m_rpc_session;
    struct service_context *m_attest_service_context;
    struct service_context *m_crypto_service_context;
    struct rpc_caller_session *m_crypto_session;
};

TEST(AttestationServiceTests, checkTokenSize)
{
    uint8_t token_buf[PSA_INITIAL_ATTEST_MAX_TOKEN_SIZE];
    uint8_t challenge[PSA_INITIAL_ATTEST_CHALLENGE_SIZE_64];

    memset(challenge, 0x87, sizeof(challenge));

    /* Check that the get_token_size operation returns the same size
     * as theh get_token operation.
     */
    psa_status_t status;
    size_t reported_token_size = 0;
    size_t actual_token_size = 0;

    status = psa_initial_attest_get_token_size(
        sizeof(challenge),
        &reported_token_size);

    LONGS_EQUAL(PSA_SUCCESS, status);
    CHECK_TRUE(reported_token_size);

    status = psa_initial_attest_get_token(
        challenge, sizeof(challenge),
        token_buf, sizeof(token_buf),
        &actual_token_size);

    LONGS_EQUAL(PSA_SUCCESS, status);
    UNSIGNED_LONGS_EQUAL(reported_token_size, actual_token_size);
}

TEST(AttestationServiceTests, invalidChallengeLen)
{
    uint8_t token_buf[PSA_INITIAL_ATTEST_MAX_TOKEN_SIZE];
    uint8_t challenge[PSA_INITIAL_ATTEST_CHALLENGE_SIZE_64];

    memset(challenge, 0x87, sizeof(challenge));

    /* Check that invalid challenge lengths are rejected cleanly */
    psa_status_t status;
    size_t challenge_len;
    size_t token_size = 0;

    /* Zero length challenge */
    challenge_len = 0;
    status = psa_initial_attest_get_token_size(
        challenge_len,
        &token_size);
    LONGS_EQUAL(PSA_ERROR_INVALID_ARGUMENT, status);

    status = psa_initial_attest_get_token(
        challenge, challenge_len,
        token_buf, sizeof(token_buf),
        &token_size);
    LONGS_EQUAL(PSA_ERROR_INVALID_ARGUMENT, status);

    /* Extra large challenge */
    challenge_len = UINT32_MAX;
    status = psa_initial_attest_get_token_size(
        challenge_len,
        &token_size);
    LONGS_EQUAL(PSA_ERROR_INVALID_ARGUMENT, status);

    status = psa_initial_attest_get_token(
        challenge, challenge_len,
        token_buf, sizeof(token_buf),
        &token_size);
    LONGS_EQUAL(PSA_ERROR_INVALID_ARGUMENT, status);

    /* Slightly large challenge */
    challenge_len = PSA_INITIAL_ATTEST_CHALLENGE_SIZE_64 + 1;
    status = psa_initial_attest_get_token_size(
        challenge_len,
        &token_size);
    LONGS_EQUAL(PSA_ERROR_INVALID_ARGUMENT, status);

    status = psa_initial_attest_get_token(
        challenge, challenge_len,
        token_buf, sizeof(token_buf),
        &token_size);
    LONGS_EQUAL(PSA_ERROR_INVALID_ARGUMENT, status);
}

TEST(AttestationServiceTests, repeatedOperation)
{
    uint8_t token_buf[PSA_INITIAL_ATTEST_MAX_TOKEN_SIZE];
    uint8_t challenge[PSA_INITIAL_ATTEST_CHALLENGE_SIZE_48];

    memset(challenge, 0x61, sizeof(challenge));

    /* Check reports can be requested repeatedly without
     * resource exhausation due to a memory leak.
     */
    for (int i = 0; i < 100; ++i) {

        psa_status_t status;
        size_t reported_token_size = 0;
        size_t actual_token_size = 0;

        status = psa_initial_attest_get_token_size(
            sizeof(challenge),
            &reported_token_size);

        LONGS_EQUAL(PSA_SUCCESS, status);
        CHECK_TRUE(reported_token_size);

        status = psa_initial_attest_get_token(
            challenge, sizeof(challenge),
            token_buf, sizeof(token_buf),
            &actual_token_size);

        LONGS_EQUAL(PSA_SUCCESS, status);
        UNSIGNED_LONGS_EQUAL(reported_token_size, actual_token_size);
    }
}
