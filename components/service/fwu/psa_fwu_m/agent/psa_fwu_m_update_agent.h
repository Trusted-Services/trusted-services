/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef PSA_FWU_M_UPDATE_AGENT_H
#define PSA_FWU_M_UPDATE_AGENT_H

#include "service/fwu/common/update_agent_interface.h"
#include "service/fwu/psa_fwu_m/interface/update.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct psa_fwu_m_image_mapping {
	struct uuid_octets uuid;
	psa_fwu_component_t component;
};

/**
 * \brief Initialise the PSA FWU M update_agent
 *
 * \param[in]  image_mapping    Component mapping array
 * \param[in]  image_count      Component mapping count
 * \param[in]  max_payload_size The maximum number of bytes that a payload can contain
 *
 * \return  The update_agent
 */
struct update_agent *psa_fwu_m_update_agent_init(
	const struct psa_fwu_m_image_mapping image_mapping[], size_t image_count,
	uint32_t max_payload_size);

/**
 * \brief De-initialise the update agent
 *
 * \param[in]  update_agent    The subject update_agent
 */
void psa_fwu_m_update_agent_deinit(struct update_agent *update_agent);

#ifdef __cplusplus
}
#endif

#endif /* PSA_FWU_M_UPDATE_AGENT_H */
