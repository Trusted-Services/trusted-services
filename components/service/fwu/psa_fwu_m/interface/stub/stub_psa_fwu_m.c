/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "../update.h"
#include "protocols/service/fwu/status.h"

psa_status_t psa_fwu_query(psa_fwu_component_t component, psa_fwu_component_info_t *info)
{
	return FWU_STATUS_DENIED;
}

psa_status_t psa_fwu_start(psa_fwu_component_t component, const void *manifest,
			   size_t manifest_size)
{
	return FWU_STATUS_DENIED;
}

psa_status_t psa_fwu_write(psa_fwu_component_t component, size_t image_offset, const void *block,
			   size_t block_size)
{
	return FWU_STATUS_DENIED;
}

psa_status_t psa_fwu_finish(psa_fwu_component_t component)
{
	return FWU_STATUS_DENIED;
}

psa_status_t psa_fwu_cancel(psa_fwu_component_t component)
{
	return FWU_STATUS_DENIED;
}

psa_status_t psa_fwu_clean(psa_fwu_component_t component)
{
	return FWU_STATUS_DENIED;
}

psa_status_t psa_fwu_install(void)
{
	return FWU_STATUS_DENIED;
}

psa_status_t psa_fwu_request_reboot(void)
{
	return FWU_STATUS_DENIED;
}

psa_status_t psa_fwu_reject(psa_status_t error)
{
	return FWU_STATUS_DENIED;
}

psa_status_t psa_fwu_accept(void)
{
	return FWU_STATUS_DENIED;
}
