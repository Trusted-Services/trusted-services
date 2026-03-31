/*
 * Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cstring>
#include <common/endian/le.h>
#include <common/endian/be.h>
#include <CppUTest/TestHarness.h>

TEST_GROUP(EndianTests)
{

};

TEST(EndianTests, le8)
{
    const uint8_t test_vector[] = {0x00, 0x01, 0x02, 0x03, 0x04};
    uint8_t store_result[sizeof(test_vector)];

    memcpy(store_result, test_vector, sizeof(test_vector));

    UNSIGNED_LONGS_EQUAL(0x03, load_u8_le(test_vector, 3));

    store_u8_le(store_result, 2, 0x02);
    MEMCMP_EQUAL(test_vector, store_result, sizeof(test_vector));
}

TEST(EndianTests, le16)
{
    const uint8_t test_vector[] = {0x00, 0x01, 0x02, 0x03, 0x04};
    uint8_t store_result[sizeof(test_vector)];

    memcpy(store_result, test_vector, sizeof(test_vector));

    UNSIGNED_LONGS_EQUAL(0x0201, load_u16_le(test_vector, 1));

    store_u16_le(store_result, 1, 0x0201);
    MEMCMP_EQUAL(test_vector, store_result, sizeof(test_vector));
}

TEST(EndianTests, le32)
{
    const uint8_t test_vector[] = {0x00, 0x01, 0x02, 0x03, 0x04};
    uint8_t store_result[sizeof(test_vector)];

    memcpy(store_result, test_vector, sizeof(test_vector));

    UNSIGNED_LONGS_EQUAL(0x04030201, load_u32_le(test_vector, 1));

    store_u32_le(store_result, 1, 0x04030201);
    MEMCMP_EQUAL(test_vector, store_result, sizeof(test_vector));
}

TEST(EndianTests, le64)
{
    const uint8_t test_vector[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    uint8_t store_result[sizeof(test_vector)];

    memcpy(store_result, test_vector, sizeof(test_vector));

    UNSIGNED_LONGS_EQUAL(0x0807060504030201, load_u64_le(test_vector, 1));

    store_u64_le(store_result, 1, 0x0807060504030201);
    MEMCMP_EQUAL(test_vector, store_result, sizeof(test_vector));
}

TEST(EndianTests, be8)
{
    const uint8_t test_vector[] = {0x00, 0x01, 0x02, 0x03, 0x04};
    uint8_t store_result[sizeof(test_vector)];

    memcpy(store_result, test_vector, sizeof(test_vector));

    UNSIGNED_LONGS_EQUAL(0x03, load_u8_be(test_vector, 3));

    store_u8_be(store_result, 2, 0x02);
    MEMCMP_EQUAL(test_vector, store_result, sizeof(test_vector));
}

TEST(EndianTests, be16)
{
    const uint8_t test_vector[] = {0x00, 0x01, 0x02, 0x03, 0x04};
    uint8_t store_result[sizeof(test_vector)];

    memcpy(store_result, test_vector, sizeof(test_vector));

    UNSIGNED_LONGS_EQUAL(0x0102, load_u16_be(test_vector, 1));

    store_u16_be(store_result, 1, 0x0102);
    MEMCMP_EQUAL(test_vector, store_result, sizeof(test_vector));
}

TEST(EndianTests, be32)
{
    const uint8_t test_vector[] = {0x00, 0x01, 0x02, 0x03, 0x04};
    uint8_t store_result[sizeof(test_vector)];

    memcpy(store_result, test_vector, sizeof(test_vector));

    UNSIGNED_LONGS_EQUAL(0x01020304, load_u32_be(test_vector, 1));

    store_u32_be(store_result, 1, 0x01020304);
    MEMCMP_EQUAL(test_vector, store_result, sizeof(test_vector));
}

TEST(EndianTests, be64)
{
    const uint8_t test_vector[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    uint8_t store_result[sizeof(test_vector)];

    memcpy(store_result, test_vector, sizeof(test_vector));

    UNSIGNED_LONGS_EQUAL(0x0102030405060708, load_u64_be(test_vector, 1));

    store_u64_be(store_result, 1, 0x0102030405060708);
    MEMCMP_EQUAL(test_vector, store_result, sizeof(test_vector));
}
