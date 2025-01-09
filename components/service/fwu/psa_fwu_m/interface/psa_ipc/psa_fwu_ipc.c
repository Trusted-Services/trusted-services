/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <string.h>
#include <trace.h>

#include <protocols/rpc/common/packed-c/status.h>
#include <psa/client.h>
#include <psa/sid.h>
#include <service/common/client/service_client.h>
#include "service/fwu/psa_fwu_m/interface/psa_ipc/psa_fwu_ipc.h"
#include "service/fwu/psa_fwu_m/interface/tfm_fwu_defs.h"
#include "service/fwu/psa_fwu_m/interface/update.h"

/**
 * @brief      The singleton psa_fwu_ipc instance
 *
 * The psa attestation C API assumes a single backend service provider.
 */
static struct service_client instance;

psa_status_t psa_fwu_ipc_init(struct rpc_caller_session *session)
{
	return service_client_init(&instance, session);
}

void psa_fwu_ipc_deinit(void)
{
	service_client_deinit(&instance);
}

int psa_fwu_rpc_status(void)
{
	return instance.rpc_status;
}

psa_status_t psa_fwu_query(psa_fwu_component_t component,
			   psa_fwu_component_info_t *info)
{
	if (!instance.session)
		return PSA_ERROR_BAD_STATE;
	if (!info)
		return PSA_ERROR_INVALID_ARGUMENT;

	psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;
	struct rpc_caller_interface *caller = instance.session->caller;
	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&component), .len = sizeof(component) },
	};
	struct psa_outvec out_vec[] = {
		{ .base = psa_ptr_to_u32(info), .len = sizeof(*info) },
	};

	status = psa_call(caller, TFM_FIRMWARE_UPDATE_SERVICE_HANDLE,
			  TFM_FWU_QUERY, in_vec, IOVEC_LEN(in_vec),
			  out_vec, IOVEC_LEN(out_vec));
	if (status != PSA_SUCCESS)
		EMSG("failed to psa_call: %d", status);

	return status;
}

psa_status_t psa_fwu_start(psa_fwu_component_t component,
			   const void *manifest,
			   size_t manifest_size)
{
	if(manifest_size > UINT32_MAX)
		return PSA_ERROR_INVALID_ARGUMENT;
	if (!instance.session)
		return PSA_ERROR_BAD_STATE;

	psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;
	struct rpc_caller_interface *caller = instance.session->caller;
	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&component), .len = sizeof(component) },
		{ .base = psa_ptr_const_to_u32(manifest), .len = manifest_size },
	};

	status = psa_call(caller, TFM_FIRMWARE_UPDATE_SERVICE_HANDLE,
			  TFM_FWU_START, in_vec, IOVEC_LEN(in_vec),
			  NULL, 0);
	if (status != PSA_SUCCESS)
		EMSG("failed to psa_call: %d", status);

	return status;
}

psa_status_t psa_fwu_write(psa_fwu_component_t component,
			   size_t image_offset,
			   const void *block,
			   size_t block_size)
{
	if (!instance.session)
		return PSA_ERROR_BAD_STATE;
	if (!block || !block_size)
		return PSA_ERROR_INVALID_ARGUMENT;
	if((image_offset > UINT32_MAX) || (block_size > UINT32_MAX))
		return PSA_ERROR_INVALID_ARGUMENT;

	psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;
	struct rpc_caller_interface *caller = instance.session->caller;
	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&component), .len = sizeof(component) },
		{ .base = psa_ptr_to_u32(&image_offset), .len = sizeof(uint32_t) },
		{ .base = psa_ptr_const_to_u32(block), .len = block_size },
	};

	status = psa_call(caller, TFM_FIRMWARE_UPDATE_SERVICE_HANDLE,
			  TFM_FWU_WRITE, in_vec, IOVEC_LEN(in_vec),
			  NULL, 0);
	if (status != PSA_SUCCESS)
		EMSG("failed to psa_call: %d", status);

	return status;
}

