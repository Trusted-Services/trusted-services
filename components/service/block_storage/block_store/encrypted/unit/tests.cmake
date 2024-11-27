#
# Copyright (c) 2024, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include(UnitTest)

unit_test_add_suite(
	NAME encrypted_block_store
	SOURCES
		${UNIT_TEST_PROJECT_PATH}/components/service/block_storage/block_store/encrypted/encrypted_block_store.c
		${UNIT_TEST_PROJECT_PATH}/components/service/block_storage/block_store/block_store.c
		${UNIT_TEST_PROJECT_PATH}/components/service/block_storage/block_store/device/mock/mock_block_store.cpp
		${UNIT_TEST_PROJECT_PATH}/components/service/crypto/mock/mock_crypto.cpp
		${UNIT_TEST_PROJECT_PATH}/components/common/libc/mock/mock_libc.cpp
		${CMAKE_CURRENT_LIST_DIR}/test_encrypted_block_store.cpp
	INCLUDE_DIRECTORIES
		${UNIT_TEST_PROJECT_PATH}
		${UNIT_TEST_PROJECT_PATH}/deployments/unit-tests/include
		${UNIT_TEST_PROJECT_PATH}/components/service/block_storage/block_store/encrypted
		${UNIT_TEST_PROJECT_PATH}/components/service/block_storage/block_store/device/mock
		${UNIT_TEST_PROJECT_PATH}/components/service/crypto/mock
		${UNIT_TEST_PROJECT_PATH}/components/service/crypto/include
		${UNIT_TEST_PROJECT_PATH}/components/common/libc/mock
		${UNIT_TEST_PROJECT_PATH}/components
		${UNIT_TEST_PROJECT_PATH}/components/service/common/include
	COMPILE_DEFINITIONS
		-DARM64
)

# To test if the component handles memory allocation errors properly, the calloc function will be replaced with a custom function
set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/../encrypted_block_store.c PROPERTIES
	COMPILE_DEFINITIONS calloc=MOCK_CALLOC
)

target_compile_definitions(encrypted_block_store PRIVATE
	"TRACE_PREFIX=UT"
	"TRACE_LEVEL=0"
)

add_components(TARGET encrypted_block_store
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/common/trace"
		"components/common/utils"
)