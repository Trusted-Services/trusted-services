/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fwu_provider_shim.h"
#include "protocols/service/fwu/fwu_proto.h"
#include "util.h"

static rpc_status_t receive(void *context, struct rpc_request *req)
{
	struct rpc_service_interface *fwu_rpc_interface = (struct rpc_service_interface *)context;
	struct fwu_request_header *req_header = NULL;
	struct fwu_response_header *resp_header = NULL;
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	struct rpc_request fwu_req = { 0 };

	if (req->request.data_length < sizeof(*req_header))
		return RPC_ERROR_INVALID_REQUEST_BODY;

	if (req->response.size < sizeof(*resp_header))
		return RPC_ERROR_INVALID_RESPONSE_BODY;

	req_header = (struct fwu_request_header *)req->request.data;
	resp_header = (struct fwu_response_header *)req->response.data;

	/* Build new RPC request without the request headers */
	fwu_req = *req;
	fwu_req.opcode = req_header->func_id & 0xffff;
	fwu_req.request.data = req_header->payload;
	fwu_req.request.data_length -= sizeof(*req_header);
	fwu_req.request.size -= sizeof(*req_header);
	fwu_req.response.data = resp_header->payload;
	fwu_req.response.size -= sizeof(*resp_header);

	rpc_status = rpc_service_receive(fwu_rpc_interface, &fwu_req);
	if (rpc_status != RPC_SUCCESS)
		return rpc_status;

	/* Insert service status into response buffer */
	resp_header->status = fwu_req.service_status;

	req->service_status = fwu_req.service_status;

	/* Update response data length */
	if (ADD_OVERFLOW(fwu_req.response.data_length, sizeof(uint32_t),
			 &req->response.data_length))
		return RPC_ERROR_INVALID_RESPONSE_BODY;

	if (req->response.data_length > req->response.size)
		return RPC_ERROR_INVALID_RESPONSE_BODY;

	return rpc_status;
}

struct rpc_service_interface *
fwu_provider_shim_init(struct fwu_provider_shim *shim,
		       struct rpc_service_interface *fwu_rpc_interface)
{
	shim->shim_rpc_interface.receive = receive;
	shim->shim_rpc_interface.context = fwu_rpc_interface;
	shim->shim_rpc_interface.uuid = fwu_rpc_interface->uuid;

	return &shim->shim_rpc_interface;
}

void fwu_provider_shim_deinit(struct fwu_provider_shim *shim)
{
	(void)shim;
}
