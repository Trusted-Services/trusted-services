/*
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <CppUTest/TestHarness.h>
#include <service/secure_storage/frontend/psa/its/its_frontend.h>
#include <service/secure_storage/frontend/psa/its/test/its_api_tests.h>
#include <service/secure_storage/frontend/psa/ps/ps_frontend.h>
#include <service/secure_storage/frontend/psa/ps/test/ps_api_tests.h>
#include <service/secure_storage/backend/secure_flash_store/secure_flash_store.h>
#include <service/secure_storage/backend/secure_flash_store/flash/ram/sfs_flash_ram.h>

/**
 * Tests the secure flash store with a ram flash driver.
 */
TEST_GROUP(SfsRamTests)
{
    void setup()
    {
        struct storage_backend *storage_backend = sfs_init(sfs_flash_ram_instance());

        psa_its_frontend_init(storage_backend);
        psa_ps_frontend_init(storage_backend);
    }
};

TEST(SfsRamTests, itsStoreNewItem)
{
    its_api_tests::storeNewItem();
}

TEST(SfsRamTests, itsStorageLimitTest)
{
    /* This value should match CONFIG_SFS_MIN_FLASH_BLOCK_SIZE */
    its_api_tests::storageLimitTest(20480);
}

TEST(SfsRamTests, Set)
{
    ps_api_tests::set();
}

TEST(SfsRamTests, psCreateAndSetExtended)
{
    ps_api_tests::createAndSetExtended();
}
