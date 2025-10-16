#
# Copyright (c) 2026, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

add_library(mock_secure_storage_store STATIC)
target_sources(mock_secure_storage_store PRIVATE
    mock_store.c
    utest_mock_store.cpp
)
target_include_directories(mock_secure_storage_store PRIVATE
    ${UNIT_TEST_PROJECT_PATH}
    ${UNIT_TEST_PROJECT_PATH}/components
    ${UNIT_TEST_PROJECT_PATH}/components/common/utils/include
    ${UNIT_TEST_PROJECT_PATH}/components/service/common/include
    ${UNIT_TEST_PROJECT_PATH}/components/service/secure_storage/include
)
