/*
 * Copyright (c) 2022-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef FW_UPDATE_AGENT_H
#define FW_UPDATE_AGENT_H

#include <stdbool.h>
#include <stdint.h>

#include "common/uuid/uuid.h"
#include "fw_directory.h"
#include "service/fwu/inspector/fw_inspector.h"
#include "stream_manager.h"
#include "components/service/fwu/common/update_agent_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Interface dependencies
 */
struct fw_store;

/**
 * \brief Initialise the update_agent
 *
 * \param[in]  update_agent    The subject update_agent
 * \param[in]  boot_index      The boot_index used by the bootloader
 * \param[in]  fw_inspect_method  fw_inspector inspect method
 * \param[in]  fw_store        The fw_store to manage
 *
 * \return Update agent instance or NULL on error
 */
struct update_agent *update_agent_init(unsigned int boot_index,
				       fw_inspector_inspect fw_inspect_method,
				       struct fw_store *fw_store);

/**
 * \brief De-initialise the update_agent
 *
 * \param[in]  update_agent    The subject update_agent
 */
void update_agent_deinit(struct update_agent *update_agent);

#ifdef __cplusplus
}
#endif

#endif /* FW_UPDATE_AGENT_H */
