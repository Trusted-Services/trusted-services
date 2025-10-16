#
# Copyright (c) 2025, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include(UnitTest)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/mock" "mock")
add_subdirectory("${UNIT_TEST_PROJECT_PATH}/components/service/secure_storage/backend/mock_store" "mock_secure_storage_backend_store")

add_library(uefi_smm_variable_common INTERFACE)

target_sources(uefi_smm_variable_common
INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/test/unit/uefi_variable_store_load_variable_index_test.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test/unit/uefi_variable_store_sync_variable_index_test.cpp
# PRIVATE
#     ${CMAKE_CURRENT_LIST_DIR}/test/unit/uefi_variable_store_test_storage_backend_mock.c
)

target_include_directories(uefi_smm_variable_common INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    ${UNIT_TEST_PROJECT_PATH}
    ${UNIT_TEST_PROJECT_PATH}/components
    ${UNIT_TEST_PROJECT_PATH}/components/common/trace/include
    ${UNIT_TEST_PROJECT_PATH}/components/common/utils/include
    ${UNIT_TEST_PROJECT_PATH}/components/rpc/common/caller
    ${UNIT_TEST_PROJECT_PATH}/components/rpc/common/interface
    ${UNIT_TEST_PROJECT_PATH}/components/service/common/include
    ${UNIT_TEST_PROJECT_PATH}/components/service/locator/interface
    # ${UNIT_TEST_PROJECT_PATH}/components/service/locator/sp/ffa
    ${UNIT_TEST_PROJECT_PATH}/components/service/secure_storage/include
)

target_compile_definitions(uefi_smm_variable_common INTERFACE
    -DTRACE_LEVEL=TRACE_LEVEL_NONE
    -DDEFAULT_MAX_VARIABLE_SIZE=4096
    "-DSMM_VARIABLE_INDEX_STORAGE_A_UID=UINT64_C(0x8000000000000001)"
    "-DSMM_VARIABLE_INDEX_STORAGE_B_UID=UINT64_C(0x8000000000000002)"
)

target_link_libraries(uefi_smm_variable_common INTERFACE
    mock_secure_storage_store
)

unit_test_add_suite(
    NAME uefi_smm_variable
    SOURCES
        # ${CMAKE_CURRENT_LIST_DIR}/test/unit/uefi_variable_store_load_variable_index_test_minimal.cpp
        ${CMAKE_CURRENT_LIST_DIR}/test/unit/uefi_variable_store_sync_variable_index_test_minimal.cpp
        ${CMAKE_CURRENT_LIST_DIR}/test/unit/uefi_variable_store_test.yml
)
target_compile_options(uefi_smm_variable PRIVATE
    -include ${CMAKE_CURRENT_LIST_DIR}/mock/uefi_variable_store_test_if.h
)

target_link_libraries(uefi_smm_variable PRIVATE
    uefi_smm_variable_common
    mock_uefi_smm_variable_backend_store
    mock_uefi_smm_variable_index
)

unit_test_add_suite(
    NAME uefi_smm_variable_set_extended
    SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/test/unit/uefi_variable_store_load_variable_index_test_set_extended.cpp
        ${CMAKE_CURRENT_LIST_DIR}/test/unit/uefi_variable_store_sync_variable_index_test_set_extended.cpp
        ${CMAKE_CURRENT_LIST_DIR}/test/unit/uefi_variable_store_test.yml
    COMPILE_DEFINITIONS
        -DCFG_SMM_REQUIRE_SET_EXTENDED=1
)
target_compile_options(uefi_smm_variable_set_extended PRIVATE
    -include ${CMAKE_CURRENT_LIST_DIR}/mock/uefi_variable_store_test_if.h
)
target_link_libraries(uefi_smm_variable_set_extended PRIVATE
    uefi_smm_variable_common
    mock_uefi_smm_variable_backend_store
    mock_uefi_smm_variable_index
)
