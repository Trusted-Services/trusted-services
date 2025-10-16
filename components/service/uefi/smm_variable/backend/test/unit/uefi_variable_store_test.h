/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef UEFI_VARIABLE_STORE_TEST_H
#define UEFI_VARIABLE_STORE_TEST_H

#include <uefi_variable_store.h>

#include <service/secure_storage/backend/mock_store/mock_store.h>
#include <service/secure_storage/backend/mock_store/utest_mock_store.h>

#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

#include <memory>

extern "C" struct storage_backend_interface uefi_variable_store_test_mock_storage_backend_interface;

TEST_BASE(uefi_smm_variable_store)
{
    TEST_SETUP() {

        m_persistent_backend = mock_store_init(&m_persistent_store);
        m_volatile_backend = mock_store_init(&m_volatile_store);

        m_persistent_backend->interface = &uefi_variable_store_test_mock_storage_backend_interface;
        m_volatile_backend->interface = &uefi_variable_store_test_mock_storage_backend_interface;

        // uefi_variable_store_init
        /* Initialise persistent store defaults */
        context.persistent_store.is_nv = true;
        context.persistent_store.max_variable_size = DEFAULT_MAX_VARIABLE_SIZE;
        context.persistent_store.total_capacity = DEFAULT_MAX_VARIABLE_SIZE * MAX_VARIABLES;
        context.persistent_store.storage_backend = m_persistent_backend;
        /* Initialise volatile store defaults */
        context.volatile_store.is_nv = false;
        context.volatile_store.max_variable_size = DEFAULT_MAX_VARIABLE_SIZE;
        context.volatile_store.total_capacity = DEFAULT_MAX_VARIABLE_SIZE * MAX_VARIABLES;
        context.volatile_store.storage_backend = m_volatile_backend;

        context.owner_id = OWNER_ID;
        context.is_boot_service = true;


        // variable_index_init
        context.variable_index.max_variables = MAX_VARIABLES;
        context.variable_index.counter = 0;
        context.variable_index.entries = variable_index_entries.data();


        // variable_index_max_dump_size
        context.index_sync_buffer_size =
            sizeof(context.variable_index.counter)
            + (sizeof(struct variable_metadata)
            + sizeof(bool)
            + sizeof(struct variable_constraints)) * context.variable_index.max_variables;


        index_sync_buffer.reset(new uint8_t[context.index_sync_buffer_size]);
        context.index_sync_buffer = index_sync_buffer.get();
    }

    TEST_TEARDOWN() {
        variable_index_entries = {};
        index_sync_buffer.reset();

        mock().checkExpectations();
        mock().clear();
    }

    struct uefi_variable_store context;
    static const uint32_t OWNER_ID = 100u;
    static const size_t MAX_VARIABLES = 2048;
    struct mock_store m_persistent_store;
    struct mock_store m_volatile_store;
    struct storage_backend *m_persistent_backend;
    struct storage_backend *m_volatile_backend;

    std::array<variable_entry, MAX_VARIABLES> variable_index_entries = {0};
    std::unique_ptr<uint8_t> index_sync_buffer;
};


#endif // UEFI_VARIABLE_STORE_TEST_H
