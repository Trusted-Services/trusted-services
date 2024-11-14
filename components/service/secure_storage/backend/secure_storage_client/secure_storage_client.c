/*
 * Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "secure_storage_client.h"
#include "protocols/service/secure_storage/packed-c/secure_storage_proto.h"
#include "protocols/rpc/common/packed-c/status.h"
#include "rpc_caller_session.h"
#include "util.h"
#include <string.h>

static psa_status_t secure_storage_client_set(void *context,
					      uint32_t client_id,
					      psa_storage_uid_t uid,
					      size_t data_length,
					      const void *p_data,
					      psa_storage_create_flags_t create_flags)
{
	struct secure_storage_client *this_context = (struct secure_storage_client *)context;
	uint8_t *request = NULL;
	uint8_t *response = NULL;
	size_t request_length = 0;
	size_t response_length = 0;
	struct secure_storage_request_set *request_desc = NULL;
	rpc_call_handle handle = 0;
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	service_status_t service_status = 0;
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;

	this_context->client.rpc_status = RPC_SUCCESS;

	(void)client_id;

	/* Validating input parameters */
	if (p_data == NULL && data_length != 0)
		return PSA_ERROR_INVALID_ARGUMENT;

	if (ADD_OVERFLOW(sizeof(*request_desc), data_length, &request_length))
		return PSA_ERROR_INVALID_ARGUMENT;

	handle = rpc_caller_session_begin(this_context->client.session, &request, request_length,
					  0);
	if (!handle)
		goto out;

	/* Populating request descriptor */
	request_desc = (struct secure_storage_request_set *)request;
	request_desc->uid = uid;
	request_desc->data_length = data_length;
	request_desc->create_flags = create_flags;
	memcpy(&request_desc->p_data, p_data, data_length);

	rpc_status = rpc_caller_session_invoke(handle, TS_SECURE_STORAGE_OPCODE_SET,
						&response, &response_length, &service_status);
	if (rpc_status == RPC_SUCCESS)
		psa_status = service_status;

	rpc_status = rpc_caller_session_end(handle);
	if (psa_status == PSA_SUCCESS && rpc_status != RPC_SUCCESS)
		psa_status = PSA_ERROR_GENERIC_ERROR;

out:
	return psa_status;
}

static psa_status_t secure_storage_client_get(void *context,
					      uint32_t client_id,
					      psa_storage_uid_t uid,
					      size_t data_offset,
					      size_t data_size,
					      void *p_data,
					      size_t *p_data_length)
{
	struct secure_storage_client *this_context = (struct secure_storage_client*)context;
	uint8_t *request = NULL;
	uint8_t *response = NULL;
	size_t response_length = 0;
	size_t expected_response_length = data_size;
	struct secure_storage_request_get *request_desc = NULL;
	rpc_call_handle handle = 0;
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	service_status_t service_status = 0;
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;

	(void)client_id;

	/* Validating input parameters */
	if (p_data == NULL && data_size != 0)
		return PSA_ERROR_INVALID_ARGUMENT;

	handle = rpc_caller_session_begin(this_context->client.session, &request,
					  sizeof(*request_desc), expected_response_length);
	if (!handle)
		goto out;

	/* Populating request descriptor */
	request_desc = (struct secure_storage_request_get *)request;
	request_desc->uid = uid;
	request_desc->data_offset = data_offset;
	request_desc->data_size = data_size;

	rpc_status = rpc_caller_session_invoke(handle, TS_SECURE_STORAGE_OPCODE_GET, &response,
					       &response_length, &service_status);
	if (rpc_status != RPC_SUCCESS || response_length > data_size)
		goto session_end;

	psa_status = service_status;

	/* Filling output parameters */
	if (psa_status != PSA_SUCCESS)
		goto session_end;

	*p_data_length = response_length;
	memcpy(p_data, response, response_length);

session_end:
	rpc_status = rpc_caller_session_end(handle);
	if (psa_status == PSA_SUCCESS && rpc_status != RPC_SUCCESS)
		psa_status = PSA_ERROR_GENERIC_ERROR;

out:
	return psa_status;
}

