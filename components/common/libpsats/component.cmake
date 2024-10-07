#-------------------------------------------------------------------------------
# Copyright (c) 2020-2021, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------
if (NOT DEFINED TGT)
	message(FATAL_ERROR "mandatory parameter TGT is not defined.")
endif()

target_sources(${TGT} PRIVATE
	"${CMAKE_CURRENT_LIST_DIR}/libpsats-attestation.c"
	"${CMAKE_CURRENT_LIST_DIR}/libpsats-crypto.c"
	"${CMAKE_CURRENT_LIST_DIR}/libpsats-its.c"
	"${CMAKE_CURRENT_LIST_DIR}/libpsats-ps.c"
	)
