/*
 * Copyright (c) 2021-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/*
 * This file implements the authenticated variable handling defined by UEFI standard.
 * The UEFI comments refer to v2.9 of the specification:
 *     https://uefi.org/sites/default/files/resources/UEFI_Spec_2_9_2021_03_18.pdf
 */
#include "uefi_variable_store.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "trace.h"
#include "util.h"
#include <service/locator/sp/ffa/spffa_service_context.h>
#include "variable_checker.h"
#include "variable_index_iterator.h"

#if defined(UEFI_AUTH_VAR)
#include "psa/crypto.h"
#include "service/crypto/client/psa/crypto_client.h"
#endif

static psa_status_t get_active_variable_uid(struct uefi_variable_store *context,
					    uint64_t *active_index_uid, uint32_t *counter);

static efi_status_t load_variable_index(struct uefi_variable_store *context);

static efi_status_t sync_variable_index(struct uefi_variable_store *context);

static efi_status_t check_capabilities(const SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *var);

static efi_status_t check_access_permitted(const struct uefi_variable_store *context,
					   const struct variable_info *info);

static efi_status_t
check_access_permitted_on_set(const struct uefi_variable_store *context,
			      const struct variable_info *info,
			      const SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *var);

#if defined(UEFI_AUTH_VAR)

/* Creating a map of the EFI SMM variable for easier access */
typedef struct {
	const SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *smm_variable;
	const uint8_t *smm_variable_data;
	const EFI_VARIABLE_AUTHENTICATION_2 *efi_auth_descriptor;
	size_t efi_auth_descriptor_len;
	size_t efi_auth_descriptor_certdata_len;
	const uint8_t *payload;
	size_t payload_len;
	const EFI_SIGNATURE_LIST *efi_signature_list;
} efi_data_map;

static bool init_efi_data_map(const SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *var,
			      bool with_auth_hdr, efi_data_map *map);

static void create_smm_variable(SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE **variable,
				size_t name_size, size_t data_size, const uint8_t *name,
				EFI_GUID *guid);

static bool calc_variable_hash(const efi_data_map *var_map, uint8_t *hash_buffer,
			       size_t hash_buffer_size, size_t *hash_len);

static efi_status_t
select_verification_keys(const efi_data_map new_var, EFI_GUID global_variable_guid,
			 EFI_GUID security_database_guid, uint64_t maximum_variable_size,
			 SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE **allowed_key_store_variables);

static efi_status_t verify_var_by_key_var(const efi_data_map *new_var,
					  const efi_data_map *key_store_var,
					  const uint8_t *hash_buffer, size_t hash_len);

static bool is_private_auth_var(SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *var);

static efi_status_t authenticate_variable(const struct uefi_variable_store *context,
					  EFI_TIME *stored_timestamp,
					  uint8_t (*fingerprint)[FINGERPRINT_SIZE],
					  bool new_variable,
					  SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *var);

static efi_status_t authenticate_secure_boot_variable(const struct uefi_variable_store *context,
						      efi_data_map* var_map,
						      uint8_t* hash_buffer,
						      size_t hash_len,
						      uint64_t max_variable_size);

static efi_status_t authenticate_private_variable(const struct uefi_variable_store *context,
						  efi_data_map* var_map,
						  uint8_t* hash_buffer,
						  size_t hash_len,
						  uint64_t max_variable_size,
						  bool new_variable,
						  uint8_t (*fingerprint)[FINGERPRINT_SIZE]);
#endif

static efi_status_t store_variable_data(const struct uefi_variable_store *context,
					const struct variable_info *info,
					const SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *var);

static efi_status_t remove_variable_data(const struct uefi_variable_store *context,
					 const struct variable_info *info);

static efi_status_t load_variable_data(const struct uefi_variable_store *context,
				       const struct variable_info *info,
				       SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *var,
				       size_t max_data_len);

static psa_status_t store_overwrite(struct delegate_variable_store *delegate_store,
				    uint32_t client_id, uint64_t uid, size_t data_length,
				    const void *data);

static psa_status_t store_append_write(struct delegate_variable_store *delegate_store,
				       uint32_t client_id, uint64_t uid, size_t data_length,
				       const void *data);

static void purge_orphan_index_entries(const struct uefi_variable_store *context);

static struct delegate_variable_store *
select_delegate_store(const struct uefi_variable_store *context, uint32_t attributes);

static size_t space_used(const struct uefi_variable_store *context, uint32_t attributes,
			 struct storage_backend *storage_backend);

static efi_status_t psa_to_efi_storage_status(psa_status_t psa_status);

static efi_status_t check_name_terminator(const int16_t *name, size_t name_size);

#if defined(UEFI_AUTH_VAR)
static bool compare_name_to_key_store_name(const int16_t *name1, size_t size1,
					   const uint16_t *name2, size_t size2);
#endif

/* Private UID for storing the variable index */
#define SMM_VARIABLE_INDEX_STORAGE_A_UID UINT64_C(0x8000000000000001)
#define SMM_VARIABLE_INDEX_STORAGE_B_UID UINT64_C(0x8000000000000002)

_Static_assert(SMM_VARIABLE_INDEX_STORAGE_A_UID != SMM_VARIABLE_INDEX_STORAGE_B_UID,
	       "SMM_VARIABLE_INDEX_STORAGE_A_UID must not be the same value as "
	       "SMM_VARIABLE_INDEX_STORAGE_B_UID");

/* Default maximum variable size -
 * may be overridden using uefi_variable_store_set_storage_limits()
 */
#ifndef DEFAULT_MAX_VARIABLE_SIZE
#define DEFAULT_MAX_VARIABLE_SIZE (4096)
#endif

_Static_assert(DEFAULT_MAX_VARIABLE_SIZE <= RPC_CALLER_SESSION_SHARED_MEMORY_SIZE,
	       "Maximum UEFI variable size must not exceed RPC buffer size. please increase " \
	       "RPC_CALLER_SESSION_SHARED_MEMORY_SIZE or decrease DEFAULT_MAX_VARIABLE_SIZE");

efi_status_t uefi_variable_store_init(struct uefi_variable_store *context, uint32_t owner_id,
				      size_t max_variables,
				      struct storage_backend *persistent_store,
				      struct storage_backend *volatile_store)
{
	efi_status_t status = EFI_SUCCESS;

	/* Initialise persistent store defaults */
	context->persistent_store.is_nv = true;
	context->persistent_store.max_variable_size = DEFAULT_MAX_VARIABLE_SIZE;
	context->persistent_store.total_capacity = DEFAULT_MAX_VARIABLE_SIZE * max_variables;
	context->persistent_store.storage_backend = persistent_store;

	/* Initialise volatile store defaults */
	context->volatile_store.is_nv = false;
	context->volatile_store.max_variable_size = DEFAULT_MAX_VARIABLE_SIZE;
	context->volatile_store.total_capacity = DEFAULT_MAX_VARIABLE_SIZE * max_variables;
	context->volatile_store.storage_backend = volatile_store;

	context->owner_id = owner_id;
	context->is_boot_service = true;

	status = variable_index_init(&context->variable_index, max_variables);

	if (status == EFI_SUCCESS) {
		/* Allocate a buffer for synchronizing the variable index with the persistent store */
		context->index_sync_buffer_size =
			variable_index_max_dump_size(&context->variable_index);
		context->index_sync_buffer = NULL;

		if (context->index_sync_buffer_size) {
			context->index_sync_buffer = malloc(context->index_sync_buffer_size);
			status = (context->index_sync_buffer) ? EFI_SUCCESS : EFI_OUT_OF_RESOURCES;
		} else {
			EMSG("Variable store must be capable of storing at least one variable");
			return EFI_INVALID_PARAMETER;
		}

		/* Load the variable index with NV variable info from the persistent store */
		if (context->index_sync_buffer) {
			status = load_variable_index(context);

			if (status == EFI_SUCCESS)
				purge_orphan_index_entries(context);
		}
	}

	return status;
}

void uefi_variable_store_deinit(struct uefi_variable_store *context)
{
	variable_index_deinit(&context->variable_index);

	free(context->index_sync_buffer);
	context->index_sync_buffer = NULL;
}

void uefi_variable_store_set_storage_limits(const struct uefi_variable_store *context,
					    uint32_t attributes, size_t total_capacity,
					    size_t max_variable_size)
{
	struct delegate_variable_store *delegate_store = select_delegate_store(context, attributes);

	delegate_store->total_capacity = total_capacity;
	delegate_store->max_variable_size = max_variable_size;
}