static psa_status_t secure_storage_client_get_info(void *context,
				uint32_t client_id,
				psa_storage_uid_t uid,
				struct psa_storage_info_t *p_info)
{
	struct secure_storage_client *this_context = (struct secure_storage_client*)context;
	uint8_t *request = NULL;
	uint8_t *response = NULL;
	size_t response_length = 0;
	struct secure_storage_request_get_info *request_desc = NULL;
	struct secure_storage_response_get_info *response_desc = NULL;
	rpc_call_handle handle = 0;
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	service_status_t service_status = 0;
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;

	(void)client_id;

	/* Validating input parameters */
	if (p_info == NULL)
		return PSA_ERROR_INVALID_ARGUMENT;

	handle = rpc_caller_session_begin(this_context->client.session, &request,
					  sizeof(*request_desc), sizeof(*response_desc));
	if (!handle)
		goto out;

	/* Populating request descriptor */
	request_desc = (struct secure_storage_request_get_info *)request;
	request_desc->uid = uid;

	rpc_status = rpc_caller_session_invoke(handle, TS_SECURE_STORAGE_OPCODE_GET_INFO,
					       &response, &response_length, &service_status);
	if (rpc_status != RPC_SUCCESS ||
	   (service_status == PSA_SUCCESS && response_length != sizeof(*response_desc)))
		goto session_end;

	psa_status = service_status;

	if (psa_status == PSA_SUCCESS) {
		response_desc = (struct secure_storage_response_get_info *)response;

		p_info->capacity = response_desc->capacity;
		p_info->size = response_desc->size;
		p_info->flags = response_desc->flags;
	} else {
		p_info->capacity = 0;
		p_info->size = 0;
		p_info->flags = PSA_STORAGE_FLAG_NONE;
	}

session_end:
	rpc_status = rpc_caller_session_end(handle);
	if (psa_status == PSA_SUCCESS && rpc_status != RPC_SUCCESS)
		psa_status = PSA_ERROR_GENERIC_ERROR;

out:
	return psa_status;
}

static psa_status_t secure_storage_client_remove(void *context,
						 uint32_t client_id,
						 psa_storage_uid_t uid)
{
	struct secure_storage_client *this_context = (struct secure_storage_client*)context;
	uint8_t *request = NULL;
	uint8_t *response = NULL;
	size_t response_length = 0;
	struct secure_storage_request_remove *request_desc = NULL;
	rpc_call_handle handle = 0;
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	service_status_t service_status = 0;
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;

	(void)client_id;

	handle = rpc_caller_session_begin(this_context->client.session, &request,
					  sizeof(*request_desc), 0);
	if (!handle)
		goto out;

	/* Populating request descriptor */
	request_desc = (struct secure_storage_request_remove *)request;
	request_desc->uid = uid;

	rpc_status = rpc_caller_session_invoke(handle, TS_SECURE_STORAGE_OPCODE_REMOVE,
					       &response, &response_length, &service_status);
	if (rpc_status == RPC_SUCCESS)
		psa_status = service_status;

	rpc_status = rpc_caller_session_end(handle);
	if (psa_status == PSA_SUCCESS && rpc_status != RPC_SUCCESS)
		psa_status = PSA_ERROR_GENERIC_ERROR;

out:
	return psa_status;
}

static psa_status_t secure_storage_client_create(void *context,
						 uint32_t client_id,
						 uint64_t uid,
						 size_t capacity,
						 uint32_t create_flags)
{
	struct secure_storage_client *this_context = (struct secure_storage_client*)context;
	uint8_t *request = NULL;
	uint8_t *response = NULL;
	size_t response_length = 0;
	struct secure_storage_request_create *request_desc = NULL;
	rpc_call_handle handle = 0;
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	service_status_t service_status = 0;
	psa_status_t psa_status = PSA_SUCCESS;

	(void)client_id;

	handle = rpc_caller_session_begin(this_context->client.session, &request,
					  sizeof(*request_desc), 0);
	if (!handle)
		goto out;

	request_desc = (struct secure_storage_request_create *)request;
	request_desc->uid = uid;
	request_desc->capacity = capacity;
	request_desc->create_flags = create_flags;

	rpc_status = rpc_caller_session_invoke(handle, TS_SECURE_STORAGE_OPCODE_CREATE,
					       &response, &response_length, &service_status);
	if (rpc_status == RPC_SUCCESS)
		psa_status = service_status;

	rpc_status = rpc_caller_session_end(handle);
	if (psa_status == PSA_SUCCESS && rpc_status != RPC_SUCCESS)
		psa_status = PSA_ERROR_GENERIC_ERROR;

out:
	return psa_status;
}

