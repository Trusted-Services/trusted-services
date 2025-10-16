/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "uefi_variable_store_test.h"

#include "mock/uefi_variable_store_test_if.h"

TEST_GROUP_BASE(uefi_smm_variable_store_sync_variable_index_minimal, uefi_smm_variable_store) {
    TEST_SETUP() {
        uefi_smm_variable_store::setup();

        context.active_variable_index_uid = SMM_VARIABLE_INDEX_STORAGE_A_UID;
    }
};

TEST(uefi_smm_variable_store_sync_variable_index_minimal, variable_index_dump_dirty) {
    bool any_dirty = true;
    psa_status_t psa_status = PSA_SUCCESS;

    expect_variable_index_dump(nullptr, &any_dirty, EFI_SUCCESS);
    expect_set(psa_status);
    expect_variable_index_confirm_write(&context.variable_index);

    auto status = sync_variable_index(&context);
    CHECK_EQUAL(psa_to_efi_storage_status(psa_status), status);
    CHECK_EQUAL(SMM_VARIABLE_INDEX_STORAGE_B_UID, context.active_variable_index_uid);
}
