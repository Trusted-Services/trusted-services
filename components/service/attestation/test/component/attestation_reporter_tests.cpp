/*
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <psa/error.h>
#include <psa/lifecycle.h>
#include <qcbor/qcbor_spiffy_decode.h>
#include <t_cose/t_cose_sign1_verify.h>
#include <service/attestation/claims/claims_register.h>
#include <service/attestation/claims/sources/event_log/event_log_claim_source.h>
#include <service/attestation/claims/sources/event_log/mock/mock_event_log.h>
#include <service/attestation/claims/sources/boot_seed_generator/boot_seed_generator.h>
#include <service/attestation/claims/sources/null_lifecycle/null_lifecycle_claim_source.h>
#include <service/attestation/claims/sources/instance_id/instance_id_claim_source.h>
#include <service/attestation/reporter/attest_report.h>
#include <service/attestation/reporter/dump/raw/raw_report_dump.h>
#include <service/attestation/key_mngr/attest_key_mngr.h>
#include <service/attestation/key_mngr/local/local_attest_key_mngr.h>
#include <protocols/service/attestation/packed-c/eat.h>
#include <service/crypto/client/psa/psa_crypto_client.h>
#include <service_locator.h>
#include <CppUTest/TestHarness.h>


TEST_GROUP(AttestationReporterTests)
{
    void setup()
    {
        struct claim_source *claim_source;

        report = NULL;
        report_len = 0;
        const struct mock_event_log_info *event_log_info = mock_event_log_list_get_default();


        open_crypto_session();
        local_attest_key_mngr_init(LOCAL_ATTEST_KEY_MNGR_VOLATILE_IAK);

        /* The set of registered claim_sources determines the content
         * of a generated attestation source.  The set and type of
         * claim_sources registered will be deployment specific.
         */
        claims_register_init();

        /* Boot measurement claim source */
        claim_source = event_log_claim_source_init(&event_log_claim_source,
            event_log_info->data, event_log_info->data_length);
        claims_register_add_claim_source(CLAIM_CATEGORY_BOOT_MEASUREMENT, claim_source);

        /* Boot seed claim source */
        claim_source = boot_seed_generator_init(&boot_seed_claim_source);
        claims_register_add_claim_source(CLAIM_CATEGORY_DEVICE, claim_source);

        /* Lifecycle state claim source */
        claim_source = null_lifecycle_claim_source_init(&lifecycle_claim_source);
        claims_register_add_claim_source(CLAIM_CATEGORY_DEVICE, claim_source);

        /* Instance ID claim source */
        claim_source = instance_id_claim_source_init(&instance_id_claim_source);
        claims_register_add_claim_source(CLAIM_CATEGORY_DEVICE, claim_source);
    }

    void teardown()
    {
        attest_report_destroy(report);
        claims_register_deinit();
        local_attest_key_mngr_deinit();
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

    struct service_context *m_crypto_service_context;
    struct rpc_caller_session *m_crypto_session;
    struct event_log_claim_source event_log_claim_source;
    struct boot_seed_generator boot_seed_claim_source;
    struct null_lifecycle_claim_source lifecycle_claim_source;
    struct instance_id_claim_source instance_id_claim_source;
    const uint8_t *report;
    size_t report_len;
};