efi_status_t uefi_variable_store_set_variable(const struct uefi_variable_store *context,
					      const SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *var)
{
	bool should_sync_index = false;
	EFI_TIME stored_timestamp = { 0 };
	uint8_t fingerprint[FINGERPRINT_SIZE] = { 0 };

	/* Validate incoming request */
	efi_status_t status = check_name_terminator(var->Name, var->NameSize);

	if (status != EFI_SUCCESS)
		return status;

	status = check_capabilities(var);
	if (status != EFI_SUCCESS)
		return status;

	/* Find an existing entry in the variable index or add a new one */
	struct variable_info *info =
		variable_index_find(&context->variable_index, &var->Guid, var->NameSize, var->Name);

	if (!info) {
		/* If it is a delete request and the target does not exist return error */
		if (var->DataSize) {
			info = variable_index_add_entry(&context->variable_index, &var->Guid,
							var->NameSize, var->Name);
		} else {
			return EFI_NOT_FOUND;
		}

		if (!info)
			return EFI_OUT_OF_RESOURCES;
	}

	/* Save the timestamp and fingerprints into a buffer, which can be overwritten later */
	memcpy(&stored_timestamp, &info->metadata.timestamp, sizeof(EFI_TIME));
	memcpy(&fingerprint, &info->metadata.fingerprint, FINGERPRINT_SIZE);

	/* Control access */
	status = check_access_permitted_on_set(context, info, var);

	if (status == EFI_SUCCESS) {
		/* Access permitted */
		if (info->is_variable_set) {
#if defined(UEFI_AUTH_VAR)
			/*
			 * Check if the variable needs authentication.
			 * The authentication header is not needed after this point so
			 * it is deleted from the variable.
			 */
			if (info->metadata.attributes &
			    EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) {
				status = authenticate_variable(
					context, &stored_timestamp,
					&fingerprint, false,
					(SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *)var);

				if (status != EFI_SUCCESS)
					return status;
			}
#endif

			/**
			 *
			 * UEFI: Page 245
			 * Setting a data variable with no access attributes
			 * causes it to be deleted.
			 *
			 * UEFI: Page 245
			 * Unless the EFI_VARIABLE_APPEND_WRITE,
			 * EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS, or
			 * EFI_VARIABLE_ENHANCED_AUTHENTICATED_WRITE_ACCESS attribute is set,
			 * setting a data variable with zero DataSize specified, causes it to
			 * be deleted.
			 *
			 * UEFI: Page 247
			 * To delete a variable created with the
			 * EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS attribute,
			 * SetVariable must be used with attributes matching the existing
			 * variable and the DataSize set to the size of the AuthInfo descriptor.
			 * The Data buffer must contain an instance of the AuthInfo descriptor
			 * which will be validated according to the steps in the appropriate
			 * section above referring to updates of Authenticated variables. An
			 * attempt to delete a variable created with the
			 * EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS attribute for
			 * which the prescribed AuthInfo validation fails or when called using
			 * DataSize of zero will fail with an EFI_SECURITY_VIOLATION status.
			 *
			 * Remarks:
			 *   In case of EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS, the
			 *      authentication header is already removed from the data at this
			 *      point, so the size will be zero at this point. That is why this
			 *      condition is removed from the check below.
			 */

			if (!(var->Attributes &
			      (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)) ||
			    (!(var->Attributes & (EFI_VARIABLE_APPEND_WRITE |
						  EFI_VARIABLE_ENHANCED_AUTHENTICATED_ACCESS)) &&
			     !var->DataSize)) {
				/* It's a remove operation - for a remove, the variable
				 * data must be removed from the storage backend before
				 * modifying and syncing the variable index.  This ensures
				 * that it's never possible for an object to exist within
				 * the storage backend without a corresponding index entry.
				 */
				remove_variable_data(context, info);
				variable_index_clear_variable(&context->variable_index, info);

				should_sync_index = (var->Attributes & EFI_VARIABLE_NON_VOLATILE);
			} else {
				/**
				 * UEFI: Page 245
				 * If a preexisting variable is rewritten with different attributes,
				 * SetVariable() shall not modify the variable and shall return
				 * EFI_INVALID_PARAMETER. The only  exception to this is when
				 * the only attribute differing is EFI_VARIABLE_APPEND_WRITE.
				 */
				if ((info->metadata.attributes | EFI_VARIABLE_APPEND_WRITE) !=
				    (var->Attributes | EFI_VARIABLE_APPEND_WRITE)) {
					return EFI_INVALID_PARAMETER;
				}

				/* It's a set operation where variable data is potentially
				 * being overwritten or extended.
				 */
				if ((var->Attributes & ~EFI_VARIABLE_APPEND_WRITE) !=
				    info->metadata.attributes) {
					/* Modifying attributes is forbidden */
					return EFI_INVALID_PARAMETER;
				}
			}
		} else {
#if defined(UEFI_AUTH_VAR)
			/*
			 * Check if the variable needs authentication.
			 * The authentication header is not needed after this point so
			 * it is deleted from the variable.
			 */
			if (var->Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) {
				status = authenticate_variable(
					context, &stored_timestamp,
					&fingerprint, true,
					(SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *)var);

				if (status != EFI_SUCCESS)
					return status;
			}
#endif

			if (var->DataSize) {
				/*  It's a request to create a new variable */
				variable_index_set_variable(info, var->Attributes);
				should_sync_index = (var->Attributes & EFI_VARIABLE_NON_VOLATILE);
			} else {
				/* Attempting to remove a non-existent variable
				 * This part shall never be reached, because covered in
				 * earlier check. Left here for safety reasons.
				 */
				EMSG("Attempting to remove a non-existent variable");
				return EFI_NOT_FOUND;
			}
		}

		/* The order of these operations is important. For an update
		 * or create operation, The variable index is always synchronized
		 * to NV storage first, then the variable data is stored. If the
		 * data store operation fails or doesn't happen due to a power failure,
		 * the inconistency between the variable index and the persistent
		 * store is detectable and may be corrected by purging the corresponding
		 * index entry.
		 */
		if (should_sync_index)
			status = sync_variable_index((struct uefi_variable_store *)context);

		/* Store any variable data to the storage backend with the updated metadata */
		if (info->is_variable_set && (status == EFI_SUCCESS)) {
			memcpy(&info->metadata.timestamp, &stored_timestamp, sizeof(EFI_TIME));
			memcpy(&info->metadata.fingerprint, &fingerprint, FINGERPRINT_SIZE);
			status = store_variable_data(context, info, var);
		}
	}

	variable_index_remove_unused_entry(&context->variable_index, info);

	return status;
}

efi_status_t uefi_variable_store_get_variable(const struct uefi_variable_store *context,
					      SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *var,
					      size_t max_data_len, size_t *total_length)
{
	efi_status_t status = check_name_terminator(var->Name, var->NameSize);

	if (status != EFI_SUCCESS)
		return status;

	status = EFI_NOT_FOUND;
	*total_length = 0;

	const struct variable_info *info =
		variable_index_find(&context->variable_index, &var->Guid, var->NameSize, var->Name);

	if (info && info->is_variable_set) {
		/* Variable already exists */
		status = check_access_permitted(context, info);

		if (status == EFI_SUCCESS) {
			status = load_variable_data(context, info, var, max_data_len);
			var->Attributes = info->metadata.attributes;

			if (status == EFI_SUCCESS)
				*total_length =
					SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_TOTAL_SIZE(var);
			else if (status == EFI_BUFFER_TOO_SMALL)
				*total_length =
					SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_DATA_OFFSET(var);
		}
	}

	return status;
}

efi_status_t
uefi_variable_store_get_next_variable_name(const struct uefi_variable_store *context,
					   SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME *cur,
					   size_t *total_length)
{
	efi_status_t status = EFI_SUCCESS;
	size_t buffer_size = 0;

	if (!cur)
		return EFI_INVALID_PARAMETER;
	/*
	 * NameSize is set to the buffer size to store the names,
	 * let's calculate the size actually being used.
	 */
	buffer_size = cur->NameSize;
	for (int i = 0; i < buffer_size / sizeof(int16_t); i++) {
		if (cur->Name[i] == 0) {
			/* With null terminator */
			cur->NameSize = 2*(i+1);
			break;
		}
	}

	status = check_name_terminator(cur->Name, cur->NameSize);

	if (status != EFI_SUCCESS)
		return status;

	*total_length = 0;

	while (1) {
		const struct variable_info *info = variable_index_find_next(
			&context->variable_index, &cur->Guid, cur->NameSize, cur->Name, &status);

		if (info && (status == EFI_SUCCESS)) {
			if (info->metadata.name_size <= buffer_size) {
				cur->Guid = info->metadata.guid;
				cur->NameSize = info->metadata.name_size;
				memcpy(cur->Name, info->metadata.name, info->metadata.name_size);

				/*
				 * Check if variable is accessible (e.g boot variable is not
				 * accessible at runtime)
				 */
				status = check_access_permitted(context, info);

				if (status == EFI_SUCCESS)
					break;
			} else {
				/* The VariableNameSize is updated to reflect the size of buffer needed */
				cur->NameSize = info->metadata.name_size;
				memset(cur->Name, 0, buffer_size);
				memset(&cur->Guid, 0, sizeof(EFI_GUID));
				status = EFI_BUFFER_TOO_SMALL;
				break;
			}

		} else {
			/* Do not hide original error if there is any */
			if (status == EFI_SUCCESS)
				status = EFI_NOT_FOUND;

			memset(cur->Name, 0, buffer_size);
			memset(&cur->Guid, 0, sizeof(EFI_GUID));
			cur->NameSize = 0;
			break;
		}
	}

	if (status == EFI_SUCCESS) {
		/* Store everything including the name */
		*total_length =
			SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME_TOTAL_SIZE(
				cur);
	} else {
		/* Do not store the name, only the size */
		*total_length =
			SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME_NAME_OFFSET;
	}
	return status;
}

