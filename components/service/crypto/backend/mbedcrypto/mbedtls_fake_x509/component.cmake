#-------------------------------------------------------------------------------
# Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
if (NOT DEFINED TGT)
	message(FATAL_ERROR "mandatory parameter TGT is not defined.")
endif()

target_include_directories(${TGT} PRIVATE
	"${MBEDTLS_INSTALL_DIR}/include"
)
target_sources(${TGT} PRIVATE
	"${CMAKE_CURRENT_LIST_DIR}/mbedtls_fake_x509.c"
	)
