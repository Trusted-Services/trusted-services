/*
 * Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "smm_variable_client.h"

#include <cstring>
#include <protocols/rpc/common/packed-c/status.h>

static size_t string_get_size_in_bytes(const std::u16string &string)
{
	return string.size() * sizeof(typename std::u16string::value_type);
}

smm_variable_client::smm_variable_client()
	: session(NULL)
	, m_err_rpc_status(RPC_SUCCESS)
{
}

smm_variable_client::smm_variable_client(struct rpc_caller_session *session)
	: session(session)
	, m_err_rpc_status(RPC_SUCCESS)
{
}

smm_variable_client::~smm_variable_client()
{
}

std::u16string smm_variable_client::to_variable_name(const char16_t *name) const
{
	std::u16string name_var(name);
	name_var.push_back(0);

	return name_var;
}

void smm_variable_client::set_caller_session(struct rpc_caller_session *session)
{
	this->session = session;
}

int smm_variable_client::err_rpc_status() const
{
	return m_err_rpc_status;
}

efi_status_t smm_variable_client::set_variable(const EFI_GUID &guid, const char16_t *name,
					       const std::string data, uint32_t attributes)
{
	return set_variable(guid, to_variable_name(name), data, attributes, 0, 0);
}

efi_status_t smm_variable_client::set_variable(const EFI_GUID &guid, const char16_t *name,
					       const unsigned char* data, size_t data_length,
					       uint32_t attributes)
{
	std::string data_var((const char *)data, data_length);
	return set_variable(guid, to_variable_name(name), data_var, attributes, 0, 0);
}

efi_status_t smm_variable_client::set_variable(const EFI_GUID &guid, const std::u16string &name,
					       const unsigned char *data, size_t data_length,
					       uint32_t attributes)
{
	std::string data_string(reinterpret_cast<char const *>(data), data_length);

	return set_variable(guid, name, data_string, attributes, 0, 0);
}

efi_status_t smm_variable_client::set_variable(const EFI_GUID &guid, const std::u16string &name,
					       const std::string &data, uint32_t attributes)
{
	return set_variable(guid, name, data, attributes, 0, 0);
}

efi_status_t smm_variable_client::set_variable(const EFI_GUID &guid, const std::u16string &name,
					       const std::string &data, uint32_t attributes,
					       size_t override_name_size, size_t override_data_size)
{
	efi_status_t efi_status = EFI_NOT_READY;
	size_t name_size = string_get_size_in_bytes(name);
	size_t data_size = data.size();
	size_t req_len = SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_SIZE(name_size, data_size);

	rpc_call_handle call_handle;
	uint8_t *req_buf;

	call_handle = rpc_caller_session_begin(session, &req_buf, req_len, 0);

	if (call_handle) {
		uint8_t *resp_buf;
		size_t resp_len;
		service_status_t service_status;

		SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *access_var =
			(SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *)req_buf;

		access_var->Guid = guid;
		access_var->NameSize = name_size;
		access_var->DataSize = data_size;
		access_var->Attributes = attributes;

		memcpy(access_var->Name, name.c_str(), name_size);
		memcpy(&req_buf[SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_DATA_OFFSET(access_var)],
		       data.data(), data_size);

		/* To support invalid size testing, use overrides if set */
		if (override_name_size)
			access_var->NameSize = override_name_size;
		if (override_data_size)
			access_var->DataSize = override_data_size;

		m_err_rpc_status = rpc_caller_session_invoke(call_handle,
							     SMM_VARIABLE_FUNCTION_SET_VARIABLE,
							     &resp_buf, &resp_len, &service_status);

		if (m_err_rpc_status == RPC_SUCCESS) {
			efi_status = service_status;
		} else {
			efi_status = rpc_to_efi_status();
		}

		rpc_caller_session_end(call_handle);
	}

	return efi_status;
}

efi_status_t smm_variable_client::get_variable(const EFI_GUID &guid, const char16_t *name,
					       std::string &data)
{
	return get_variable(guid, to_variable_name(name), data, 0, MAX_VAR_DATA_SIZE);
}