efi_status_t
uefi_variable_store_query_variable_info(const struct uefi_variable_store *context,
					SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO *var_info)
{
	struct delegate_variable_store *delegate_store =
		select_delegate_store(context, var_info->Attributes);

	size_t total_used =
		space_used(context, var_info->Attributes, delegate_store->storage_backend);

	var_info->MaximumVariableSize = delegate_store->max_variable_size;
	var_info->MaximumVariableStorageSize = delegate_store->total_capacity;
	var_info->RemainingVariableStorageSize =
		(total_used < delegate_store->total_capacity) ?
			delegate_store->total_capacity - total_used :
			0;

	return EFI_SUCCESS;
}

efi_status_t uefi_variable_store_exit_boot_service(struct uefi_variable_store *context)
{
	context->is_boot_service = false;
	return EFI_SUCCESS;
}

efi_status_t uefi_variable_store_set_var_check_property(
	struct uefi_variable_store *context,
	const SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY *property)
{
	efi_status_t status = check_name_terminator(property->Name, property->NameSize);

	if (status != EFI_SUCCESS)
		return status;

	/* Find in index or create a new entry */
	struct variable_info *info = variable_index_find(&context->variable_index, &property->Guid,
							 property->NameSize, property->Name);

	if (!info) {
		info = variable_index_add_entry(&context->variable_index, &property->Guid,
						property->NameSize, property->Name);

		if (!info)
			return EFI_OUT_OF_RESOURCES;
	}

	/* Applying check constraints to an existing variable that may have
	 * constraints already set.  These could constrain the setting of
	 * the constraints.
	 */
	struct variable_constraints constraints = info->check_constraints;

	status = variable_checker_set_constraints(&constraints, info->is_constraints_set,
						  &property->VariableProperty);

	if (status == EFI_SUCCESS) {
		variable_index_set_constraints(info, &constraints);
		status = sync_variable_index(context);
	}

	variable_index_remove_unused_entry(&context->variable_index, info);

	return status;
}

efi_status_t uefi_variable_store_get_var_check_property(
	struct uefi_variable_store *context,
	SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY *property)
{
	efi_status_t status = check_name_terminator(property->Name, property->NameSize);

	if (status != EFI_SUCCESS)
		return status;

	status = EFI_NOT_FOUND;

	const struct variable_info *info = variable_index_find(
		&context->variable_index, &property->Guid, property->NameSize, property->Name);

	if (info && info->is_constraints_set) {
		variable_checker_get_constraints(&info->check_constraints,
						 &property->VariableProperty);

		status = EFI_SUCCESS;
	}

	return status;
}

/* Checks which index contains the latest data, which shall be loaded */
static psa_status_t get_active_variable_uid(struct uefi_variable_store *context,
					    uint64_t *active_index_uid, uint32_t *counter)
{
	uint32_t counter_A = 0;
	uint32_t counter_B = 0;
	size_t data_len = 0;
	psa_status_t psa_status_A = PSA_SUCCESS;
	psa_status_t psa_status_B = PSA_SUCCESS;
	struct storage_backend *persistent_store = context->persistent_store.storage_backend;

	/* Set default value for the case when the index does not exist yet */
	*active_index_uid = SMM_VARIABLE_INDEX_STORAGE_A_UID;
	*counter = 0;

	if (persistent_store) {
		psa_status_A = persistent_store->interface->get(persistent_store->context,
								context->owner_id,
								SMM_VARIABLE_INDEX_STORAGE_A_UID, 0,
								sizeof(counter_A), &counter_A,
								&data_len);

		if (psa_status_A == PSA_SUCCESS && data_len == 0) {
			psa_status_A = persistent_store->interface->remove(
				persistent_store->context, context->owner_id,
				SMM_VARIABLE_INDEX_STORAGE_A_UID);

			if (psa_status_A == PSA_SUCCESS)
				psa_status_A = PSA_ERROR_DOES_NOT_EXIST;
			else {
				EMSG("Erronous state of variable index");
				return PSA_ERROR_STORAGE_FAILURE;
			}
		}

		psa_status_B = persistent_store->interface->get(persistent_store->context,
								context->owner_id,
								SMM_VARIABLE_INDEX_STORAGE_B_UID, 0,
								sizeof(counter_B), &counter_B,
								&data_len);

		if (psa_status_B == PSA_SUCCESS && data_len == 0) {
			psa_status_B = persistent_store->interface->remove(
				persistent_store->context, context->owner_id,
				SMM_VARIABLE_INDEX_STORAGE_B_UID);

			if (psa_status_B == PSA_SUCCESS)
				psa_status_B = PSA_ERROR_DOES_NOT_EXIST;
			else {
				EMSG("Erronous state of variable index");
				return PSA_ERROR_STORAGE_FAILURE;
			}
		}

		if ((psa_status_A != PSA_SUCCESS && psa_status_A != PSA_ERROR_DOES_NOT_EXIST) ||
		    (psa_status_B != PSA_SUCCESS && psa_status_B != PSA_ERROR_DOES_NOT_EXIST))
			return PSA_ERROR_STORAGE_FAILURE;

		if (psa_status_A == PSA_ERROR_DOES_NOT_EXIST) {
			if (psa_status_B == PSA_ERROR_DOES_NOT_EXIST)
				return PSA_ERROR_DOES_NOT_EXIST;

			*active_index_uid = SMM_VARIABLE_INDEX_STORAGE_B_UID;
			*counter = counter_B;

			return PSA_SUCCESS;
		} else if (psa_status_B == PSA_ERROR_DOES_NOT_EXIST) {
			*active_index_uid = SMM_VARIABLE_INDEX_STORAGE_A_UID;
			*counter = counter_A;

			return PSA_SUCCESS;
		}

		if (counter_A + 1 == counter_B) {
			*active_index_uid = SMM_VARIABLE_INDEX_STORAGE_B_UID;
			*counter = counter_B;
			return PSA_SUCCESS;
		} else if (counter_B + 1 == counter_A) {
			*active_index_uid = SMM_VARIABLE_INDEX_STORAGE_A_UID;
			*counter = counter_A;
			return PSA_SUCCESS;
		} else {
			EMSG("UEFI metadata variable index is invalid.");
			return PSA_ERROR_STORAGE_FAILURE;
		}
	} else {
		EMSG("Store backend is not accessible");
		return PSA_ERROR_STORAGE_FAILURE;
	}

	return PSA_ERROR_STORAGE_FAILURE;
}

static efi_status_t load_variable_index(struct uefi_variable_store *context)
{
	struct storage_backend *persistent_store = context->persistent_store.storage_backend;
	psa_status_t psa_status = PSA_SUCCESS;

	if (persistent_store) {
		size_t data_len = 0;
		size_t data_offset = 0;
		struct psa_storage_info_t variable_index_info = { 0 };

		psa_status = get_active_variable_uid(context, &context->active_variable_index_uid,
						     &context->variable_index.counter);
		switch (psa_status) {
		case PSA_SUCCESS:
			break;

		case PSA_ERROR_DOES_NOT_EXIST:
			IMSG("Variable index does not exist in NV store, continuing with empty index");
			return EFI_SUCCESS;

		default:
			EMSG("Loading variable index failed: %d", psa_status);
			return EFI_LOAD_ERROR;
		}

		/* Make sure the variable index fits the buffer */
		persistent_store->interface->get_info(persistent_store->context, context->owner_id,
						      context->active_variable_index_uid,
						      &variable_index_info);

		if (variable_index_info.size > context->index_sync_buffer_size) {
			EMSG("Variable index cannot fit the sync buffer");
			return EFI_LOAD_ERROR;
		}

		do {
			psa_status = persistent_store->interface->get(
				persistent_store->context, context->owner_id,
				context->active_variable_index_uid, data_offset,
				RPC_CALLER_SESSION_SHARED_MEMORY_SIZE,
				context->index_sync_buffer + data_offset, &data_len);

			if (psa_status != PSA_SUCCESS) {
				EMSG("Loading variable index failed: %d", psa_status);
				return EFI_LOAD_ERROR;
			}

			data_offset += data_len;

		} while (data_len == RPC_CALLER_SESSION_SHARED_MEMORY_SIZE);

		variable_index_restore(&context->variable_index, data_offset,
				       context->index_sync_buffer);
	} else {
		EMSG("Loading variable index failed, store backend is not accessible");
		return EFI_LOAD_ERROR;
	}

	return EFI_SUCCESS;
}

