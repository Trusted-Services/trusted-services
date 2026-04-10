#
# Copyright (c) 2026, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include(UnitTest)

unit_test_add_suite(
	NAME rpmb_block_store
	SOURCES
		${UNIT_TEST_PROJECT_PATH}/components/service/block_storage/block_store/device/rpmb/rpmb_block_store.c
		${UNIT_TEST_PROJECT_PATH}/components/service/block_storage/block_store/block_store.c
		${UNIT_TEST_PROJECT_PATH}/components/service/block_storage/block_store/device/block_device.c
		${UNIT_TEST_PROJECT_PATH}/components/service/block_storage/block_store/storage_partition.c
		${UNIT_TEST_PROJECT_PATH}/components/service/block_storage/block_store/storage_partition_acl.c
		${UNIT_TEST_PROJECT_PATH}/components/common/uuid/uuid.c
		${UNIT_TEST_PROJECT_PATH}/components/common/libc/mock/mock_libc.cpp
		${UNIT_TEST_PROJECT_PATH}/components/service/rpmb/frontend/mock/mock_rpmb_frontend.cpp
		${CMAKE_CURRENT_LIST_DIR}/rpmb_block_store_tests.cpp
	INCLUDE_DIRECTORIES
		${UNIT_TEST_PROJECT_PATH}
		${UNIT_TEST_PROJECT_PATH}/deployments/unit-tests/include
		${UNIT_TEST_PROJECT_PATH}/components/service/block_storage/block_store/rpmb
		${UNIT_TEST_PROJECT_PATH}/components/service/block_storage/block_store/device/mock
		${UNIT_TEST_PROJECT_PATH}/components
		${UNIT_TEST_PROJECT_PATH}/components/service/common/include
		${UNIT_TEST_PROJECT_PATH}/components/rpc/common/interface
	COMPILE_DEFINITIONS
		-DARM64
)

# To test if the component handles memory allocation errors properly, the calloc function will be replaced with a custom function
set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/../rpmb_block_store.c PROPERTIES
	COMPILE_DEFINITIONS calloc=MOCK_CALLOC
)

target_compile_definitions(rpmb_block_store PRIVATE
	"TRACE_PREFIX=UT"
	"TRACE_LEVEL=0"
)

add_components(TARGET rpmb_block_store
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/common/trace"
		"components/common/utils"
)