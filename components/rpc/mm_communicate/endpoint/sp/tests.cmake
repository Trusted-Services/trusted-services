#
# Copyright (c) 2021-2022, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include(UnitTest)

unit_test_add_suite(
	NAME mm_communicate_call_ep
	SOURCES
		${CMAKE_CURRENT_LIST_DIR}/mm_communicate_call_ep.c
		${CMAKE_CURRENT_LIST_DIR}/test/test_mm_communicate_call_ep.cpp
		${CMAKE_CURRENT_LIST_DIR}/test/mock_mm_service.cpp
		${CMAKE_CURRENT_LIST_DIR}/test/test_mock_mm_service.cpp
		${UNIT_TEST_PROJECT_PATH}/components/messaging/ffa/libsp/mock/mock_assert.cpp
	INCLUDE_DIRECTORIES
		${UNIT_TEST_PROJECT_PATH}
		${UNIT_TEST_PROJECT_PATH}/components/common/utils/include
		${UNIT_TEST_PROJECT_PATH}/components/messaging/ffa/libsp/include
		${UNIT_TEST_PROJECT_PATH}/components/messaging/ffa/libsp/mock
		${UNIT_TEST_PROJECT_PATH}/components/rpc/common/interface
	COMPILE_DEFINITIONS
		-DARM64
		-DCFG_FFA_VERSION=0x00010001
)