psa_status_t psa_fwu_finish(psa_fwu_component_t component)
{
	if (!instance.session)
		return PSA_ERROR_BAD_STATE;

	psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;
	struct rpc_caller_interface *caller = instance.session->caller;
	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&component), .len = sizeof(component) },
	};

	status = psa_call(caller, TFM_FIRMWARE_UPDATE_SERVICE_HANDLE,
			  TFM_FWU_FINISH, in_vec, IOVEC_LEN(in_vec),
			  NULL, 0);
	if (status != PSA_SUCCESS)
		EMSG("failed to psa_call: %d", status);

	return status;
}

psa_status_t psa_fwu_cancel(psa_fwu_component_t component)
{
	if (!instance.session)
		return PSA_ERROR_BAD_STATE;

	psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;
	struct rpc_caller_interface *caller = instance.session->caller;
	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&component), .len = sizeof(component) },
	};

	status = psa_call(caller, TFM_FIRMWARE_UPDATE_SERVICE_HANDLE,
			  TFM_FWU_CANCEL, in_vec, IOVEC_LEN(in_vec),
			  NULL, 0);
	if (status != PSA_SUCCESS)
		EMSG("failed to psa_call: %d", status);

	return status;
}

psa_status_t psa_fwu_clean(psa_fwu_component_t component)
{
	if (!instance.session)
		return PSA_ERROR_BAD_STATE;

	psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;
	struct rpc_caller_interface *caller = instance.session->caller;
	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&component), .len = sizeof(component) },
	};

	status = psa_call(caller, TFM_FIRMWARE_UPDATE_SERVICE_HANDLE,
			  TFM_FWU_CLEAN, in_vec, IOVEC_LEN(in_vec),
			  NULL, 0);
	if (status != PSA_SUCCESS)
		EMSG("failed to psa_call: %d", status);

	return status;
}

psa_status_t psa_fwu_install(void)
{
	if (!instance.session)
		return PSA_ERROR_BAD_STATE;

	psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;
	struct rpc_caller_interface *caller = instance.session->caller;
	struct psa_invec in_vec[] = {};

	status = psa_call(caller, TFM_FIRMWARE_UPDATE_SERVICE_HANDLE,
			  TFM_FWU_INSTALL, in_vec, 0,
			  NULL, 0);
	if (status != PSA_SUCCESS)
		EMSG("failed to psa_call: %d", status);

	return status;
}

psa_status_t psa_fwu_request_reboot(void)
{
	if (!instance.session)
		return PSA_ERROR_BAD_STATE;

	psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;
	struct rpc_caller_interface *caller = instance.session->caller;
	struct psa_invec in_vec[] = {};

	status = psa_call(caller, TFM_FIRMWARE_UPDATE_SERVICE_HANDLE,
			  TFM_FWU_REQUEST_REBOOT, in_vec, 0,
			  NULL, 0);
	if (status != PSA_SUCCESS)
		EMSG("failed to psa_call: %d", status);

	return status;
}

psa_status_t psa_fwu_reject(psa_status_t error)
{
	if (!instance.session)
		return PSA_ERROR_BAD_STATE;

	psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;
	struct rpc_caller_interface *caller = instance.session->caller;
	struct psa_invec in_vec[] = {
		{ .base = psa_ptr_to_u32(&error), .len = sizeof(error) },
	};

	status = psa_call(caller, TFM_FIRMWARE_UPDATE_SERVICE_HANDLE,
			  TFM_FWU_REJECT, in_vec, IOVEC_LEN(in_vec),
			  NULL, 0);
	if (status != PSA_SUCCESS)
		EMSG("failed to psa_call: %d", status);

	return status;
}

psa_status_t psa_fwu_accept(void)
{
	if (!instance.session)
		return PSA_ERROR_BAD_STATE;

	psa_status_t status = PSA_ERROR_INVALID_ARGUMENT;
	struct rpc_caller_interface *caller = instance.session->caller;
	struct psa_invec in_vec[] = {};

	status = psa_call(caller, TFM_FIRMWARE_UPDATE_SERVICE_HANDLE,
			  TFM_FWU_ACCEPT, in_vec, 0,
			  NULL, 0);
	if (status != PSA_SUCCESS)
		EMSG("failed to psa_call: %d", status);

	return status;
}
