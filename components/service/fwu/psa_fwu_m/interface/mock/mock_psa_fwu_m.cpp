/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "mock_psa_fwu_m.h"
#include <CppUTestExt/MockSupport.h>

void expect_mock_psa_fwu_query(psa_fwu_component_t component, const psa_fwu_component_info_t *info,
			       psa_status_t result)
{
	mock().
		expectOneCall("psa_fwu_query").
		withUnsignedIntParameter("component", component).
		withOutputParameterReturning("info", info, sizeof(*info)).
		andReturnValue(result);
}

psa_status_t psa_fwu_query(psa_fwu_component_t component, psa_fwu_component_info_t *info)
{
	return mock().
		actualCall("psa_fwu_query").
		withUnsignedIntParameter("component", component).
		withOutputParameter("info", info).
		returnIntValue();
}

void expect_mock_psa_fwu_start(psa_fwu_component_t component, const void *manifest,
			       size_t manifest_size, psa_status_t result)
{
	mock().
		expectOneCall("psa_fwu_start").
		withUnsignedIntParameter("component", component).
		withMemoryBufferParameter("manifest", (const uint8_t *)manifest, manifest_size).
		withUnsignedIntParameter("manifest_size", manifest_size).
		andReturnValue(result);
}

psa_status_t psa_fwu_start(psa_fwu_component_t component, const void *manifest,
                           size_t manifest_size)
{
	return mock().
		actualCall("psa_fwu_start").
		withUnsignedIntParameter("component", component).
		withMemoryBufferParameter("manifest", (const uint8_t *)manifest, manifest_size).
		withUnsignedIntParameter("manifest_size", manifest_size).
		returnIntValue();
}

void expect_mock_psa_fwu_write(psa_fwu_component_t component, size_t image_offset,
			       void *block, size_t block_size, psa_status_t result)
{
	mock().
		expectOneCall("psa_fwu_write").
		withUnsignedIntParameter("image_offset", image_offset).
		withMemoryBufferParameter("block", (const uint8_t *)block, block_size).
		withUnsignedIntParameter("block_size", block_size).
		andReturnValue(result);
}

psa_status_t psa_fwu_write(psa_fwu_component_t component, size_t image_offset, const void *block,
                           size_t block_size)
{
	return mock().
		actualCall("psa_fwu_write").
		withUnsignedIntParameter("image_offset", image_offset).
		withMemoryBufferParameter("block", (const uint8_t *)block, block_size).
		withUnsignedIntParameter("block_size", block_size).
		returnIntValue();
}

void expect_mock_psa_fwu_finish(psa_fwu_component_t component, psa_status_t result)
{
	mock().
		expectOneCall("psa_fwu_finish").
		withUnsignedIntParameter("component", component).
		andReturnValue(result);
}

psa_status_t psa_fwu_finish(psa_fwu_component_t component)
{
	return mock().
		actualCall("psa_fwu_finish").
		withUnsignedIntParameter("component", component).
		returnIntValue();
}

void expect_mock_psa_fwu_cancel(psa_fwu_component_t component, psa_status_t result)
{
	mock().
		expectOneCall("psa_fwu_cancel").
		withUnsignedIntParameter("component", component).
		andReturnValue(result);
}

psa_status_t psa_fwu_cancel(psa_fwu_component_t component)
{
	return mock().
		actualCall("psa_fwu_cancel").
		withUnsignedIntParameter("component", component).
		returnIntValue();
}

void expect_mock_psa_fwu_clean(psa_fwu_component_t component, psa_status_t result)
{
	mock().
		expectOneCall("psa_fwu_clean").
		withUnsignedIntParameter("component", component).
		andReturnValue(result);
}

psa_status_t psa_fwu_clean(psa_fwu_component_t component)
{
	return mock().
		actualCall("psa_fwu_clean").
		withUnsignedIntParameter("component", component).
		returnIntValue();
}

void expect_mock_psa_fwu_install(psa_status_t result)
{
	mock().expectOneCall("psa_fwu_install").andReturnValue(result);
}

psa_status_t psa_fwu_install(void)
{
	return mock().actualCall("psa_fwu_install").returnIntValue();
}

void expect_mock_psa_fwu_request_reboot(psa_status_t result)
{
	mock().expectOneCall("psa_fwu_request_reboot").andReturnValue(result);
}

psa_status_t psa_fwu_request_reboot(void)
{
	return mock().actualCall("psa_fwu_request_reboot").returnIntValue();
}

void expect_mock_psa_fwu_reject(psa_status_t error, psa_status_t result)
{
	mock().expectOneCall("psa_fwu_reject").withIntParameter("error", error).andReturnValue(result);
}

psa_status_t psa_fwu_reject(psa_status_t error)
{
	return mock().actualCall("psa_fwu_reject").withIntParameter("error", error).returnIntValue();
}

void expect_mock_psa_fwu_accept(psa_status_t result)
{
	mock().expectOneCall("psa_fwu_accept").andReturnValue(result);
}

psa_status_t psa_fwu_accept(void)
{
	return mock().actualCall("psa_fwu_accept").returnIntValue();
}