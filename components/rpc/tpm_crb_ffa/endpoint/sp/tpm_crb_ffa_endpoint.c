// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 */

#include <string.h>
#include "components/rpc/tpm_crb_ffa/common/tpm_crb_ffa.h"
#include "tpm_crb_ffa_endpoint.h"
#include "trace.h"

bool tpm_crb_ffa_endpoint_init(struct tpm_crb_ffa_ep *endpoint)
{
	if (!endpoint)
		return false;

	endpoint->service = NULL;

	return true;
}

bool tpm_crb_ffa_endpoint_add_service(struct tpm_crb_ffa_ep *endpoint,
				      struct rpc_service_interface *service)
{
	if (!endpoint || !service || endpoint->service != NULL)
		return false;

	endpoint->service = service;

	return true;
}

static void handle_management_command(struct tpm_crb_ffa_ep *endpoint, uint16_t source_id,
				      const uint64_t request[5], uint64_t response[5])
{
	switch (tpm_crb_ffa_get_function_id(request)) {
	case TPM_FUNC_GET_INTERFACE_VERSION:
		DMSG("Received TPM get interface version function");
		tpm_crb_ffa_set_interface_version(response, TPM_INTERFACE_VERSION);
		tpm_crb_ffa_set_status(response, TPM_STATUS_OK_RESULTS_RETURNED);
		break;
	case TPM_FUNC_GET_FEATURE_INFO:
		DMSG("Received TPM get feature info function");
		tpm_crb_ffa_set_status(response, TPM_ERROR_NOTSUP);
		break;
	case TPM_FUNC_REGISTER_FOR_NOTIFICATION:
		DMSG("Received TPM register for notification function");
		tpm_crb_ffa_set_status(response, TPM_ERROR_NOTSUP);
		break;
	case TPM_FUNC_UNREGISTER_FROM_NOTIFICATION:
		DMSG("Received TPM unregister from notification function");
		tpm_crb_ffa_set_status(response, TPM_ERROR_NOTSUP);
		break;
	case TPM_FUNC_FINISH_NOTIFIED:
		DMSG("Received TPM finish notified function");
		tpm_crb_ffa_set_status(response, TPM_ERROR_NOTSUP);
		break;
	default:
		EMSG("Received unknown TPM function: 0x%lx", tpm_crb_ffa_get_function_id(request));
		tpm_crb_ffa_set_status(response, TPM_ERROR_NOFUNC);
		break;
	}
}

static void handle_start_command(struct tpm_crb_ffa_ep *endpoint, uint16_t source_id,
				 const uint64_t request[5], uint64_t response[5])
{
	rpc_status_t status = RPC_ERROR_INTERNAL;
	struct rpc_request rpc_request = { 0 };
	uint8_t command_type = 0;
	uint8_t locality = 0;

	if (tpm_crb_ffa_get_function_id(request) != TPM_FUNC_START) {
		EMSG("Expected TPM start command");
		tpm_crb_ffa_set_status(response, TPM_ERROR_INVARG);
		return;
	}

	command_type = tpm_crb_ffa_get_start_qualifier(request);
	if (command_type != TPM_START_QUALIFIER_COMMAND &&
	    command_type != TPM_START_QUALIFIER_LOCALITY_REQ) {
		EMSG("Invalid TPM service start function qualifier: %d", command_type);
		tpm_crb_ffa_set_status(response, TPM_ERROR_INVARG);
		return;
	}

	locality = tpm_crb_ffa_get_start_loc_qualifier(request);
	if (locality > 4) {
		EMSG("Invalid TPM service start function locality qualifier.: %d", locality);
		tpm_crb_ffa_set_status(response, TPM_ERROR_INVARG);
		return;
	}

	DMSG("Received TPM start command. Type: %d. Locality: %d", command_type, locality);

	/*
	 * The common RPC request type can be reused for TPM CRB too. Some of the fields are not
	 * used, since the TPM CRB provider uses static carveouts for the payload and status codes.
	 */
	rpc_request.source_id = source_id;
	rpc_request.opcode = command_type;
	rpc_request.client_id = locality;

	status = rpc_service_receive(endpoint->service, &rpc_request);
	if (status != RPC_SUCCESS) {
		tpm_crb_ffa_set_status(response, rpc_request.service_status);
	}

	tpm_crb_ffa_set_status(response, TPM_STATUS_OK);
}

void tpm_crb_ffa_endpoint_receive(struct tpm_crb_ffa_ep *endpoint, const struct sp_msg *request,
				  struct sp_msg *response)
{
	uint64_t req_copy[SP_MSG_ARG_COUNT] = { 0 };
	uint64_t resp_copy[SP_MSG_ARG_COUNT] = { 0 };

	response->source_id = request->destination_id;
	response->destination_id = request->source_id;
	response->is_64bit_message = request->is_64bit_message;
	memset(&response->args, 0x00, sizeof(response->args));

	/*
	 * We have to accept both 32-bit and 64-bit messages, but the spec hasn't been updated yet
	 * to cover this. For now let's just treat them the same way, zero extending the 32-bit
	 * arguments and only use 64-bit from this point.
	 */
	if (request->is_64bit_message) {
		memcpy(req_copy, request->args.args64, sizeof(request->args.args64));
	} else {
		for (unsigned i = 0; i < SP_MSG_ARG_COUNT; i++)
			req_copy[i] = request->args.args32[i];
	}

	/* Sanity check */
	if (req_copy[0] != 0) {
		EMSG("The TPM CRB services requires x3 MBZ");
		return;
	}

	if (tpm_crb_ffa_get_function_id(req_copy) == TPM_FUNC_START)
		handle_start_command(endpoint, request->source_id, req_copy, resp_copy);
	else
		handle_management_command(endpoint, request->source_id, req_copy, resp_copy);

	/* Truncate the response arguments to 32 bits if necessary */
	if (response->is_64bit_message) {
		memcpy(response->args.args64, resp_copy, sizeof(request->args.args64));
	} else {
		for (unsigned i = 0; i < SP_MSG_ARG_COUNT; i++)
			response->args.args32[i] = (uint32_t)resp_copy[i];
	}
}