efi_status_t smm_variable_client::get_variable(const EFI_GUID &guid, const char16_t *name,
					       std::string &data, size_t override_name_size,
					       size_t max_data_size)
{
	return get_variable(guid, to_variable_name(name), data, override_name_size, max_data_size);
}

efi_status_t smm_variable_client::get_variable(const EFI_GUID &guid, const std::u16string &name,
					       std::string &data)
{
	return get_variable(guid, name, data, 0, MAX_VAR_DATA_SIZE);
}

efi_status_t smm_variable_client::get_variable(const EFI_GUID &guid, const std::u16string &name,
					       std::string &data, size_t override_name_size,
					       size_t max_data_size)
{
	efi_status_t efi_status = EFI_NOT_READY;

	size_t name_size = string_get_size_in_bytes(name);
	size_t req_len = SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_SIZE(name_size, 0);
	size_t resp_len = SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_SIZE(name_size, 0);

	rpc_call_handle call_handle;
	uint8_t *req_buf;

	call_handle = rpc_caller_session_begin(session, &req_buf, req_len, resp_len);

	if (call_handle) {
		uint8_t *resp_buf;
		service_status_t service_status;

		SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *access_var =
			(SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *)req_buf;

		access_var->Guid = guid;
		access_var->NameSize = name_size;
		access_var->DataSize = max_data_size;

		memcpy(access_var->Name, name.c_str(), name_size);

		/* To support invalid size testing, use overrides if set */
		if (override_name_size)
			access_var->NameSize = override_name_size;

		m_err_rpc_status = rpc_caller_session_invoke(call_handle,
							     SMM_VARIABLE_FUNCTION_GET_VARIABLE,
							     &resp_buf, &resp_len, &service_status);

		if (m_err_rpc_status == RPC_SUCCESS) {
			efi_status = service_status;

			if (resp_len >= SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_NAME_OFFSET) {
				access_var = (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *)resp_buf;
				size_t data_size = access_var->DataSize;

				if (resp_len >= SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_TOTAL_SIZE(
							access_var)) {
					if (efi_status == EFI_SUCCESS) {
						const char *data_start = (const char *)&resp_buf
							[SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_DATA_OFFSET(
								access_var)];

						data.assign(data_start, data_size);
					}
				} else if (efi_status == EFI_BUFFER_TOO_SMALL) {
					data.clear();
					data.insert(0, data_size, '!');
				} else {
					efi_status = EFI_PROTOCOL_ERROR;
				}
			}
		} else {
			efi_status = rpc_to_efi_status();
		}

		rpc_caller_session_end(call_handle);
	}

	return efi_status;
}

efi_status_t smm_variable_client::remove_variable(const EFI_GUID &guid, const char16_t *name)
{
	return remove_variable(guid, to_variable_name(name));
}

efi_status_t smm_variable_client::remove_variable(const EFI_GUID &guid, const std::u16string &name)
{
	/* Variable is removed by performing a 'set' with zero length data */
	return set_variable(guid, name, std::string(), 0);
}

efi_status_t smm_variable_client::get_next_variable_name(EFI_GUID &guid, std::u16string &name)
{
	return get_next_variable_name(guid, name, 0);
}

