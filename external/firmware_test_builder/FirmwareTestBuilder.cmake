#-------------------------------------------------------------------------------
# Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------


set(FIRMWARE_TEST_BUILDER_URL "https://git.trustedfirmware.org/TS/trusted-services.git" CACHE STRING "firmware-test-builder repository URL")
set(FIRMWARE_TEST_BUILDER_REFSPEC "topics/fwtb" CACHE STRING "firmware-test-builder git refspec")
set(BRANCH_PROTECTION @BRANCH_PROTECTION@ CACHE STRING "")

set(GIT_OPTIONS
	GIT_REPOSITORY ${FIRMWARE_TEST_BUILDER_URL}
	GIT_TAG ${FIRMWARE_TEST_BUILDER_REFSPEC}
	GIT_SHALLOW FALSE)

include(${TS_ROOT}/tools/cmake/common/LazyFetch.cmake REQUIRED)
LazyFetch_MakeAvailable(
	DEP_NAME firmware_test_builder
	FETCH_OPTIONS "${GIT_OPTIONS}"
	)

# Appending firmware-test-builder's CMake directory to CMake module path
list(APPEND CMAKE_MODULE_PATH ${firmware_test_builder_SOURCE_DIR}/cmake)
