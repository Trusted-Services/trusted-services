/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "uefi_variable_store_test.h"

#include "mock/uefi_variable_store_test_if.h"

TEST_GROUP_BASE(uefi_smm_variable_store_sync_variable_index, uefi_smm_variable_store) {
    TEST_SETUP() {
        uefi_smm_variable_store::setup();

        context.active_variable_index_uid = SMM_VARIABLE_INDEX_STORAGE_A_UID;
    }
};

TEST(uefi_smm_variable_store_sync_variable_index, variable_index_dump_buffer_too_small) {
    expect_variable_index_dump(nullptr, nullptr, EFI_BUFFER_TOO_SMALL);

    auto status = sync_variable_index(&context);
    CHECK_EQUAL(EFI_BUFFER_TOO_SMALL, status);
    CHECK_EQUAL(SMM_VARIABLE_INDEX_STORAGE_A_UID, context.active_variable_index_uid);
}

TEST(uefi_smm_variable_store_sync_variable_index, variable_index_dump_none_dirty) {
    bool any_dirty = false;

    expect_variable_index_dump(nullptr, &any_dirty, EFI_SUCCESS);

    auto status = sync_variable_index(&context);
    CHECK_EQUAL(psa_to_efi_storage_status(PSA_SUCCESS), status);
    CHECK_EQUAL(SMM_VARIABLE_INDEX_STORAGE_A_UID, context.active_variable_index_uid);
}

TEST(uefi_smm_variable_store_sync_variable_index, variable_index_dump_dirty_no_persistent_store_backend) {
    bool any_dirty = true;
    context.persistent_store.storage_backend = nullptr;

    expect_variable_index_dump(nullptr, &any_dirty, EFI_SUCCESS);

    auto status = sync_variable_index(&context);
    CHECK_EQUAL(EFI_LOAD_ERROR, status);
    CHECK_EQUAL(SMM_VARIABLE_INDEX_STORAGE_A_UID, context.active_variable_index_uid);
}