static psa_status_t secure_storage_set_extended(void *context,
                            uint32_t client_id,
                            uint64_t uid,
                            size_t data_offset,
                            size_t data_length,
                            const void *p_data)
{
	struct secure_storage_client *this_context = (struct secure_storage_client*)context;
	uint8_t *request = NULL;
	uint8_t *response = NULL;
	size_t request_length = 0;
	size_t response_length = 0;
	struct secure_storage_request_set_extended *request_desc = NULL;
	rpc_call_handle handle = 0;
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	service_status_t service_status = 0;
	psa_status_t psa_status = PSA_SUCCESS;

	(void)client_id;

	/* Validating input parameters */
	if (p_data == NULL)
		return PSA_ERROR_INVALID_ARGUMENT;

	if (ADD_OVERFLOW(sizeof(*request_desc), data_length, &request_length))
		return PSA_ERROR_INVALID_ARGUMENT;

	handle = rpc_caller_session_begin(this_context->client.session, &request, request_length,
					  0);
	if (!handle)
		return PSA_ERROR_INVALID_ARGUMENT;

	/* Populating request descriptor */
	request_desc = (struct secure_storage_request_set_extended *)request;
	request_desc->uid = uid;
	request_desc->data_offset = data_offset;
	request_desc->data_length = data_length;
	memcpy(&request_desc->p_data, p_data, data_length);

	rpc_status = rpc_caller_session_invoke(handle, TS_SECURE_STORAGE_OPCODE_SET_EXTENDED,
					       &response, &response_length, &service_status);
	if (rpc_status == RPC_SUCCESS)
		psa_status = service_status;

	rpc_status = rpc_caller_session_end(handle);
	if (psa_status == PSA_SUCCESS && rpc_status != RPC_SUCCESS)
		psa_status = PSA_ERROR_GENERIC_ERROR;

	return psa_status;
}

static uint32_t secure_storage_get_support(void *context, uint32_t client_id)
{
	struct secure_storage_client *this_context = (struct secure_storage_client*)context;
	uint8_t *request = NULL;
	uint8_t *response = NULL;
	size_t response_length = 0;
	struct secure_storage_response_get_support *response_desc = NULL;
	rpc_call_handle handle = 0;
	psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
	service_status_t service_status = 0;
	rpc_status_t rpc_status = RPC_ERROR_INTERNAL;
	uint32_t feature_map = 0;

	(void)client_id;

	handle = rpc_caller_session_begin(this_context->client.session, &request, 0,
					  sizeof(*response_desc));
	if (!handle)
		goto out;

	rpc_status = rpc_caller_session_invoke(handle, TS_SECURE_STORAGE_OPCODE_GET_SUPPORT,
					       &response, &response_length, &service_status);
	if (rpc_status != RPC_SUCCESS || response_length < sizeof(*response_desc))
		goto session_end;

	psa_status = service_status;
	if (psa_status != PSA_SUCCESS)
		goto session_end;

	response_desc = (struct secure_storage_response_get_support *)response;
	feature_map = response_desc->support;

session_end:
	rpc_caller_session_end(handle);

out:
	return feature_map;
}


struct storage_backend *secure_storage_client_init(struct secure_storage_client *context,
								struct rpc_caller_session *session)
{
	service_client_init(&context->client, session);

	static const struct storage_backend_interface interface =
	{
		secure_storage_client_set,
		secure_storage_client_get,
		secure_storage_client_get_info,
		secure_storage_client_remove,
		secure_storage_client_create,
		secure_storage_set_extended,
		secure_storage_get_support
	};

	context->backend.context = context;
	context->backend.interface = &interface;

	return &context->backend;
}

void secure_storage_client_deinit(struct secure_storage_client *context)
{
	service_client_deinit(&context->client);
}