efi_status_t smm_variable_client::query_variable_info(uint32_t attributes,
						      size_t *max_variable_storage_size,
						      size_t *remaining_variable_storage_size,
						      size_t *max_variable_size)
{
	efi_status_t efi_status = EFI_NOT_READY;

	size_t req_len = sizeof(SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO);
	size_t resp_len = sizeof(SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO);
	rpc_call_handle call_handle;
	uint8_t *req_buf;

	call_handle = rpc_caller_session_begin(session, &req_buf, req_len, resp_len);

	if (call_handle) {
		uint8_t *resp_buf;
		size_t resp_len;
		service_status_t service_status;

		SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO *query =
			(SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO *)req_buf;

		query->Attributes = attributes;
		query->MaximumVariableSize = 0;
		query->MaximumVariableStorageSize = 0;
		query->RemainingVariableStorageSize = 0;

		m_err_rpc_status = rpc_caller_session_invoke(
			call_handle, SMM_VARIABLE_FUNCTION_QUERY_VARIABLE_INFO, &resp_buf,
			&resp_len, &service_status);

		if (m_err_rpc_status == RPC_SUCCESS) {
			efi_status = service_status;

			if (efi_status == EFI_SUCCESS) {
				if (resp_len >=
				    sizeof(SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO)) {
					query = (SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO *)
						resp_buf;

					*max_variable_storage_size =
						query->MaximumVariableStorageSize;
					*remaining_variable_storage_size =
						query->RemainingVariableStorageSize;
					*max_variable_size = query->MaximumVariableSize;
				} else {
					efi_status = EFI_PROTOCOL_ERROR;
				}
			} else {
				efi_status = EFI_PROTOCOL_ERROR;
			}
		} else {
			efi_status = rpc_to_efi_status();
		}

		rpc_caller_session_end(call_handle);
	}

	return efi_status;
}

efi_status_t smm_variable_client::get_next_variable_name(EFI_GUID &guid, std::u16string &name,
							 size_t override_name_size)
{
	efi_status_t efi_status = EFI_NOT_READY;

	size_t name_size = string_get_size_in_bytes(name);
	size_t req_len = SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME_SIZE(name_size);
	size_t resp_len = SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME_SIZE(name_size);

	rpc_call_handle call_handle;
	uint8_t *req_buf;

	call_handle = rpc_caller_session_begin(session, &req_buf, req_len, resp_len);

	if (call_handle) {
		uint8_t *resp_buf;
		size_t resp_len;
		service_status_t service_status;

		SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME *next_var =
			(SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME *)req_buf;

		next_var->Guid = guid;
		next_var->NameSize = name_size;

		memcpy(next_var->Name, name.c_str(), name_size);

		/* To support invalid size testing, use overrides if set */
		if (override_name_size)
			next_var->NameSize = override_name_size;

		m_err_rpc_status = rpc_caller_session_invoke(
			call_handle, SMM_VARIABLE_FUNCTION_GET_NEXT_VARIABLE_NAME, &resp_buf,
			&resp_len, &service_status);

		if (m_err_rpc_status == RPC_SUCCESS) {
			efi_status = service_status;

			if (efi_status == EFI_SUCCESS) {
				efi_status = EFI_PROTOCOL_ERROR;

				if (resp_len >=
				    SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME_NAME_OFFSET) {
					next_var =
						(SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME *)
							resp_buf;

					if (resp_len >=
					    SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME_TOTAL_SIZE(
						    next_var)) {
						guid = next_var->Guid;
						from_variable_name(next_var->Name,
									  next_var->NameSize, name);

						efi_status = EFI_SUCCESS;
					}
				}
			}
		} else {
			efi_status = rpc_to_efi_status();
		}

		rpc_caller_session_end(call_handle);
	}

	return efi_status;
}

efi_status_t smm_variable_client::exit_boot_service()
{
	efi_status_t efi_status = EFI_NOT_READY;

	rpc_call_handle call_handle;
	uint8_t *req_buf;

	call_handle = rpc_caller_session_begin(session, &req_buf, 0, 0);

	if (call_handle) {
		uint8_t *resp_buf;
		size_t resp_len;
		service_status_t service_status;

		m_err_rpc_status = rpc_caller_session_invoke(
			call_handle, SMM_VARIABLE_FUNCTION_EXIT_BOOT_SERVICE, &resp_buf, &resp_len,
			&service_status);

		if (m_err_rpc_status == RPC_SUCCESS) {
			efi_status = service_status;
		} else {
			efi_status = rpc_to_efi_status();
		}

		rpc_caller_session_end(call_handle);
	}

	return efi_status;
}

efi_status_t smm_variable_client::set_var_check_property(const EFI_GUID &guid, const char16_t *name,
					    const VAR_CHECK_VARIABLE_PROPERTY &check_property)
{
	return set_var_check_property(guid, to_variable_name(name), check_property, 0);
}