static efi_status_t sync_variable_index(struct uefi_variable_store *context)
{
	efi_status_t status = EFI_SUCCESS;
	psa_status_t psa_status = PSA_SUCCESS;
	bool is_dirty = false;

	/* Sync the variable index to storage if anything is dirty */
	size_t remaining_data_len = 0;

	status = variable_index_dump(&context->variable_index, context->index_sync_buffer_size,
				     context->index_sync_buffer, &remaining_data_len, &is_dirty);
	if (status != EFI_SUCCESS)
		return status;

	if (is_dirty) {
		struct storage_backend *persistent_store =
			context->persistent_store.storage_backend;

		if (persistent_store) {
			size_t data_offset = 0;
			uint64_t next_index_uid = 0;

			/* Write the older one */
			next_index_uid = (context->active_variable_index_uid ==
							  SMM_VARIABLE_INDEX_STORAGE_A_UID ?
						  SMM_VARIABLE_INDEX_STORAGE_B_UID :
						  SMM_VARIABLE_INDEX_STORAGE_A_UID);

			psa_status = persistent_store->interface->remove(
				persistent_store->context, context->owner_id, next_index_uid);

			if (psa_status != PSA_SUCCESS && psa_status != PSA_ERROR_DOES_NOT_EXIST)
				goto end;

			/* Check if the index exists and create if not yet */
			psa_status = persistent_store->interface->create(
				persistent_store->context, context->owner_id, next_index_uid,
				remaining_data_len, PSA_STORAGE_FLAG_NONE);

			if (psa_status != PSA_SUCCESS)
				goto end;

			do {
				size_t data_of_this_iteration = MIN(
					remaining_data_len, RPC_CALLER_SESSION_SHARED_MEMORY_SIZE);

				psa_status = persistent_store->interface->set_extended(
					persistent_store->context, context->owner_id,
					next_index_uid, data_offset, data_of_this_iteration,
					context->index_sync_buffer + data_offset);

				if (psa_status != PSA_SUCCESS)
					goto end;

				data_offset += RPC_CALLER_SESSION_SHARED_MEMORY_SIZE;
				remaining_data_len -= data_of_this_iteration;

			} while (remaining_data_len);

			variable_index_confirm_write(&context->variable_index);
			context->active_variable_index_uid = next_index_uid;
		} else {
			EMSG("Syncing variable index failed, store backend is not accessible");
			return EFI_LOAD_ERROR;
		}
	}

	end:
	return psa_to_efi_storage_status(psa_status);
}

/* Check attribute usage rules */
static efi_status_t check_capabilities(const SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *var)
{
	/**
	 * UEFI: Page 245
	 * Runtime access to a data variable implies boot service access. Attributes that
	 * have EFI_VARIABLE_RUNTIME_ACCESS set must also have EFI_VARIABLE_BOOTSERVICE_ACCESS
	 * set.
	 */
	if ((var->Attributes & EFI_VARIABLE_RUNTIME_ACCESS) &&
	    !(var->Attributes & EFI_VARIABLE_BOOTSERVICE_ACCESS)) {
		return EFI_INVALID_PARAMETER;
	}

	/**
	 * UEFI: Page 245
	 * EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS is deprecated and should not be used.
	 * Platforms should return EFI_UNSUPPORTED if a caller to SetVariable() specifies this
	 * attribute.
	 */
	if (var->Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS)
		return EFI_UNSUPPORTED;

	/**
	 * UEFI: Page 246
	 * If both the EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS and the
	 * EFI_VARIABLE_ENHANCED_AUTHENTICATED_ACCESS attribute are set in a
	 * SetVariable() call, then the firmware must return EFI_INVALID_PARAMETER.
	 */
	if ((var->Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) &&
	    (var->Attributes & EFI_VARIABLE_ENHANCED_AUTHENTICATED_ACCESS))
		return EFI_INVALID_PARAMETER;

	/* EFI_VARIABLE_AUTHENTICATION_3 is not supported by trusted-services */
	if (var->Attributes & EFI_VARIABLE_ENHANCED_AUTHENTICATED_ACCESS)
		return EFI_UNSUPPORTED;

	/* A non existing attribute setting has been requested */
	if (var->Attributes & ~EFI_VARIABLE_MASK)
		return EFI_UNSUPPORTED;

	/* EFI_VARIABLE_HARDWARE_ERROR_RECORD is not supported */
	if (var->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD)
		return EFI_UNSUPPORTED;

	return EFI_SUCCESS;
}

/**
 * UEFI: Page 237
 * If EFI_BOOT_SERVICES.ExitBootServices() has already been executed, data variables
 * without the EFI_VARIABLE_RUNTIME_ACCESS attribute set will not be visible to GetVariable()
 * and will return an EFI_NOT_FOUND error.
 */
static efi_status_t check_access_permitted(const struct uefi_variable_store *context,
					   const struct variable_info *info)
{
	efi_status_t status = EFI_SUCCESS;

	if (info->is_variable_set && (info->metadata.attributes & (EFI_VARIABLE_BOOTSERVICE_ACCESS |
								   EFI_VARIABLE_RUNTIME_ACCESS))) {
		/* Access is controlled */
		status = EFI_NOT_FOUND;

		if (context->is_boot_service) {
			if (info->metadata.attributes & EFI_VARIABLE_BOOTSERVICE_ACCESS)
				status = EFI_SUCCESS;
		} else {
			if (info->metadata.attributes & EFI_VARIABLE_RUNTIME_ACCESS)
				status = EFI_SUCCESS;
		}
	}

	return status;
}

static efi_status_t
check_access_permitted_on_set(const struct uefi_variable_store *context,
			      const struct variable_info *info,
			      const SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *var)
{
	efi_status_t status = check_access_permitted(context, info);

	if ((status == EFI_SUCCESS) && info->is_constraints_set) {
		/* Apply check constraints */
		status = variable_checker_check_on_set(&info->check_constraints, var->Attributes,
						       var->DataSize);
	}

	return status;
}

#if defined(UEFI_AUTH_VAR)

/*
 * Creates a "map" that contains pointers to some of the fields of the SMM variable and the
 * UEFI variable stored in the SMM data field. This way a variable is parsed only once.
 */
static bool init_efi_data_map(const SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *var,
			      bool with_auth_hdr, efi_data_map *map)
{
	bool is_valid = false;

	EFI_GUID pkcs7_guid = EFI_CERT_TYPE_PKCS7_GUID;

	if (var && map) {
		map->smm_variable = var;

		map->smm_variable_data =
			(uint8_t *)var + SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_DATA_OFFSET(var);

		if (with_auth_hdr) {
			map->efi_auth_descriptor =
				(EFI_VARIABLE_AUTHENTICATION_2 *)map->smm_variable_data;

			if (var->DataSize < EFI_VARIABLE_AUTHENTICATION_2_SIZE_WITHOUT_CERTDATA)
				return false;

			if (ADD_OVERFLOW(sizeof(map->efi_auth_descriptor->TimeStamp),
					 map->efi_auth_descriptor->AuthInfo.Hdr.dwLength,
					 &map->efi_auth_descriptor_len)) {
				EMSG("Auth descriptor overflow (TimeStamp - dwLength)");
				return false;
			}

			if (map->efi_auth_descriptor_len > var->DataSize) {
				EMSG("auth descriptor is longer than the max variable size");
				return false;
			}

			if (SUB_OVERFLOW(map->efi_auth_descriptor->AuthInfo.Hdr.dwLength,
					 sizeof(map->efi_auth_descriptor->AuthInfo.Hdr),
					 &map->efi_auth_descriptor_certdata_len)) {
				EMSG("Auth descriptor overflow (dwLength - Hdr)");
				return false;
			}

			if (SUB_OVERFLOW(map->efi_auth_descriptor_certdata_len,
					 sizeof(map->efi_auth_descriptor->AuthInfo.CertType),
					 &map->efi_auth_descriptor_certdata_len)) {
				EMSG("Auth descriptor overflow (dwLength - Hdr - CertType)");
				return false;
			}

			if (map->efi_auth_descriptor_certdata_len > var->DataSize) {
				EMSG("auth descriptor certdata field is longer than the max variable size");
				return false;
			}

			if (ADD_OVERFLOW((uintptr_t)(map->efi_auth_descriptor),
					 map->efi_auth_descriptor_len,
					 (uintptr_t *)(&map->payload))) {
				EMSG("auth descriptor overflow");
				return false;
			}

			if (SUB_OVERFLOW(map->smm_variable->DataSize, map->efi_auth_descriptor_len,
					 &map->payload_len)) {
				EMSG("Payload length overflow");
				return false;
			}

			if (map->payload_len > map->smm_variable->DataSize) {
				EMSG("auth descriptor certdata field is longer than the max variable size");
				return false;
			}

			/**
			 * Check a viable auth descriptor is present at start of variable data
			 * and that certificates and the signature are of the right type.
			 *
			 * UEFI: Page 253
			 * 1. Verify that the correct AuthInfo.CertType (EFI_CERT_TYPE_PKCS7_GUID)
			 * has been used and that the AuthInfo.CertData value parses correctly as a
			 * PKCS #7 SignedData value
			 */
			if (map->smm_variable->DataSize >= map->efi_auth_descriptor_len &&
			    map->efi_auth_descriptor->AuthInfo.Hdr.wRevision ==
				    WIN_CERT_CURRENT_VERSION &&
			    map->efi_auth_descriptor->AuthInfo.Hdr.wCertificateType ==
				    WIN_CERT_TYPE_EFI_GUID &&
			    compare_guid(&pkcs7_guid,
					 &map->efi_auth_descriptor->AuthInfo.CertType)) {
				/*
				 * If it the descriptor fits, determine the
				 * start and length of certificate data
				 */
				if (map->smm_variable->DataSize >= map->efi_auth_descriptor_len) {
					/* The certificate buffer follows the fixed sized header */
					uintptr_t cert_data_offset =
						(size_t)map->efi_auth_descriptor->AuthInfo.CertData -
						(size_t)map->smm_variable_data;

					if (map->efi_auth_descriptor_len >= cert_data_offset)
						is_valid = true;
				}
			}
		} else {
			map->payload = map->smm_variable_data;
			map->payload_len = map->smm_variable->DataSize;
			is_valid = true;
		}

		if (is_valid) {
			/*
			 * In case of key variables the payload is also
			 * accessible as a signature list
			 */
			if (compare_name_to_key_store_name(var->Name, var->NameSize,
							   EFI_PLATFORM_KEY_NAME,
							   sizeof(EFI_PLATFORM_KEY_NAME)) ||
			    compare_name_to_key_store_name(var->Name, var->NameSize,
							   EFI_KEY_EXCHANGE_KEY_NAME,
							   sizeof(EFI_KEY_EXCHANGE_KEY_NAME)) ||
			    compare_name_to_key_store_name(var->Name, var->NameSize,
							   EFI_IMAGE_SECURITY_DATABASE,
							   sizeof(EFI_IMAGE_SECURITY_DATABASE)) ||
			    compare_name_to_key_store_name(var->Name, var->NameSize,
							   EFI_IMAGE_SECURITY_DATABASE1,
							   sizeof(EFI_IMAGE_SECURITY_DATABASE1))) {
				map->efi_signature_list = (EFI_SIGNATURE_LIST *)map->payload;
			}

			return true;
		} else {
			return false;
		}
	}

	return false;
}

