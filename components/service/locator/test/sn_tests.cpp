/*
 * Copyright (c) 2020-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <service/locator/service_name.h>
#include <CppUTest/TestHarness.h>
#include <common/uuid/uuid.h>
#include <string.h>

TEST_GROUP(ServiceNameTests) {

};

TEST(ServiceNameTests, checkValidServiceNames) {

    const char *sn1 = "sn:trustedfirmware.org:crypto:0";
    CHECK(sn_is_valid(sn1));

    const char *sn2 = "sn:trustedfirmware.org:secure-storage.1.0:0";
    CHECK(sn_is_valid(sn2));

    const char *sn3 = "urn:sn:trustedfirmware.org:tpm:3";
    CHECK(sn_is_valid(sn3));

    const char *sn4 = "sn:ffa:d9df52d5-16a2-4bb2-9aa4-d26d3b84e8c0:0";
    CHECK(sn_is_valid(sn4));

    const char *sn5 = "sn:ffa:d9df52d5-16a2-4bb2-9aa4-d26d3b84e8c0";
    CHECK(sn_is_valid(sn5));
}

TEST(ServiceNameTests, checkInvalidServiceNames) {

    const char *sn1 = "sn:trustedfirmware.org";
    CHECK(!sn_is_valid(sn1));

    const char *sn2 = "trustedfirmware.org:secure-storage.1.0:0";
    CHECK(!sn_is_valid(sn2));

    const char *sn3 = "urn:trustedfirmware.org:tpm";
    CHECK(!sn_is_valid(sn3));

    const char *sn4 = "sn:";
    CHECK(!sn_is_valid(sn4));

    const char *sn5 = "sn";
    CHECK(!sn_is_valid(sn5));
}

TEST(ServiceNameTests, checkFields) {

    const char *sn1 = "sn:trustedfirmware.org:crypto:2";
    CHECK(sn_check_authority(sn1, "trustedfirmware.org"));
    CHECK(!sn_check_authority(sn1, "ffa"));
    CHECK(sn_check_service(sn1, "crypto"));
    CHECK_EQUAL(2, sn_get_service_instance(sn1));

    const char *sn2 = "sn:trustedfirmware.org:secure-storage.1.0:0";
    CHECK(sn_check_authority(sn2, "trustedfirmware.org"));
    CHECK(sn_check_service(sn2, "secure-storage.1.0"));
    CHECK(sn_check_service(sn2, "secure-storage"));
    CHECK_EQUAL(0, sn_get_service_instance(sn2));

    const char *sn3 = "sn:ffa:d9df52d5-16a2-4bb2-9aa4-d26d3b84e8c0:77";
    CHECK(sn_check_authority(sn3, "ffa"));
    CHECK(sn_check_service(sn3, "d9df52d5-16a2-4bb2-9aa4-d26d3b84e8c0"));
    CHECK_EQUAL(77, sn_get_service_instance(sn3));

    /* Check instance defaults to zero */
    const char *sn4 = "sn:ffa:d9df52d5-16a2-4bb2-9aa4-d26d3b84e8c0";
    CHECK(sn_is_valid(sn4));
    CHECK_EQUAL(0, sn_get_service_instance(sn4));

    /* Check instance is zero when contains non-number characters */
    const char *sn5 = "sn:trustedfirmware.org:secure-storage.1.0:0xbeef9";
    CHECK(sn_is_valid(sn5));
    CHECK_EQUAL(0, sn_get_service_instance(sn5));

    /* Checks with invalid service names */
    const char *sn6 = "trustedfirmware.org:secure-storage";
    CHECK(!sn_check_authority(sn6, "trustedfirmware.org"));
    CHECK(!sn_check_service(sn6, "secure-storage"));
    CHECK_EQUAL(0, sn_get_service_instance(sn6));

    const char *sn7 = "sn::secure-storage";
    CHECK(!sn_check_authority(sn7, ""));

    const char *sn8 = "sn:trustedfirmware.org:";
    CHECK(!sn_check_service(sn8, ""));
}

TEST(ServiceNameTests, readService) {

    char buf[UUID_CANONICAL_FORM_LEN + 1];

    const char *sn1 = "sn:trustedfirmware.org:crypto:2";
    CHECK_EQUAL(strlen("crypto"), sn_read_service(sn1, buf, sizeof(buf)));
    CHECK(memcmp(buf, "crypto", strlen("crypto") + 1) == 0);
    CHECK_EQUAL(strlen("crypto"), strlen(buf));

    const char *sn2 = "sn:trustedfirmware.org:crypto.1.7.0:2";
    CHECK_EQUAL(strlen("crypto.1.7.0"), sn_read_service(sn2, buf, sizeof(buf)));
    CHECK(memcmp(buf, "crypto.1.7.0", strlen("crypto.1.7.0") + 1) == 0);
    CHECK_EQUAL(strlen("crypto.1.7.0"), strlen(buf));

    const char *sn3 = "sn:ffa:d9df52d5-16a2-4bb2-9aa4-d26d3b84e8c0:7";
    CHECK_EQUAL(UUID_CANONICAL_FORM_LEN, sn_read_service(sn3, buf, sizeof(buf)));
    CHECK(memcmp(buf, "d9df52d5-16a2-4bb2-9aa4-d26d3b84e8c0", UUID_CANONICAL_FORM_LEN + 1) == 0);
    CHECK_EQUAL(UUID_CANONICAL_FORM_LEN, strlen(buf));

    const char *sn4 = "sn:ffa:d9df52d5-16a2-4bb2-9aa4-d26d3b84e8c0";
    CHECK_EQUAL(UUID_CANONICAL_FORM_LEN, sn_read_service(sn4, buf, sizeof(buf)));
    CHECK(memcmp(buf, "d9df52d5-16a2-4bb2-9aa4-d26d3b84e8c0", UUID_CANONICAL_FORM_LEN + 1) == 0);
    CHECK_EQUAL(UUID_CANONICAL_FORM_LEN, strlen(buf));

    /* Checks with invalid service name */
    const char *sn5 = "trustedfirmware.org:crypto.1.7.0";
    CHECK_EQUAL(0, sn_read_service(sn5, buf, sizeof(buf)));

    /* Check when buffer is smaller than service name */
    static size_t SIZE = 4;
    char buf2[SIZE];
    const char *sn6 = "sn:trustedfirmware.org:crypto.1.7.0:2";
    CHECK_EQUAL(0, sn_read_service(sn6, buf2, SIZE));
}
