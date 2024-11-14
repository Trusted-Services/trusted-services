/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "../update.h"

void expect_mock_psa_fwu_query(psa_fwu_component_t component, const psa_fwu_component_info_t *info,
			       psa_status_t result);

void expect_mock_psa_fwu_start(psa_fwu_component_t component, const void *manifest,
			       size_t manifest_size, psa_status_t result);

void expect_mock_psa_fwu_write(psa_fwu_component_t component, size_t image_offset,
			       void *block, size_t block_size, psa_status_t result);

void expect_mock_psa_fwu_finish(psa_fwu_component_t component, psa_status_t result);

void expect_mock_psa_fwu_cancel(psa_fwu_component_t component, psa_status_t result);

void expect_mock_psa_fwu_clean(psa_fwu_component_t component, psa_status_t result);

void expect_mock_psa_fwu_install(psa_status_t result);

void expect_mock_psa_fwu_request_reboot(psa_status_t result);

void expect_mock_psa_fwu_reject(psa_status_t error, psa_status_t result);

void expect_mock_psa_fwu_accept(psa_status_t result);