/*
 * Creates an SMM variable that can be passed to GetVariable call to be filled with the
 * data from the store.
 */
static void create_smm_variable(SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE **variable,
				size_t name_size, size_t data_size, const uint8_t *name,
				EFI_GUID *guid)
{
	/*
	 * name_size:             Length of the name array in bytes.
	 * (*variable)->NameSize: Length of (*variable)->Name array in bytes.
	 */

	if (name && guid) {
		*variable = (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *)malloc(
			SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_SIZE(name_size, data_size));

		if (*variable) {
			(*variable)->NameSize = name_size;
			(*variable)->DataSize = data_size;

			memcpy(&(*variable)->Guid, guid, sizeof(EFI_GUID));
			memcpy(&(*variable)->Name, name, name_size);
		}
	} else {
		*variable = NULL;
	}
}

/**
 * UEFI: Page 252
 * Hash the serialization of the values of the VariableName, VendorGuid and Attributes
 * parameters of the SetVariable() call and the TimeStamp component of the
 * EFI_VARIABLE_AUTHENTICATION_2 descriptor followed by the variable’s new value (i.e.
 * the Data parameter’s new variable content). That is, digest = hash (VariableName, VendorGuid,
 * Attributes, TimeStamp, DataNew_variable_content). The NULL character terminating the
 * VariableName value shall not be included in the hash computation.
 */
static bool calc_variable_hash(const efi_data_map *var_map, uint8_t *hash_buffer,
			       size_t hash_buffer_size, size_t *hash_len)
{
	psa_hash_operation_t op = psa_hash_operation_init();

	if (psa_hash_setup(&op, PSA_ALG_SHA_256) != PSA_SUCCESS)
		return false;

	/* Skip the NULL character at the end! */
	int status = psa_hash_update(&op, (const uint8_t *)var_map->smm_variable->Name,
				     var_map->smm_variable->NameSize - sizeof(uint16_t));

	if (!status)
		status = psa_hash_update(&op, (const uint8_t *)&var_map->smm_variable->Guid,
					 sizeof(EFI_GUID));

	if (!status)
		status = psa_hash_update(&op, (const uint8_t *)&var_map->smm_variable->Attributes,
					 sizeof(var_map->smm_variable->Attributes));

	if (!status)
		status = psa_hash_update(&op,
					 (const uint8_t *)&var_map->efi_auth_descriptor->TimeStamp,
					 sizeof(EFI_TIME));

	if (!status)
		status = psa_hash_update(&op, (const uint8_t *)var_map->payload,
					 var_map->payload_len);

	if (!status)
		status = psa_hash_finish(&op, hash_buffer, hash_buffer_size, hash_len);

	if (!status)
		return true;

	psa_hash_abort(&op);
	return false;
}

static efi_status_t
select_verification_keys(const efi_data_map new_var, EFI_GUID global_variable_guid,
			 EFI_GUID security_database_guid, uint64_t maximum_variable_size,
			 SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE **allowed_key_store_variables)
{
	/**
	 * UEFI: Page 254
	 * 5. If the variable is the global PK variable or the global KEK variable,
	 * verify that the signature has been made with the current Platform Key.
	 */
	if (compare_name_to_key_store_name(new_var.smm_variable->Name,
					   new_var.smm_variable->NameSize, EFI_PLATFORM_KEY_NAME,
					   sizeof(EFI_PLATFORM_KEY_NAME))) {
		/* This variable must have global guid */
		if (!compare_guid(&new_var.smm_variable->Guid, &global_variable_guid))
			return EFI_SECURITY_VIOLATION;

		/*
		 * PK is verified by itself if exists. In case of an empty PK,
		 * the verification shall be skipped.
		 */
		create_smm_variable(&(allowed_key_store_variables[0]),
				    sizeof(EFI_PLATFORM_KEY_NAME), maximum_variable_size,
				    (uint8_t *)EFI_PLATFORM_KEY_NAME, &global_variable_guid);
	} else if (compare_name_to_key_store_name(
			   new_var.smm_variable->Name, new_var.smm_variable->NameSize,
			   EFI_KEY_EXCHANGE_KEY_NAME, sizeof(EFI_KEY_EXCHANGE_KEY_NAME))) {
		/* This variable must have global guid */
		if (!compare_guid(&new_var.smm_variable->Guid, &global_variable_guid))
			return EFI_SECURITY_VIOLATION;

		/* KEK must be verified by PK */
		create_smm_variable(&(allowed_key_store_variables[0]),
				    sizeof(EFI_PLATFORM_KEY_NAME), maximum_variable_size,
				    (uint8_t *)EFI_PLATFORM_KEY_NAME, &global_variable_guid);
	}
	/**
	 * UEFI: Page 254:
	 * 5. If the variable is the “db”, “dbt”, “dbr”, or “dbx” variable mentioned
	 * in step 3, verify that the signer’s certificate chains to a certificate in the Key
	 * Exchange Key database (or that the signature was made with the current Platform Key).
	 */
	else if (compare_name_to_key_store_name(
			 new_var.smm_variable->Name, new_var.smm_variable->NameSize,
			 EFI_IMAGE_SECURITY_DATABASE, sizeof(EFI_IMAGE_SECURITY_DATABASE)) ||
		 compare_name_to_key_store_name(
			 new_var.smm_variable->Name, new_var.smm_variable->NameSize,
			 EFI_IMAGE_SECURITY_DATABASE1, sizeof(EFI_IMAGE_SECURITY_DATABASE1)) ||
		 compare_name_to_key_store_name(
			 new_var.smm_variable->Name, new_var.smm_variable->NameSize,
			 EFI_IMAGE_SECURITY_DATABASE2, sizeof(EFI_IMAGE_SECURITY_DATABASE2)) ||
		 compare_name_to_key_store_name(
			 new_var.smm_variable->Name, new_var.smm_variable->NameSize,
			 EFI_IMAGE_SECURITY_DATABASE3, sizeof(EFI_IMAGE_SECURITY_DATABASE3))) {
		/* This variable must have global guid */
		if (!compare_guid(&new_var.smm_variable->Guid, &security_database_guid))
			return EFI_SECURITY_VIOLATION;

		/* Databases can be verified by either KEK or PK */
		create_smm_variable(&(allowed_key_store_variables[0]),
				    sizeof(EFI_PLATFORM_KEY_NAME), maximum_variable_size,
				    (uint8_t *)EFI_PLATFORM_KEY_NAME, &global_variable_guid);

		create_smm_variable(&(allowed_key_store_variables[1]),
				    sizeof(EFI_KEY_EXCHANGE_KEY_NAME), maximum_variable_size,
				    (uint8_t *)EFI_KEY_EXCHANGE_KEY_NAME, &global_variable_guid);
	}

	return EFI_SUCCESS;
}

