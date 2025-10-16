#
# Copyright (c) 2026, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

add_library(mock_get_active_variable_uid OBJECT)
target_sources(mock_get_active_variable_uid PRIVATE mock_get_active_variable_uid.cpp)
target_include_directories(mock_get_active_variable_uid
    PRIVATE
        ${UNIT_TEST_PROJECT_PATH}
        ${UNIT_TEST_PROJECT_PATH}/components
        ${UNIT_TEST_PROJECT_PATH}/components/service/common/include
        ${UNIT_TEST_PROJECT_PATH}/components/service/secure_storage/include
        ${UNIT_TEST_PROJECT_PATH}/components/service/uefi/smm_variable/backend
)

add_library(mock_variable_index_restore OBJECT)
target_sources(mock_variable_index_restore PRIVATE mock_variable_index_restore.cpp)
target_include_directories(mock_variable_index_restore
    PRIVATE
        ${UNIT_TEST_PROJECT_PATH}
        ${UNIT_TEST_PROJECT_PATH}/components
        ${UNIT_TEST_PROJECT_PATH}/components/service/common/include
        ${UNIT_TEST_PROJECT_PATH}/components/service/secure_storage/include
        ${UNIT_TEST_PROJECT_PATH}/components/service/uefi/smm_variable/backend
)

add_library(mock_variable_index_confirm_write OBJECT)
target_sources(mock_variable_index_confirm_write PRIVATE mock_variable_index_confirm_write.cpp)
target_include_directories(mock_variable_index_confirm_write
    PRIVATE
        ${UNIT_TEST_PROJECT_PATH}
        ${UNIT_TEST_PROJECT_PATH}/components
        ${UNIT_TEST_PROJECT_PATH}/components/service/common/include
        ${UNIT_TEST_PROJECT_PATH}/components/service/secure_storage/include
        ${UNIT_TEST_PROJECT_PATH}/components/service/uefi/smm_variable/backend
)

add_library(mock_variable_index_dump OBJECT)
target_sources(mock_variable_index_dump PRIVATE mock_variable_index_dump.cpp)
target_include_directories(mock_variable_index_dump
    PRIVATE
        ${UNIT_TEST_PROJECT_PATH}
        ${UNIT_TEST_PROJECT_PATH}/components
        ${UNIT_TEST_PROJECT_PATH}/components/service/common/include
        ${UNIT_TEST_PROJECT_PATH}/components/service/secure_storage/include
        ${UNIT_TEST_PROJECT_PATH}/components/service/uefi/smm_variable/backend
)

add_library(mock_uefi_smm_variable_backend_store INTERFACE)
target_sources(mock_uefi_smm_variable_backend_store INTERFACE
    $<TARGET_OBJECTS:mock_get_active_variable_uid>
)

add_library(mock_uefi_smm_variable_index INTERFACE)
target_sources(mock_uefi_smm_variable_index INTERFACE
    $<TARGET_OBJECTS:mock_variable_index_restore>
    $<TARGET_OBJECTS:mock_variable_index_confirm_write>
    $<TARGET_OBJECTS:mock_variable_index_dump>
)

add_library(mock_uefi_smm_variable_backend_full INTERFACE)
target_link_libraries(mock_uefi_smm_variable_backend_full INTERFACE
    mock_uefi_smm_variable_backend_store
    mock_uefi_smm_variable_index
)