efi_status_t smm_variable_client::set_var_check_property(const EFI_GUID &guid, const char16_t *name,
					    const VAR_CHECK_VARIABLE_PROPERTY &check_property,
					    size_t override_name_size)
{
	return set_var_check_property(guid, to_variable_name(name), check_property,
		override_name_size);
}


efi_status_t
smm_variable_client::set_var_check_property(const EFI_GUID &guid, const std::u16string &name,
					    const VAR_CHECK_VARIABLE_PROPERTY &check_property)
{
	return set_var_check_property(guid, name, check_property, 0);
}

efi_status_t
smm_variable_client::set_var_check_property(const EFI_GUID &guid, const std::u16string &name,
					    const VAR_CHECK_VARIABLE_PROPERTY &check_property,
					    size_t override_name_size)
{
	efi_status_t efi_status = EFI_NOT_READY;

	size_t name_size = string_get_size_in_bytes(name);
	size_t req_len = SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY_SIZE(name_size);
	size_t resp_len = SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY_SIZE(name_size);

	rpc_call_handle call_handle;
	uint8_t *req_buf;

	call_handle = rpc_caller_session_begin(session, &req_buf, req_len, resp_len);

	if (call_handle) {
		uint8_t *resp_buf;
		size_t resp_len;
		service_status_t service_status;

		SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY *req_msg =
			(SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY *)req_buf;

		req_msg->Guid = guid;
		req_msg->NameSize = name_size;
		req_msg->VariableProperty = check_property;

		memcpy(req_msg->Name, name.data(), name_size);

		/* To support invalid size testing, use override if set */
		if (override_name_size)
			req_msg->NameSize = override_name_size;

		m_err_rpc_status = rpc_caller_session_invoke(
			call_handle, SMM_VARIABLE_FUNCTION_VAR_CHECK_VARIABLE_PROPERTY_SET,
			&resp_buf, &resp_len, &service_status);

		if (m_err_rpc_status == RPC_SUCCESS) {
			efi_status = service_status;
		} else {
			efi_status = rpc_to_efi_status();
		}

		rpc_caller_session_end(call_handle);
	}

	return efi_status;
}

efi_status_t smm_variable_client::get_var_check_property(const EFI_GUID &guid, const char16_t *name,
					VAR_CHECK_VARIABLE_PROPERTY &check_property)
{
	return get_var_check_property(guid, to_variable_name(name), check_property, 0);
}

efi_status_t smm_variable_client::get_var_check_property(const EFI_GUID &guid, const char16_t *name,
					VAR_CHECK_VARIABLE_PROPERTY &check_property,
					size_t override_name_size)
{
	return get_var_check_property(guid, to_variable_name(name), check_property,
		override_name_size);
}

efi_status_t
smm_variable_client::get_var_check_property(const EFI_GUID &guid, const std::u16string &name,
					    VAR_CHECK_VARIABLE_PROPERTY &check_property)
{
	return get_var_check_property(guid, name, check_property, 0);
}

efi_status_t
smm_variable_client::get_var_check_property(const EFI_GUID &guid, const std::u16string &name,
					    VAR_CHECK_VARIABLE_PROPERTY &check_property,
					    size_t override_name_size)
{
	efi_status_t efi_status = EFI_NOT_READY;

	size_t name_size = string_get_size_in_bytes(name);
	size_t req_len = SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY_SIZE(name_size);
	size_t resp_len = SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY_SIZE(name_size);

	rpc_call_handle call_handle;
	uint8_t *req_buf;

	call_handle = rpc_caller_session_begin(session, &req_buf, req_len, resp_len);

	if (call_handle) {
		uint8_t *resp_buf;
		size_t resp_len;
		service_status_t service_status;

		SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY *req_msg =
			(SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY *)req_buf;

		req_msg->Guid = guid;
		req_msg->NameSize = name_size;

		memcpy(req_msg->Name, name.data(), name_size);

		/* To support invalid size testing, use overrides if set */
		if (override_name_size)
			req_msg->NameSize = override_name_size;

		m_err_rpc_status = rpc_caller_session_invoke(
			call_handle, SMM_VARIABLE_FUNCTION_VAR_CHECK_VARIABLE_PROPERTY_GET,
			&resp_buf, &resp_len, &service_status);

		if (m_err_rpc_status == RPC_SUCCESS) {
			efi_status = service_status;

			if (efi_status == EFI_SUCCESS) {
				efi_status = EFI_PROTOCOL_ERROR;

				if (resp_len >=
				    SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY_NAME_OFFSET) {
					SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY *resp_msg =
						(SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY
							 *)resp_buf;

					if (resp_len >=
					    SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY_TOTAL_SIZE(
						    resp_msg)) {
						check_property = resp_msg->VariableProperty;
						efi_status = EFI_SUCCESS;
					}
				}
			}
		} else {
			efi_status = rpc_to_efi_status();
		}

		rpc_caller_session_end(call_handle);
	}

	return efi_status;
}