/*
 * Verifies the signature of an authenticated variable by another variable.
 * Only key variables that has a special payload(PK, KEK, db, dbx, dbr, dbt) can verify signature.
 */
static efi_status_t verify_var_by_key_var(const efi_data_map *new_var,
					  const efi_data_map *key_store_var,
					  const uint8_t *hash_buffer, size_t hash_len)
{
	EFI_GUID cert_x509_guid = EFI_CERT_X509_GUID;

	const EFI_SIGNATURE_LIST *current_signature_list = key_store_var->efi_signature_list;

	/* Initialized to the size of the payload and decreased by the processed data */
	size_t remaining_data = key_store_var->payload_len;

	/* Iterate through the signature lists */
	while ((remaining_data > 0) &&
	       (remaining_data >= current_signature_list->SignatureListSize)) {
		/*
		 * Check if the GUID of the signature list is properly set
		 * TODO: Only X509 certificate based signature list is supported
		 */
		if (compare_guid(&current_signature_list->SignatureType, &cert_x509_guid)) {
			EFI_SIGNATURE_DATA *current_signature =
				(EFI_SIGNATURE_DATA *)((uint8_t *)current_signature_list +
						       sizeof(EFI_SIGNATURE_LIST) +
						       current_signature_list->SignatureHeaderSize);

			size_t number_of_signatures =
				(current_signature_list->SignatureListSize -
				 sizeof(EFI_SIGNATURE_LIST) -
				 current_signature_list->SignatureHeaderSize) /
				current_signature_list->SignatureSize;

			/* Iterate through the certificates in the current signature list */
			for (int index = 0; index < number_of_signatures; index++) {
				uint8_t *next_certificate = current_signature->SignatureData;

				size_t next_certificate_size =
					current_signature_list->SignatureSize -
					sizeof(current_signature->SignatureOwner);

				if (verify_pkcs7_signature(
					    new_var->efi_auth_descriptor->AuthInfo.CertData,
					    new_var->efi_auth_descriptor_certdata_len, hash_buffer,
					    hash_len, next_certificate, next_certificate_size) == 0)

					return EFI_SUCCESS;

				/* Switch to the next certificate */
				current_signature =
					(EFI_SIGNATURE_DATA *)((uint8_t *)current_signature +
							       current_signature_list
								       ->SignatureSize);
			}
		} else {
			/* Wrong guid */
			return EFI_INVALID_PARAMETER;
		}

		remaining_data -= current_signature_list->SignatureListSize;
		current_signature_list =
			(EFI_SIGNATURE_LIST *)((uint8_t *)current_signature_list +
					       current_signature_list->SignatureListSize);
	}

	/* None of the entries verifies the new variable */
	return EFI_SECURITY_VIOLATION;
}

static bool is_private_auth_var(SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *var)
{
	if (compare_name_to_key_store_name(var->Name,
					   var->NameSize, EFI_PLATFORM_KEY_NAME,
					   sizeof(EFI_PLATFORM_KEY_NAME)) ||
		 compare_name_to_key_store_name(
			   var->Name, var->NameSize,
			   EFI_KEY_EXCHANGE_KEY_NAME, sizeof(EFI_KEY_EXCHANGE_KEY_NAME)) ||
		 compare_name_to_key_store_name(
			 var->Name, var->NameSize,
			 EFI_IMAGE_SECURITY_DATABASE, sizeof(EFI_IMAGE_SECURITY_DATABASE)) ||
		 compare_name_to_key_store_name(
			 var->Name, var->NameSize,
			 EFI_IMAGE_SECURITY_DATABASE1, sizeof(EFI_IMAGE_SECURITY_DATABASE1)) ||
		 compare_name_to_key_store_name(
			 var->Name, var->NameSize,
			 EFI_IMAGE_SECURITY_DATABASE2, sizeof(EFI_IMAGE_SECURITY_DATABASE2)) ||
		 compare_name_to_key_store_name(
			 var->Name, var->NameSize,
			 EFI_IMAGE_SECURITY_DATABASE3, sizeof(EFI_IMAGE_SECURITY_DATABASE3)))
		return false;

	return true;
}

/*
 * Basic verification of the authentication header of the new variable.
 * First finds the key variable responsible for the authentication of the new variable,
 * then verifies it.
 */
static efi_status_t authenticate_variable(const struct uefi_variable_store *context,
					  EFI_TIME *stored_timestamp,
					  uint8_t (*fingerprint)[FINGERPRINT_SIZE],
					  bool new_variable,
					  SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *var)
{
	efi_status_t status = EFI_SUCCESS;
	EFI_GUID pkcs7_guid = EFI_CERT_TYPE_PKCS7_GUID;
	SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO variable_info = { 0, 0, 0, 0 };
	efi_data_map var_map = { NULL, NULL, NULL, 0, 0, NULL, 0, NULL };
	uint8_t hash_buffer[PSA_HASH_MAX_SIZE];
	size_t hash_len = 0;

	/* Create a map of the fields of the new variable including the auth header */
	if (!init_efi_data_map(var, true, &var_map))
		return EFI_SECURITY_VIOLATION;

	/* Find the maximal size of variables for the GetVariable operation */
	status = uefi_variable_store_query_variable_info(context, &variable_info);
	if (status != EFI_SUCCESS)
		return EFI_SECURITY_VIOLATION;

	/**
	 * UEFI: Page 246
	 * If the EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS attribute is set in a
	 * SetVariable() call, and firmware does not support signature type of the certificate
	 * included in the EFI_VARIABLE_AUTHENTICATION_2 descriptor, then the SetVariable() call
	 * shall return EFI_INVALID_PARAMETER. The list of signature types supported by the
	 * firmware is defined by the SignatureSupport variable. Signature type of the certificate
	 * is defined by its digest and encryption algorithms.
	 */
	/* TODO: Should support WIN_CERT_TYPE_PKCS_SIGNED_DATA and WIN_CERT_TYPE_EFI_PKCS115 */
	if (var_map.efi_auth_descriptor->AuthInfo.Hdr.wCertificateType != WIN_CERT_TYPE_EFI_GUID)
		return EFI_INVALID_PARAMETER;

	/* Only a CertType of EFI_CERT_TYPE_PKCS7_GUID is accepted */
	if (!compare_guid(&var_map.efi_auth_descriptor->AuthInfo.CertType, &pkcs7_guid))
		return EFI_SECURITY_VIOLATION;

	/**
	 * Time associated with the authentication descriptor. For the TimeStamp value,
	 * components Pad1, Nanosecond, TimeZone, Daylight and Pad2 shall be set to 0.
	 * This means that the time shall always be expressed in GMT.
	 *
	 * UEFI: Page 253
	 * 2. Verify that Pad1, Nanosecond, TimeZone, Daylight and Pad2 components
	 * of the TimeStamp value are set to zero.
	 */
	if ((var_map.efi_auth_descriptor->TimeStamp.Pad1 != 0) ||
	    (var_map.efi_auth_descriptor->TimeStamp.Pad2 != 0) ||
	    (var_map.efi_auth_descriptor->TimeStamp.Nanosecond != 0) ||
	    (var_map.efi_auth_descriptor->TimeStamp.TimeZone != 0) ||
	    (var_map.efi_auth_descriptor->TimeStamp.Daylight != 0)) {
		return EFI_SECURITY_VIOLATION;
	}

	/**
	 * UEFI: Page 253
	 * Unless the EFI_VARIABLE_APPEND_WRITE attribute is set, verify
	 * that the TimeStamp value is later than the current
	 * timestamp value associated with the variable
	 */
	if (memcmp(&var_map.efi_auth_descriptor->TimeStamp, stored_timestamp, sizeof(EFI_TIME)) > 0) {
		/* Save new timestamp */
		memcpy(stored_timestamp, &var_map.efi_auth_descriptor->TimeStamp, sizeof(EFI_TIME));
	} else if (!(var->Attributes & EFI_VARIABLE_APPEND_WRITE)) {
		EMSG("Timestamp violation");
		return EFI_SECURITY_VIOLATION;
	}

	/* Calculate hash for the variable only once */
	if (!calc_variable_hash(&var_map, (uint8_t *)&hash_buffer, sizeof(hash_buffer), &hash_len))
		return EFI_SECURITY_VIOLATION;

	if (is_private_auth_var(var)) {
		/* Run Private Authenticated Variable related authentication steps */
		status = authenticate_private_variable(context, &var_map, hash_buffer,
						       hash_len, variable_info.MaximumVariableSize,
						       new_variable, fingerprint);
	} else {
		/* Run Secure Boot related authentication steps */
		status = authenticate_secure_boot_variable(context, &var_map, hash_buffer,
						   hash_len, variable_info.MaximumVariableSize);
	}

	/* Remove the authentication header from the variable if the authentication is successful */
	if (status == EFI_SUCCESS) {
		uint8_t *smm_payload =
			(uint8_t *)var + SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_DATA_OFFSET(var);

		memmove(smm_payload, var_map.payload, var_map.payload_len);
		memset((uint8_t *)smm_payload + var_map.payload_len, 0,
		       var_map.efi_auth_descriptor_len);

		var->DataSize -= var_map.efi_auth_descriptor_len;
	}

	return status;
}

