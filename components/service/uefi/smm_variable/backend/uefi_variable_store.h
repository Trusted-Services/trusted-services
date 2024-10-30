/*
 * Copyright (c) 2021-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef UEFI_VARIABLE_STORE_H
#define UEFI_VARIABLE_STORE_H

#include <protocols/common/efi/efi_status.h>
#include <protocols/service/smm_variable/smm_variable_proto.h>
#include <service/secure_storage/backend/storage_backend.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "variable_index.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Size of the variable store index for a given entry count
 */
#define UEFI_VARIABLE_STORE_INDEX_SIZE(count) (sizeof(struct variable_metadata) * (count))

/**
 * \brief delegate_variable_store structure definition
 *
 * A delegate_variable_store combines an association with a concrete
 * storage backend and a set of limits parameters.
 */
struct delegate_variable_store {
	bool is_nv;
	size_t total_capacity;
	size_t max_variable_size;
	struct storage_backend *storage_backend;
};

/**
 * \brief uefi_variable_store structure definition
 *
 * A uefi_variable_store provides a variable store using a persistent and a
 * volatile storage backend.  The persistent storage backend may be realized
 * by another trusted service such as the protected storage or internal trusted
 * storage service.
 */
struct uefi_variable_store {
	bool is_boot_service;
	uint32_t owner_id;
	uint8_t *index_sync_buffer;
	size_t index_sync_buffer_size;
	uint64_t active_variable_index_uid;
	struct variable_index variable_index;
	struct delegate_variable_store persistent_store;
	struct delegate_variable_store volatile_store;
};

/**
 * @brief      Initialises a uefi_variable_store
 *
 * Initializes an instance of the uefi_variable_store.  The provided storage backends
 * are used to store variables and to store persistent metadata from the variable
 * register.
 *
 * @param[in]  context uefi_variable_store instance
 * @param[in]  owner_id The id of the owning security domain (e.g. partition id)
 * @param[in]  max_variables The maximum number of stored variables
 * @param[in]  persistent_store The persistent storage backend to use
 * @param[in]  volatile_store The volatile storage backend to use
 *
 * @return     EFI_SUCCESS if initialized successfully
 */
efi_status_t uefi_variable_store_init(struct uefi_variable_store *context, uint32_t owner_id,
				      size_t max_variables,
				      struct storage_backend *persistent_store,
				      struct storage_backend *volatile_store);

/**
 * @brief      De-initialises a uefi_variable_store
 *
 * @param[in]  context uefi_variable_store instance
 */
void uefi_variable_store_deinit(struct uefi_variable_store *context);

/**
 * @brief      Set storage limits
 *
 * Overrides the default limits for the specified storage space. These
 * values are reflected in the values returned by QueryVariableInfo.
 *
 * @param[in]  context uefi_variable_store instance
 * @param[in]  attributes EFI_VARIABLE_NON_VOLATILE or 0
 * @param[in]  total_capacity The total storage capacity in bytes
 * @param[in]  max_variable_size Variable size limit
 */
void uefi_variable_store_set_storage_limits(const struct uefi_variable_store *context,
					    uint32_t attributes, size_t total_capacity,
					    size_t max_variable_size);

/**
 * @brief      Set variable
 *
 * Corresponds to the SetVariable UEFI operation
 *
 * @param[in]  context uefi_variable_store instance
 * @param[in]  var The 'access variable' structure
 *
 * @return     EFI_SUCCESS if successful
 */
efi_status_t uefi_variable_store_set_variable(const struct uefi_variable_store *context,
					      const SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *var);

/**
 * @brief      Get variable
 *
 * Corresponds to the GetVariable UEFI operation
 *
 * @param[in]  context uefi_variable_store instance
 * @param[out] var The 'access variable' structure
 * @param[in]  max_data_len The maximum variable data length
 * @param[out] total_len The total length of the header + data
 *
 * @return     EFI_SUCCESS if successful
 */
efi_status_t uefi_variable_store_get_variable(const struct uefi_variable_store *context,
					      SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *var,
					      size_t max_data_len, size_t *total_length);

/**
 * @brief      Get next variable name
 *
 * Used for enumerating the store contents
 *
 * @param[in]  context uefi_variable_store instance
 * @param[inout] cur The size of the VariableName buffer
 * @param[out] total_len The total length of the output
 *
 * @return     EFI_SUCCESS if successful
 */
efi_status_t
uefi_variable_store_get_next_variable_name(const struct uefi_variable_store *context,
					   SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME *cur,
					   size_t *total_length);

/**
 * @brief      Query for variable info
 *
 * @param[in]  context uefi_variable_store instance
 * @param[inout] var_info Returns info
 *
 * @return     EFI_SUCCESS if successful
 */
efi_status_t
uefi_variable_store_query_variable_info(const struct uefi_variable_store *context,
					SMM_VARIABLE_COMMUNICATE_QUERY_VARIABLE_INFO *var_info);

/**
 * @brief      Exit boot service
 *
 * Called when the UEFI boot phase is complete.  Used for boot only
 * access control.
 *
 * @param[in]  context uefi_variable_store instance
  *
 * @return     EFI_SUCCESS if successful
 */
efi_status_t uefi_variable_store_exit_boot_service(struct uefi_variable_store *context);

/**
 * @brief      Set variable check property
 *
 * Corresponds to the SetVarCheckProperty EDK2 operation
 *
 * @param[in]  context uefi_variable_store instance
 * @param[in]  property The variable check property
 *
 * @return     EFI_SUCCESS if successful
 */
efi_status_t uefi_variable_store_set_var_check_property(
	struct uefi_variable_store *context,
	const SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY *property);

/**
 * @brief      Get variable check property
 *
 * Corresponds to the GetVarCheckProperty EDK2 operation
 *
 * @param[in]  context uefi_variable_store instance
 * @param[out] property The variable check property
 *
 * @return     EFI_SUCCESS if successful
 */
efi_status_t uefi_variable_store_get_var_check_property(
	struct uefi_variable_store *context,
	SMM_VARIABLE_COMMUNICATE_VAR_CHECK_VARIABLE_PROPERTY *property);

#ifdef __cplusplus
}
#endif

#endif /* UEFI_VARIABLE_STORE_H */
