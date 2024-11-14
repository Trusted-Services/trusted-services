#
# Copyright (c) 2021-2022, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include(UnitTest)

unit_test_add_suite(
	NAME smm_variable_mm_service
	SOURCES
		${CMAKE_CURRENT_LIST_DIR}/smm_variable_mm_service.c
		${CMAKE_CURRENT_LIST_DIR}/test/test_smm_variable_mm_service.cpp
		${UNIT_TEST_PROJECT_PATH}/components/rpc/common/test/mock_rpc_interface.cpp
		${UNIT_TEST_PROJECT_PATH}/components/messaging/ffa/libsp/mock/mock_assert.cpp
	INCLUDE_DIRECTORIES
		${UNIT_TEST_PROJECT_PATH}
		${UNIT_TEST_PROJECT_PATH}/components/rpc/mm_communicate/endpoint/sp
		${UNIT_TEST_PROJECT_PATH}/components/common/utils/include
		${UNIT_TEST_PROJECT_PATH}/components/messaging/ffa/libsp/include
		${UNIT_TEST_PROJECT_PATH}/components/messaging/ffa/libsp/mock
		${UNIT_TEST_PROJECT_PATH}/components/rpc/common/interface
	COMPILE_DEFINITIONS
		-DARM64
		-DCFG_FFA_VERSION=0x00010001
)
