#-------------------------------------------------------------------------------
# Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Platform definition for the 'total_compute' virtual platform.
#-------------------------------------------------------------------------------
if (NOT DEFINED TGT)
	message(FATAL_ERROR "mandatory parameter TGT is not defined.")
endif()

get_property(_platform_driver_dependencies TARGET ${TGT}
	PROPERTY TS_PLATFORM_DRIVER_DEPENDENCIES
)

#-------------------------------------------------------------------------------
#  Map platform dependencies to suitable drivers for this platform
#
#  Currently using mock drivers.  TODO: enable hardware TRNG for TC
#-------------------------------------------------------------------------------
if ("trng" IN_LIST _platform_driver_dependencies)
	target_sources(${TGT} PRIVATE "${TS_ROOT}/platform/drivers/mock/mock_trng.c")
endif()
