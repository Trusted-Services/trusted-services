/*
 * Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "smm_variable_provider.h"

#include <protocols/rpc/common/packed-c/status.h>
#include <protocols/service/smm_variable/smm_variable_proto.h>
#include <stddef.h>
#include <string.h>

/* Service request handlers */
static rpc_status_t get_variable_handler(void *context, struct rpc_request *req);
static rpc_status_t get_next_variable_name_handler(void *context, struct rpc_request *req);
static rpc_status_t set_variable_handler(void *context, struct rpc_request *req);
static rpc_status_t query_variable_info_handler(void *context, struct rpc_request *req);
static rpc_status_t exit_boot_service_handler(void *context, struct rpc_request *req);
static rpc_status_t set_var_check_property_handler(void *context, struct rpc_request *req);
static rpc_status_t get_var_check_property_handler(void *context, struct rpc_request *req);
static rpc_status_t get_payload_size_handler(void *context, struct rpc_request *req);

/* Handler mapping table for service */
static const struct service_handler handler_table[] = {
	{ SMM_VARIABLE_FUNCTION_GET_VARIABLE, get_variable_handler },
	{ SMM_VARIABLE_FUNCTION_GET_NEXT_VARIABLE_NAME, get_next_variable_name_handler },
	{ SMM_VARIABLE_FUNCTION_SET_VARIABLE, set_variable_handler },
	{ SMM_VARIABLE_FUNCTION_QUERY_VARIABLE_INFO, query_variable_info_handler },
	{ SMM_VARIABLE_FUNCTION_EXIT_BOOT_SERVICE, exit_boot_service_handler },
	{ SMM_VARIABLE_FUNCTION_VAR_CHECK_VARIABLE_PROPERTY_SET, set_var_check_property_handler },
	{ SMM_VARIABLE_FUNCTION_VAR_CHECK_VARIABLE_PROPERTY_GET, get_var_check_property_handler },
	{ SMM_VARIABLE_FUNCTION_GET_PAYLOAD_SIZE, get_payload_size_handler }
};

struct rpc_service_interface *smm_variable_provider_init(struct smm_variable_provider *context,
							 uint32_t owner_id, size_t max_variables,
							 struct storage_backend *persistent_store,
							 struct storage_backend *volatile_store)
{
	struct rpc_service_interface *rpc_interface = NULL;
	const struct rpc_uuid dummy_uuid = { .uuid = { 0 } };

	if (context) {
		service_provider_init(&context->base_provider, context, &dummy_uuid, handler_table,
				      sizeof(handler_table) / sizeof(struct service_handler));

		if (uefi_variable_store_init(&context->variable_store, owner_id, max_variables,
					     persistent_store, volatile_store) == EFI_SUCCESS) {
			rpc_interface = service_provider_get_rpc_interface(&context->base_provider);
		}
	}

	return rpc_interface;
}

void smm_variable_provider_deinit(struct smm_variable_provider *context)
{
	uefi_variable_store_deinit(&context->variable_store);
}

static efi_status_t sanitize_access_variable_param(struct rpc_request *req, size_t *param_len)
{
	efi_status_t efi_status = EFI_INVALID_PARAMETER;
	*param_len = 0;
	const struct rpc_buffer *req_buf = &req->request;

	if (req_buf->data_length >= SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_NAME_OFFSET) {
		const SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *param =
			(const SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *)req_buf->data;

		size_t max_space_for_name =
			req_buf->data_length - SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_NAME_OFFSET;

		if (param->NameSize <= max_space_for_name) {
			*param_len = SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_DATA_OFFSET(param);
			efi_status = EFI_SUCCESS;
		}
	}

	return efi_status;
}

static efi_status_t sanitize_var_check_property_param(struct rpc_request *req, size_t *param_len)
{
	efi_status_t efi_status = EFI_INVALID_PARAMETER;
	*param_len = 0;
	const struct rpc_buffer *req_buf = &req->request;

	if (req_buf->data_length >=
	    SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY_NAME_OFFSET) {
		const SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY *param =
			(const SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY *)req_buf->data;

		size_t max_space_for_name =
			req_buf->data_length -
			SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY_NAME_OFFSET;

		if (param->NameSize <= max_space_for_name) {
			*param_len =
				SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY_TOTAL_SIZE(
					param);
			efi_status = EFI_SUCCESS;
		}
	}

	return efi_status;
}

static rpc_status_t get_variable_handler(void *context, struct rpc_request *req)
{
	struct smm_variable_provider *this_instance = (struct smm_variable_provider *)context;

	size_t param_len = 0;
	efi_status_t efi_status = sanitize_access_variable_param(req, &param_len);

	if (efi_status == EFI_SUCCESS) {
		/* Valid access variable header parameter */
		struct rpc_buffer *resp_buf = &req->response;

		if (resp_buf->size >= param_len) {
			struct rpc_buffer *req_buf = &req->request;
			size_t max_data_len = resp_buf->size - param_len;

			memcpy(resp_buf->data, req_buf->data, param_len);

			efi_status = uefi_variable_store_get_variable(
				&this_instance->variable_store,
				(SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *)resp_buf->data,
				max_data_len, &resp_buf->data_length);
		} else {
			/* Response buffer not big enough */
			efi_status = EFI_BAD_BUFFER_SIZE;
		}
	}

	req->service_status = efi_status;

	return RPC_SUCCESS;
}