static efi_status_t authenticate_secure_boot_variable(const struct uefi_variable_store *context,
						      efi_data_map* var_map,
						      uint8_t* hash_buffer,
						      size_t hash_len,
						      uint64_t max_variable_size)
{
	efi_status_t status = EFI_SUCCESS;
	EFI_GUID global_variable_guid = EFI_GLOBAL_VARIABLE;
	EFI_GUID security_database_guid = EFI_IMAGE_SECURITY_DATABASE_GUID;
	SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *pk_variable = NULL;
	size_t pk_payload_size = 0;

	/* database variables can be verified by either PK or KEK while images
	 * should be checked by db and dbx so the length of two will be enough.
	 */
	SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *allowed_key_store_variables[] = { NULL, NULL };

	/**
	 * UEFI: Page 253
	 * 3. If the variable SetupMode==1, and the variable is a secure
	 * boot policy variable, then the firmware implementation shall
	 * consider the checks in the following steps 4 and 5 to have
	 * passed, and proceed with updating the variable value as
	 * outlined below.
	 *
	 * While no Platform Key is enrolled, the SetupMode variable shall
	 * be equal to 1. While SetupMode == 1, the platform firmware shall
	 * not require authentication in order to modify the Platform Key,
	 * Key Enrollment Key, OsRecoveryOrder, OsRecovery####,
	 * and image security databases.
	 *
	 * After the Platform Key is enrolled, the SetupMode variable shall
	 * be equal to 0. While SetupMode == 0, the platform firmware shall
	 * require authentication in order to modify the Platform Key,
	 * Key Enrollment Key, OsRecoveryOrder, OsRecovery####,
	 * and image security databases
	 *
	 * NOTE: SetupMode variable is not supported yet, so the
	 * Platform Key is checked to enable or disable authentication.
	 */
	create_smm_variable(&pk_variable, sizeof(EFI_PLATFORM_KEY_NAME),
			    max_variable_size, (uint8_t *)EFI_PLATFORM_KEY_NAME,
			    &global_variable_guid);

	if (!pk_variable)
		return EFI_OUT_OF_RESOURCES;

	status = uefi_variable_store_get_variable(
		context, pk_variable, max_variable_size, &pk_payload_size);

	/* If PK does not exist authentication is disabled */
	switch (status)	{
		case EFI_SUCCESS:
			break;
		case EFI_NOT_FOUND:
			/* If PK does not exist authentication is disabled */
			free(pk_variable);
			status = EFI_SUCCESS;
			goto end;
		default:
			EMSG("Failed to read PK");
			free(pk_variable);
			status = EFI_SECURITY_VIOLATION;
			goto end;
	}

	/*
	 * Get the size of the PK payload with the help of a variable map before freeing the object.
	 * pk_var_map points to fields of pk_variable so this code part is in a separate code block
	 * to eliminate the var map right after freeing pk_variable. This way we can avoid
	 * unexpected access to freed memory area.
	 */
	{
		efi_data_map pk_var_map;

		/* Authentication header is not stored, so don't search for it! */
		if (!init_efi_data_map(pk_variable, false, &pk_var_map)) {
			free(pk_variable);
			return EFI_SECURITY_VIOLATION;
		}

		pk_payload_size = pk_var_map.payload_len;
		free(pk_variable);
	}

	/* If PK exists, but is empty the authentication is disabled */
	if (pk_payload_size == 0) {
		status = EFI_SUCCESS;
		goto end;
	}

	status = select_verification_keys(*var_map, global_variable_guid, security_database_guid,
					  max_variable_size,
					  &allowed_key_store_variables[0]);

	if (status != EFI_SUCCESS)
		goto end;

	for (int i = 0; i < ARRAY_SIZE(allowed_key_store_variables); i++) {
		size_t actual_variable_length = 0; /* Unused */
		efi_data_map allowed_key_store_var_map = { 0 };

		if (!allowed_key_store_variables[i])
			continue;

		status = uefi_variable_store_get_variable(context, allowed_key_store_variables[i],
							max_variable_size,
							&actual_variable_length);

		if (status) {
			/* When the parent does not exist it is considered verification failure */
			if (status == EFI_NOT_FOUND)
				status = EFI_SECURITY_VIOLATION;
			goto end;
		}

		/* Create a map of the variable fields for easier access */
		if (!init_efi_data_map(allowed_key_store_variables[i], false,
				       &allowed_key_store_var_map)) {
			status = EFI_SECURITY_VIOLATION;
			goto end;
		}

		status = verify_var_by_key_var(var_map, &allowed_key_store_var_map,
					hash_buffer, hash_len);

		if (status == EFI_SUCCESS)
			goto end;
	}

end:
	/* Cleanup heap */
	for (int i = 0; i < ARRAY_SIZE(allowed_key_store_variables); i++) {
		if (allowed_key_store_variables[i])
			free(allowed_key_store_variables[i]);
	}

	return status;
}

static efi_status_t authenticate_private_variable(const struct uefi_variable_store *context,
						  efi_data_map* var_map,
						  uint8_t* hash_buffer,
						  size_t hash_len,
						  uint64_t max_variable_size,
						  bool new_variable,
						  uint8_t (*fingerprint)[FINGERPRINT_SIZE])
{
	uint8_t new_fingerprint[PSA_HASH_MAX_SIZE] = { 0 };

	/* Verify the signature of the variable */
	if (verify_pkcs7_signature(
		var_map->efi_auth_descriptor->AuthInfo.CertData,
		var_map->efi_auth_descriptor_certdata_len, hash_buffer,
		hash_len, NULL, 0) != 0)
		return EFI_SECURITY_VIOLATION;

	/**
	 * UEFI: Page 254
	 * CN of the signing certificate’s Subject and the hash of the tbsCertificate of the
	 * top-level issuer certificate (or the signing certificate itself if no other certificates
	 * are present or the certificate chain is of length 1) in SignedData.certificates is
	 * registered for use in subsequent verifications of this variable. Implementations
	 * may store just a single hash of these two elements to reduce storage requirements.
	 */
	if (get_uefi_priv_auth_var_fingerprint_handler(var_map->efi_auth_descriptor->AuthInfo.CertData,
						       var_map->efi_auth_descriptor_certdata_len,
						       new_fingerprint)) {
		EMSG("Failed to query variable fingerprint input");
		return EFI_SECURITY_VIOLATION;
	}

	/*
	 * The hash is SHA256 so only 32 bytes contain non zero values.
	 * Use only that part to decrease metadata size.
	 */
	if (!new_variable) {
		if (memcmp(&new_fingerprint, fingerprint, FINGERPRINT_SIZE)) {
			EMSG("Fingerprint verification failed");
			return EFI_SECURITY_VIOLATION;
		}
	} else {
		/* Save fingerprint */
		memcpy(fingerprint, &new_fingerprint, FINGERPRINT_SIZE);
	}

	return EFI_SUCCESS;
}
#endif

static efi_status_t store_variable_data(const struct uefi_variable_store *context,
					const struct variable_info *info,
					const SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *var)
{
	psa_status_t psa_status = PSA_SUCCESS;
	size_t data_len = var->DataSize;
	const uint8_t *data =
		(const uint8_t *)var + SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_DATA_OFFSET(var);

	struct delegate_variable_store *delegate_store =
		select_delegate_store(context, info->metadata.attributes);

	if (delegate_store->storage_backend) {
		if (!(var->Attributes & EFI_VARIABLE_APPEND_WRITE)) {
			/* Create or overwrite variable data */
			psa_status = store_overwrite(delegate_store, context->owner_id,
						     info->metadata.uid, data_len, data);
		} else {
			/**
			 * UEFI: Page 246
			 * If the EFI_VARIABLE_APPEND_WRITE attribute is set in a SetVariable()
			 * call, then any existing variable value shall be appended with the
			 * value of the Data parameter. If the firmware does not support the
			 * append operation, then the SetVariable() call shall return
			 * EFI_INVALID_PARAMETER.
			 */
			psa_status = store_append_write(delegate_store, context->owner_id,
							info->metadata.uid, data_len, data);
		}
	}

	if ((psa_status != PSA_SUCCESS) && delegate_store->is_nv) {
		/* A storage failure has occurred so attempt to fix any
		 * mismatch between the variable index and stored NV variables.
		 */
		purge_orphan_index_entries(context);
	}

	return psa_to_efi_storage_status(psa_status);
}

