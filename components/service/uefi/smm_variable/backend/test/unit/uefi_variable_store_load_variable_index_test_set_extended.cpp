/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "uefi_variable_store_test.h"

#include "mock/uefi_variable_store_test_if.h"

#include <service/locator/sp/ffa/spffa_service_context.h>

#include <vector>
#include <numeric>

TEST_GROUP_BASE(uefi_smm_variable_store_load_variable_index_set_extended, uefi_smm_variable_store) {
};

TEST(uefi_smm_variable_store_load_variable_index_set_extended, get_more_calls) {
    expect_get_active_variable_uid(PSA_SUCCESS, nullptr, nullptr);
    psa_storage_info_t info = {
        .size = (uint32_t) context.index_sync_buffer_size
    };
    expect_get_info(&info, PSA_SUCCESS);

    std::vector<size_t> data_length{
        RPC_CALLER_SESSION_SHARED_MEMORY_SIZE,
        RPC_CALLER_SESSION_SHARED_MEMORY_SIZE / 2
    };
    auto lengthAccumulator = std::accumulate(data_length.cbegin(), data_length.cend(), 0ul);
    for (auto &length : data_length) {
        expect_get(&length, PSA_SUCCESS);
    }

    expect_variable_index_restore(&context.variable_index, lengthAccumulator, 0);

    auto status = load_variable_index(&context);
    CHECK_EQUAL(EFI_SUCCESS, status);
}