efi_status_t smm_variable_client::get_payload_size(size_t &payload_size)
{
	efi_status_t efi_status = EFI_NOT_READY;

	size_t req_len = 0;
	size_t resp_len = sizeof(SMM_VARIABLE_COMMUNICATE_GET_PAYLOAD_SIZE);
	rpc_call_handle call_handle;
	uint8_t *req_buf;

	call_handle = rpc_caller_session_begin(session, &req_buf, req_len, resp_len);

	if (call_handle) {
		uint8_t *resp_buf;
		size_t resp_len;
		service_status_t service_status;

		m_err_rpc_status = rpc_caller_session_invoke(call_handle,
							     SMM_VARIABLE_FUNCTION_GET_PAYLOAD_SIZE,
							     &resp_buf, &resp_len, &service_status);

		if (m_err_rpc_status == RPC_SUCCESS) {
			efi_status = service_status;

			if (efi_status == EFI_SUCCESS) {
				if (resp_len >= sizeof(SMM_VARIABLE_COMMUNICATE_GET_PAYLOAD_SIZE)) {
					SMM_VARIABLE_COMMUNICATE_GET_PAYLOAD_SIZE *resp_msg =
						(SMM_VARIABLE_COMMUNICATE_GET_PAYLOAD_SIZE *)
							resp_buf;

					payload_size = resp_msg->VariablePayloadSize;
				} else {
					efi_status = EFI_PROTOCOL_ERROR;
				}
			}
		} else {
			efi_status = rpc_to_efi_status();
		}

		rpc_caller_session_end(call_handle);
	}

	return efi_status;
}

efi_status_t smm_variable_client::rpc_to_efi_status() const
{
	efi_status_t efi_status = EFI_INVALID_PARAMETER;

	switch (m_err_rpc_status) {
	case RPC_ERROR_INTERNAL:
		efi_status = EFI_DEVICE_ERROR;
		break;
	case RPC_ERROR_INVALID_VALUE:
		efi_status = EFI_INVALID_PARAMETER;
		break;
	case RPC_ERROR_NOT_FOUND:
		efi_status = EFI_UNSUPPORTED;
		break;
	case RPC_ERROR_INVALID_STATE:
		efi_status = EFI_NOT_READY;
		break;
	case RPC_ERROR_TRANSPORT_LAYER:
		efi_status = EFI_PROTOCOL_ERROR;
		break;
	case RPC_ERROR_INVALID_REQUEST_BODY:
		efi_status = EFI_PROTOCOL_ERROR;
		break;
	case RPC_ERROR_INVALID_RESPONSE_BODY:
		efi_status = EFI_DEVICE_ERROR;
		break;
	case RPC_ERROR_RESOURCE_FAILURE:
		efi_status = EFI_OUT_OF_RESOURCES;
		break;
	default:
		break;
	}

	return efi_status;
}

void smm_variable_client::from_variable_name(const int16_t *var_name,
							   size_t name_size, std::u16string &result)
{
	size_t num_chars = name_size / sizeof(int16_t);
	result.assign((const char16_t *) var_name, num_chars);
}
