/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "uefi_variable_store_test.h"

#include "mock/uefi_variable_store_test_if.h"

#include <service/locator/sp/ffa/spffa_service_context.h>

#include <functional>
#include <vector>

TEST_GROUP_BASE(uefi_smm_variable_store_sync_variable_index_set_extended, uefi_smm_variable_store) {
    TEST_SETUP() {
        uefi_smm_variable_store::setup();

        context.active_variable_index_uid = SMM_VARIABLE_INDEX_STORAGE_A_UID;
    }
};

void variable_index_dump_dirty(struct uefi_variable_store& context, psa_status_t psa_status, uint64_t expected_active_variable_index) {
    bool any_dirty = true;

    expect_variable_index_dump(nullptr, &any_dirty, EFI_SUCCESS);

    auto status = sync_variable_index(&context);
    CHECK_EQUAL(psa_to_efi_storage_status(psa_status), status);
    CHECK_EQUAL(expected_active_variable_index, context.active_variable_index_uid);
}

TEST(uefi_smm_variable_store_sync_variable_index_set_extended, variable_index_dump_dirty_setting_remove_error) {
    psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;

    expect_remove(psa_status);

    variable_index_dump_dirty(context, psa_status, SMM_VARIABLE_INDEX_STORAGE_A_UID);
}

TEST(uefi_smm_variable_store_sync_variable_index_set_extended, variable_index_dump_dirty_setting_create_error) {
    psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;

    expect_remove(PSA_SUCCESS);
    expect_create(psa_status);

    variable_index_dump_dirty(context, psa_status, SMM_VARIABLE_INDEX_STORAGE_A_UID);
}

TEST(uefi_smm_variable_store_sync_variable_index_set_extended, variable_index_dump_dirty_setting_does_not_exist_create_error) {
    psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;

    psa_status = PSA_ERROR_GENERIC_ERROR;
    expect_remove(PSA_ERROR_DOES_NOT_EXIST);
    expect_create(psa_status);

    variable_index_dump_dirty(context, psa_status, SMM_VARIABLE_INDEX_STORAGE_A_UID);
}

TEST(uefi_smm_variable_store_sync_variable_index_set_extended, variable_index_dump_dirty_setting_set_extended_error) {
    psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;

    expect_remove(PSA_SUCCESS);
    expect_create(PSA_SUCCESS);
    expect_set_extended(psa_status);

    variable_index_dump_dirty(context, psa_status, SMM_VARIABLE_INDEX_STORAGE_A_UID);
}

TEST(uefi_smm_variable_store_sync_variable_index_set_extended, variable_index_dump_dirty) {
    psa_status_t psa_status = PSA_SUCCESS;

    expect_remove(PSA_SUCCESS);
    expect_create(PSA_SUCCESS);
    expect_set_extended(PSA_SUCCESS);
    expect_variable_index_confirm_write(&context.variable_index);

    variable_index_dump_dirty(context, psa_status, SMM_VARIABLE_INDEX_STORAGE_B_UID);
}

TEST(uefi_smm_variable_store_sync_variable_index_set_extended, variable_index_dump_dirty_set_extended_more_calls) {
    bool any_dirty = true;

    size_t length = (RPC_CALLER_SESSION_SHARED_MEMORY_SIZE * 3) / 2;

    expect_variable_index_dump(&length, &any_dirty, EFI_SUCCESS);

    expect_remove(PSA_SUCCESS);
    expect_create(PSA_SUCCESS);
    expect_set_extended_with_length(RPC_CALLER_SESSION_SHARED_MEMORY_SIZE, PSA_SUCCESS);
    expect_set_extended_with_length(RPC_CALLER_SESSION_SHARED_MEMORY_SIZE / 2, PSA_SUCCESS);
    expect_variable_index_confirm_write(&context.variable_index);

    auto status = sync_variable_index(&context);
    CHECK_EQUAL(psa_to_efi_storage_status(PSA_SUCCESS), status);
}

TEST(uefi_smm_variable_store_sync_variable_index_set_extended, variable_index_dump_dirty_set_extended_more_calls_error) {
    bool any_dirty = true;

    size_t length = (RPC_CALLER_SESSION_SHARED_MEMORY_SIZE * 3) / 2;

    expect_variable_index_dump(&length, &any_dirty, EFI_SUCCESS);

    expect_remove(PSA_SUCCESS);
    expect_create(PSA_SUCCESS);
    expect_set_extended_with_length(RPC_CALLER_SESSION_SHARED_MEMORY_SIZE, PSA_SUCCESS);
    expect_set_extended_with_length(RPC_CALLER_SESSION_SHARED_MEMORY_SIZE / 2, PSA_ERROR_GENERIC_ERROR);

    auto status = sync_variable_index(&context);
    CHECK_EQUAL(psa_to_efi_storage_status(PSA_ERROR_GENERIC_ERROR), status);
}
