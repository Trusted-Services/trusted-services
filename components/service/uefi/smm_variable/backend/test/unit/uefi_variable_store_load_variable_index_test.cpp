/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "uefi_variable_store_test.h"

#include "mock/uefi_variable_store_test_if.h"

TEST_GROUP_BASE(uefi_smm_variable_store_load_variable_index, uefi_smm_variable_store) {
};

TEST(uefi_smm_variable_store_load_variable_index, no_persistent_storage_backend) {
    context.persistent_store.storage_backend = nullptr;

    auto status = load_variable_index(&context);
    CHECK_EQUAL(EFI_LOAD_ERROR, status);
}

TEST(uefi_smm_variable_store_load_variable_index, active_variable_does_not_exist) {
    expect_get_active_variable_uid(PSA_ERROR_DOES_NOT_EXIST, nullptr, nullptr);

    auto status = load_variable_index(&context);
    CHECK_EQUAL(EFI_SUCCESS, status);
}

TEST(uefi_smm_variable_store_load_variable_index, active_variable_storage_failure) {
    expect_get_active_variable_uid(PSA_ERROR_STORAGE_FAILURE, nullptr, nullptr);

    auto status = load_variable_index(&context);
    CHECK_EQUAL(EFI_LOAD_ERROR, status);
}

TEST(uefi_smm_variable_store_load_variable_index, get_info_insufficient_space) {
    expect_get_active_variable_uid(PSA_SUCCESS, nullptr, nullptr);

    psa_storage_info_t info = {
        .size = (uint32_t) ((context.index_sync_buffer_size + 1) & UINT32_MAX)
    };
    expect_get_info(&info, PSA_SUCCESS);

    auto status = load_variable_index(&context);
    CHECK_EQUAL(EFI_LOAD_ERROR, status);
}

TEST(uefi_smm_variable_store_load_variable_index, get_buffer_too_small) {
    expect_get_active_variable_uid(PSA_SUCCESS, nullptr, nullptr);

    psa_storage_info_t info = {
        .size = (uint32_t) context.index_sync_buffer_size
    };
    expect_get_info(&info, PSA_SUCCESS);
    expect_get(nullptr, PSA_ERROR_BUFFER_TOO_SMALL);

    auto status = load_variable_index(&context);
    CHECK_EQUAL(EFI_LOAD_ERROR, status);
}

TEST(uefi_smm_variable_store_load_variable_index, success) {
    expect_get_active_variable_uid(PSA_SUCCESS, nullptr, nullptr);

    psa_storage_info_t info = {
        .size = (uint32_t) context.index_sync_buffer_size
    };
    expect_get_info(&info, PSA_SUCCESS);
    expect_get(nullptr, PSA_SUCCESS);
    expect_variable_index_restore(&context.variable_index, 0, 0);

    auto status = load_variable_index(&context);
    CHECK_EQUAL(EFI_SUCCESS, status);
}