static efi_status_t remove_variable_data(const struct uefi_variable_store *context,
					 const struct variable_info *info)
{
	psa_status_t psa_status = PSA_SUCCESS;

	if (info->is_variable_set) {
		struct delegate_variable_store *delegate_store =
			select_delegate_store(context, info->metadata.attributes);

		if (delegate_store->storage_backend) {
			psa_status = delegate_store->storage_backend->interface->remove(
				delegate_store->storage_backend->context, context->owner_id,
				info->metadata.uid);
		}
	}

	return psa_to_efi_storage_status(psa_status);
}

static efi_status_t load_variable_data(const struct uefi_variable_store *context,
				       const struct variable_info *info,
				       SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *var,
				       size_t max_data_len)
{
	psa_status_t psa_status = PSA_SUCCESS;
	uint8_t *data = (uint8_t *)var + SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE_DATA_OFFSET(var);

	struct delegate_variable_store *delegate_store =
		select_delegate_store(context, info->metadata.attributes);

	if (delegate_store->storage_backend) {
		struct psa_storage_info_t storage_info;

		psa_status = delegate_store->storage_backend->interface->get_info(
			delegate_store->storage_backend->context, context->owner_id,
			info->metadata.uid, &storage_info);

		if (psa_status == PSA_SUCCESS) {
			size_t get_limit = (var->DataSize < max_data_len) ? var->DataSize :
									    max_data_len;

			if (get_limit >= storage_info.size) {
				size_t got_len = 0;

				psa_status = delegate_store->storage_backend->interface->get(
					delegate_store->storage_backend->context, context->owner_id,
					info->metadata.uid, 0, storage_info.size, data, &got_len);

				var->DataSize = got_len;
			} else {
				var->DataSize = storage_info.size;
				psa_status = PSA_ERROR_BUFFER_TOO_SMALL;
			}
		}
	}

	return psa_to_efi_storage_status(psa_status);
}

static psa_status_t store_overwrite(struct delegate_variable_store *delegate_store,
				    uint32_t client_id, uint64_t uid, size_t data_length,
				    const void *data)
{
	/* Police maximum variable size limit */
	if (data_length > delegate_store->max_variable_size)
		return PSA_ERROR_INVALID_ARGUMENT;

	psa_status_t psa_status = delegate_store->storage_backend->interface->set(
		delegate_store->storage_backend->context, client_id, uid, data_length, data,
		PSA_STORAGE_FLAG_NONE);

	return psa_status;
}

static psa_status_t store_append_write(struct delegate_variable_store *delegate_store,
				       uint32_t client_id, uint64_t uid, size_t data_length,
				       const void *data)
{
	struct psa_storage_info_t storage_info;

	if (data_length == 0)
		return PSA_SUCCESS;

	psa_status_t psa_status = delegate_store->storage_backend->interface->get_info(
		delegate_store->storage_backend->context, client_id, uid, &storage_info);

	if (psa_status != PSA_SUCCESS)
		return psa_status;

	/* Determine size of appended variable */
	size_t new_size = storage_info.size + data_length;

	/* Defend against integer overflow */
	if (new_size < storage_info.size)
		return PSA_ERROR_INVALID_ARGUMENT;

	/* Police maximum variable size limit */
	if (new_size > delegate_store->max_variable_size)
		return PSA_ERROR_INVALID_ARGUMENT;

	/* Storage backend doesn't support an append operation so we need
	 * to read the current variable data, extend it and write it back.
	 */
	uint8_t *rw_buf = malloc(new_size);

	if (!rw_buf)
		return PSA_ERROR_INSUFFICIENT_MEMORY;

	size_t old_size = 0;

	psa_status = delegate_store->storage_backend->interface->get(
		delegate_store->storage_backend->context, client_id, uid, 0, new_size, rw_buf,
		&old_size);

	if (psa_status == PSA_SUCCESS) {
		if ((old_size + data_length) <= new_size) {
			/* Extend the variable data */
			memcpy(&rw_buf[old_size], data, data_length);

			psa_status = delegate_store->storage_backend->interface->set(
				delegate_store->storage_backend->context, client_id, uid,
				old_size + data_length, rw_buf, storage_info.flags);
		} else {
			/* There's a mismatch between the length obtained from
			 * get_info() and the subsequent length returned by get().
			 */
			psa_status = PSA_ERROR_STORAGE_FAILURE;
		}
	}

	free(rw_buf);

	return psa_status;
}

static void purge_orphan_index_entries(const struct uefi_variable_store *context)
{
	bool any_orphans = false;
	struct variable_index_iterator iter;

	variable_index_iterator_first(&iter, &context->variable_index);

	/* Iterate over variable index looking for any entries for NV
	 * variables where there is no corresponding object in the
	 * persistent store. This condition could arise due to
	 * a power failure before an object is stored.
	 */
	while (!variable_index_iterator_is_done(&iter)) {
		struct variable_info *info = variable_index_iterator_current(&iter);

		if (info->is_variable_set &&
		    (info->metadata.attributes & EFI_VARIABLE_NON_VOLATILE)) {
			struct psa_storage_info_t storage_info;
			struct storage_backend *storage_backend =
				context->persistent_store.storage_backend;

			psa_status_t psa_status = storage_backend->interface->get_info(
				storage_backend->context, context->owner_id, info->metadata.uid,
				&storage_info);

			if (psa_status != PSA_SUCCESS) {
				/* Detected a mismatch between the index and storage */
				variable_index_clear_variable(&context->variable_index, info);
				any_orphans = true;
			}
		}

		variable_index_iterator_next(&iter);
	}

	if (any_orphans)
		sync_variable_index((struct uefi_variable_store *)context);
}

static struct delegate_variable_store *
select_delegate_store(const struct uefi_variable_store *context, uint32_t attributes)
{
	bool is_nv = (attributes & EFI_VARIABLE_NON_VOLATILE);

	return (is_nv) ? (struct delegate_variable_store *)&context->persistent_store :
			 (struct delegate_variable_store *)&context->volatile_store;
}

static size_t space_used(const struct uefi_variable_store *context, uint32_t attributes,
			 struct storage_backend *storage_backend)
{
	if (!storage_backend)
		return 0;

	size_t total_used = 0;
	struct variable_index_iterator iter;

	variable_index_iterator_first(&iter, &context->variable_index);

	while (!variable_index_iterator_is_done(&iter)) {
		struct variable_info *info = variable_index_iterator_current(&iter);

		if (info->is_variable_set &&
		    ((info->metadata.attributes & EFI_VARIABLE_NON_VOLATILE) ==
		     (attributes & EFI_VARIABLE_NON_VOLATILE))) {
			struct psa_storage_info_t storage_info;

			psa_status_t psa_status = storage_backend->interface->get_info(
				storage_backend->context, context->owner_id, info->metadata.uid,
				&storage_info);

			if (psa_status == PSA_SUCCESS)
				total_used += storage_info.size;
		}

		variable_index_iterator_next(&iter);
	}

	return total_used;
}

static efi_status_t psa_to_efi_storage_status(psa_status_t psa_status)
{
	efi_status_t efi_status = EFI_DEVICE_ERROR;

	switch (psa_status) {
	case PSA_SUCCESS:
		efi_status = EFI_SUCCESS;
		break;
	case PSA_ERROR_NOT_PERMITTED:
		efi_status = EFI_ACCESS_DENIED;
		break;
	case PSA_ERROR_INVALID_ARGUMENT:
		efi_status = EFI_INVALID_PARAMETER;
		break;
	case PSA_ERROR_BAD_STATE:
		efi_status = EFI_NOT_READY;
		break;
	case PSA_ERROR_BUFFER_TOO_SMALL:
		efi_status = EFI_BUFFER_TOO_SMALL;
		break;
	case PSA_ERROR_DOES_NOT_EXIST:
		efi_status = EFI_NOT_FOUND;
		break;
	case PSA_ERROR_INSUFFICIENT_MEMORY:
		efi_status = EFI_OUT_OF_RESOURCES;
		break;
	case PSA_ERROR_INSUFFICIENT_STORAGE:
		efi_status = EFI_OUT_OF_RESOURCES;
		break;
	case PSA_ERROR_STORAGE_FAILURE:
		efi_status = EFI_DEVICE_ERROR;
		break;
	case PSA_STATUS_HARDWARE_FAILURE:
		efi_status = EFI_DEVICE_ERROR;
		break;
	default:
		break;
	}

	return efi_status;
}

static efi_status_t check_name_terminator(const int16_t *name, size_t name_size)
{
	/* Variable names must be null terminated */
	if (name_size < sizeof(int16_t) || name[name_size / sizeof(int16_t) - 1] != u'\0')
		return EFI_INVALID_PARAMETER;

	return EFI_SUCCESS;
}

#if defined(UEFI_AUTH_VAR)
/* Compares SMM variable name to key variable name. */
static bool compare_name_to_key_store_name(const int16_t *name1, size_t size1,
					   const uint16_t *name2, size_t size2)
{
	if (!name1 || !name2)
		return false;
	if (size1 != size2)
		return false;

	return memcmp((void *)name1, (void *)name2, size1) == 0;
}
#endif
