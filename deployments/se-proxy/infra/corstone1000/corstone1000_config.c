/*
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "service/fwu/psa_fwu_m/agent/psa_fwu_m_update_agent.h"
#include "protocols/service/fwu/fwu_proto.h"

#include "corstone1000_config.h"

#ifndef CORSTONE_1000_TYPE
#error "CORSTONE_1000_TYPE is not defined."
#endif

struct psa_fwu_m_image_mapping img_mapping = {
	.count = FWU_IMAGE_INDEX_COUNT,
	.images = {
#if (CORSTONE_1000_TYPE == CORSTONE_1000_TYPE_CORTEX_A35_FVP)
		{
			.uuid = {FWU_FVP_BL2_CAPSULE_UUID},
			.component = FWU_IMAGE_INDEX_CAPSULE_BL2
		},
		{
			.uuid = {FWU_FVP_TFM_S_CAPSULE_UUID},
			.component = FWU_IMAGE_INDEX_CAPSULE_TFM_S
		},
		{
			.uuid = {FWU_FVP_FIP_CAPSULE_UUID},
			.component = FWU_IMAGE_INDEX_CAPSULE_FIP
		},
		{
			.uuid = {FWU_FVP_INITRAMFS_CAPSULE_UUID},
			.component = FWU_IMAGE_INDEX_CAPSULE_INITRAMFS
		},
#elif (CORSTONE_1000_TYPE == CORSTONE_1000_TYPE_CORTEX_A35_MPS3)
		{
			.uuid = {FWU_MPS3_BL2_CAPSULE_UUID},
			.component = FWU_IMAGE_INDEX_CAPSULE_BL2
		},
		{
			.uuid = {FWU_MPS3_TFM_S_CAPSULE_UUID},
			.component = FWU_IMAGE_INDEX_CAPSULE_TFM_S
		},
		{
			.uuid = {FWU_MPS3_FIP_CAPSULE_UUID},
			.component = FWU_IMAGE_INDEX_CAPSULE_FIP
		},
		{
			.uuid = {FWU_MPS3_INITRAMFS_CAPSULE_UUID},
			.component = FWU_IMAGE_INDEX_CAPSULE_INITRAMFS
		},
#else
#error "Unsupported CORSTONE_1000_TYPE value. Valid values are: 0 (Corstone-1000 with " \
		"Cortex-A35 FVP), 1 (Corstone-1000 with Cortex-A35 MPS3)"
#endif
		/* Placeholder for ESRT entry; will be populated at runtime */
		{} /* ESRT image */
	}
};