static rpc_status_t get_next_variable_name_handler(void *context, struct rpc_request *req)
{
	struct smm_variable_provider *this_instance = (struct smm_variable_provider *)context;

	efi_status_t efi_status = EFI_SUCCESS;
	size_t variable_size = 0;

	/* Valid get next variable name header */
	struct rpc_buffer *resp_buf = &req->response;
	struct rpc_buffer *req_buf = &req->request;

	memcpy(resp_buf->data, req_buf->data, req_buf->data_length);

	efi_status = uefi_variable_store_get_next_variable_name(
		&this_instance->variable_store,
		(SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME *)resp_buf->data,
		&variable_size);

	resp_buf->data_length = variable_size;

	req->service_status = efi_status;

	return RPC_SUCCESS;
}

static rpc_status_t set_variable_handler(void *context, struct rpc_request *req)
{
	struct smm_variable_provider *this_instance = (struct smm_variable_provider *)context;

	size_t param_len = 0;
	efi_status_t efi_status = sanitize_access_variable_param(req, &param_len);

	if (efi_status == EFI_SUCCESS) {
		/* Access variable header is whole.  Check that buffer length can
		 * accommodate the data.
		 */
		struct rpc_buffer *req_buf = &req->request;

		const SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *access_var =
			(const SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *)req_buf->data;

		size_t space_for_data =
			req_buf->data_length -
			SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_DATA_OFFSET(access_var);

		if (access_var->DataSize <= space_for_data) {
			efi_status = uefi_variable_store_set_variable(
				&this_instance->variable_store, access_var);
		} else {
			/* Invalid DataSize */
			efi_status = EFI_INVALID_PARAMETER;
		}
	}

	req->service_status = efi_status;

	return RPC_SUCCESS;
}

static rpc_status_t query_variable_info_handler(void *context, struct rpc_request *req)
{
	efi_status_t efi_status = EFI_INVALID_PARAMETER;
	struct smm_variable_provider *this_instance = (struct smm_variable_provider *)context;

	const struct rpc_buffer *req_buf = &req->request;

	if (req_buf->data_length >= sizeof(SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO)) {
		struct rpc_buffer *resp_buf = &req->response;

		if (resp_buf->size >= req_buf->data_length) {
			memcpy(resp_buf->data, req_buf->data, req_buf->data_length);

			efi_status = uefi_variable_store_query_variable_info(
				&this_instance->variable_store,
				(SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO *)resp_buf->data);

			if (efi_status == EFI_SUCCESS) {
				resp_buf->data_length =
					sizeof(SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO);
			}
		} else {
			/* Reponse buffer not big enough */
			efi_status = EFI_BAD_BUFFER_SIZE;
		}
	}

	req->service_status = efi_status;

	return RPC_SUCCESS;
}

static rpc_status_t exit_boot_service_handler(void *context, struct rpc_request *req)
{
	struct smm_variable_provider *this_instance = (struct smm_variable_provider *)context;

	efi_status_t efi_status =
		uefi_variable_store_exit_boot_service(&this_instance->variable_store);
	req->service_status = efi_status;

	return RPC_SUCCESS;
}

static rpc_status_t set_var_check_property_handler(void *context, struct rpc_request *req)
{
	struct smm_variable_provider *this_instance = (struct smm_variable_provider *)context;

	size_t param_len = 0;
	efi_status_t efi_status = sanitize_var_check_property_param(req, &param_len);

	if (efi_status == EFI_SUCCESS) {
		/* Request parameter structue looks whole */
		struct rpc_buffer *req_buf = &req->request;

		const SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY *check_property =
			(const SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY *)req_buf->data;

		efi_status = uefi_variable_store_set_var_check_property(
			&this_instance->variable_store, check_property);
	}

	req->service_status = efi_status;

	return RPC_SUCCESS;
}

static rpc_status_t get_var_check_property_handler(void *context, struct rpc_request *req)
{
	struct smm_variable_provider *this_instance = (struct smm_variable_provider *)context;

	size_t param_len = 0;
	efi_status_t efi_status = sanitize_var_check_property_param(req, &param_len);

	if (efi_status == EFI_SUCCESS) {
		/* Request parameter structue looks whole */
		struct rpc_buffer *resp_buf = &req->response;

		if (resp_buf->size >= param_len) {
			struct rpc_buffer *req_buf = &req->request;
			memcpy(resp_buf->data, req_buf->data, param_len);
			resp_buf->data_length = param_len;

			efi_status = uefi_variable_store_get_var_check_property(
				&this_instance->variable_store,
				(SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY *)
					resp_buf->data);
		} else {
			/* Reponse buffer not big enough */
			efi_status = EFI_BAD_BUFFER_SIZE;
		}
	}

	req->service_status = efi_status;

	return RPC_SUCCESS;
}

static rpc_status_t get_payload_size_handler(void *context, struct rpc_request *req)
{
	(void)context;

	/* Payload size is constrained by the size of the RPC call buffer.  Because the variable length
	 * name is also carried in the buffer, the maximum payload size depends on the name size.  This
	 * function therefore treats the payload as name + data.
	 */
	size_t payload_size =
		req->request.size - SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_NAME_OFFSET;

	efi_status_t efi_status = EFI_SUCCESS;
	struct rpc_buffer *resp_buf = &req->response;

	if (resp_buf->size >= sizeof(SMM_VARIABLE_COMMUNICATE_GET_PAYLOAD_SIZE)) {
		SMM_VARIABLE_COMMUNICATE_GET_PAYLOAD_SIZE *resp_msg =
			(SMM_VARIABLE_COMMUNICATE_GET_PAYLOAD_SIZE *)resp_buf->data;

		resp_msg->VariablePayloadSize = payload_size;
		resp_buf->data_length = sizeof(SMM_VARIABLE_COMMUNICATE_GET_PAYLOAD_SIZE);
	} else {
		/* Reponse buffer not big enough */
		efi_status = EFI_BAD_BUFFER_SIZE;
	}

	req->service_status = efi_status;

	return RPC_SUCCESS;
}