TEST(AttestationReporterTests, createReport)
{
    int status;

    /* Client inputs */
    int32_t client_id = 0x552791aa;
    const uint8_t auth_challenge[] = {
         1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,
        17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32
    };

    const struct mock_event_log_info *event_log_info = mock_event_log_list_get_at(mock_event_log_list_length() - 1);

    /* Retrieve the IAK id */
    psa_key_id_t iak_id;
    status = attest_key_mngr_get_iak_id(&iak_id);
    LONGS_EQUAL(PSA_SUCCESS, status);

    /* Create a report */
    status = attest_report_create(client_id,
        auth_challenge, sizeof(auth_challenge),
        &report, &report_len);

    /* Expect the operation to succeed and a non-zero length
     * report created.
     */
    LONGS_EQUAL(PSA_SUCCESS, status);
    CHECK_TRUE(report);
    CHECK_TRUE(report_len);

    /* Verify the signature */
    struct t_cose_sign1_verify_ctx verify_ctx;
    struct t_cose_key key_pair;

    key_pair.k.key_handle = iak_id;
    key_pair.crypto_lib = T_COSE_CRYPTO_LIB_PSA;
    UsefulBufC signed_cose;
    UsefulBufC report_body;

    signed_cose.ptr = report;
    signed_cose.len = report_len;

    t_cose_sign1_verify_init(&verify_ctx, 0);
    t_cose_sign1_set_verification_key(&verify_ctx, key_pair);

    status = t_cose_sign1_verify(&verify_ctx, signed_cose, &report_body, NULL);
    LONGS_EQUAL(T_COSE_SUCCESS, status);

    /* Check the report contents */
    QCBORDecodeContext decode_ctx;

    QCBORDecode_Init(&decode_ctx, report_body, QCBOR_DECODE_MODE_NORMAL);
    QCBORDecode_EnterMap(&decode_ctx, NULL);

    /* Check client id */
    int64_t decoded_client_id = 0;
    QCBORDecode_GetInt64InMapN(&decode_ctx,
        EAT_ARM_PSA_CLAIM_ID_CLIENT_ID, &decoded_client_id);

    LONGS_EQUAL(QCBOR_SUCCESS, QCBORDecode_GetError(&decode_ctx));
    LONGS_EQUAL(client_id, decoded_client_id);

    /* Check the auth challenge */
    UsefulBufC auth_challenge_buf;
    auth_challenge_buf.ptr = NULL;
    auth_challenge_buf.len = 0;
    QCBORDecode_GetByteStringInMapN(&decode_ctx,
        EAT_ARM_PSA_CLAIM_ID_CHALLENGE, &auth_challenge_buf);

    LONGS_EQUAL(QCBOR_SUCCESS, QCBORDecode_GetError(&decode_ctx));
    CHECK_TRUE(auth_challenge_buf.ptr);
    UNSIGNED_LONGS_EQUAL(sizeof(auth_challenge), auth_challenge_buf.len);
    MEMCMP_EQUAL(auth_challenge, auth_challenge_buf.ptr, sizeof(auth_challenge));

    /* Check the boot seed */
    UsefulBufC boot_seed_buf;
    boot_seed_buf.ptr = NULL;
    boot_seed_buf.len = 0;
    QCBORDecode_GetByteStringInMapN(&decode_ctx,
        EAT_ARM_PSA_CLAIM_ID_BOOT_SEED, &boot_seed_buf);

    LONGS_EQUAL(QCBOR_SUCCESS, QCBORDecode_GetError(&decode_ctx));
    CHECK_TRUE(boot_seed_buf.ptr);
    UNSIGNED_LONGS_EQUAL(sizeof(boot_seed_claim_source.boot_seed), boot_seed_buf.len);

    /* Check the lifecycle state */
    int64_t decoded_lifecycle_state = 0;
    QCBORDecode_GetInt64InMapN(&decode_ctx,
        EAT_ARM_PSA_CLAIM_ID_SECURITY_LIFECYCLE, &decoded_lifecycle_state);

    LONGS_EQUAL(QCBOR_SUCCESS, QCBORDecode_GetError(&decode_ctx));
    LONGS_EQUAL(PSA_LIFECYCLE_UNKNOWN, decoded_lifecycle_state);

    /* Check the instance ID */
    UsefulBufC instance_id_buf;
    instance_id_buf.ptr = NULL;
    instance_id_buf.len = 0;
    QCBORDecode_GetByteStringInMapN(&decode_ctx,
        EAT_ARM_PSA_CLAIM_ID_INSTANCE_ID, &instance_id_buf);

    LONGS_EQUAL(QCBOR_SUCCESS, QCBORDecode_GetError(&decode_ctx));
    CHECK_TRUE(instance_id_buf.ptr);
    UNSIGNED_LONGS_EQUAL(sizeof(instance_id_claim_source.instance_id), instance_id_buf.len);

    /* Shouldn't expect to see the 'NO_SW_COMPONENTS' claim */
    int64_t no_sw = 0;
    QCBORDecode_GetInt64InMapN(&decode_ctx, EAT_ARM_PSA_CLAIM_ID_NO_SW_COMPONENTS, &no_sw);
    LONGS_EQUAL(QCBOR_ERR_LABEL_NOT_FOUND, QCBORDecode_GetAndResetError(&decode_ctx));
    CHECK_FALSE(no_sw);

    /* Check the sw components */
    QCBORDecode_EnterArrayFromMapN(&decode_ctx, EAT_ARM_PSA_CLAIM_ID_SW_COMPONENTS);
    LONGS_EQUAL(QCBOR_SUCCESS, QCBORDecode_GetError(&decode_ctx));

    /* Iterate over all array members */
    size_t sw_component_count = 0;
    while (true) {

        QCBORDecode_EnterMap(&decode_ctx, NULL);

        if (QCBORDecode_GetAndResetError(&decode_ctx) == QCBOR_SUCCESS) {

            CHECK_TRUE(sw_component_count < event_log_info->n_measurements);

            UsefulBufC property;
            const struct mock_event_log_measurement *measurement =
                mock_event_log_measurement(event_log_info, sw_component_count);

            /* Check measurement id */
             QCBORDecode_GetTextStringInMapN(&decode_ctx,
                    EAT_SW_COMPONENT_CLAIM_ID_MEASUREMENT_TYPE, &property);
            LONGS_EQUAL(QCBOR_SUCCESS, QCBORDecode_GetError(&decode_ctx));
            CHECK_TRUE(property.ptr);
            CHECK_TRUE(property.len);
            MEMCMP_EQUAL(measurement->id, property.ptr, property.len);

            /* Check measurement digest */
            QCBORDecode_GetByteStringInMapN(&decode_ctx,
                    EAT_SW_COMPONENT_CLAIM_ID_MEASUREMENT_VALUE, &property);
            LONGS_EQUAL(QCBOR_SUCCESS, QCBORDecode_GetError(&decode_ctx));
            CHECK_TRUE(property.ptr);
            CHECK_TRUE(property.len);
            MEMCMP_EQUAL(measurement->digest, property.ptr, property.len);

            QCBORDecode_ExitMap(&decode_ctx);

            ++sw_component_count;
        }
        else {
            /* No more sw components */
            break;
        }
    }

    QCBORDecode_ExitArray(&decode_ctx);
    LONGS_EQUAL(QCBOR_SUCCESS, QCBORDecode_GetError(&decode_ctx));

    QCBORError qcbor_error;
    QCBORDecode_ExitMap(&decode_ctx);
    qcbor_error = QCBORDecode_Finish(&decode_ctx);
    LONGS_EQUAL(QCBOR_SUCCESS, qcbor_error);
}
