/*
 * Copyright (c) 2021-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef VARIABLE_INDEX_H
#define VARIABLE_INDEX_H

#include <protocols/common/efi/efi_status.h>
#include <protocols/common/efi/efi_types.h>
#include <protocols/service/smm_variable/smm_variable_proto.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "variable_checker.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Implementation limits
 */
#define VARIABLE_INDEX_MAX_NAME_SIZE (64)
#define FINGERPRINT_SIZE             (32)

/**
 * \brief variable_metadata structure definition
 *
 * Holds metadata associated with stored variable.
 */
struct variable_metadata {
	EFI_GUID guid;
	EFI_TIME timestamp;
	uint8_t fingerprint[FINGERPRINT_SIZE];
	size_t name_size;
	int16_t name[VARIABLE_INDEX_MAX_NAME_SIZE];
	uint32_t attributes;
	uint64_t uid;
};

/**
 * \brief variable_info structure definition
 *
 * Holds information about a stored variable.
 */
struct variable_info {
	struct variable_metadata metadata;
	struct variable_constraints check_constraints;

	bool is_variable_set;
	bool is_constraints_set;
};

/**
 * \brief An entry in the index
 *
 * Represents a store variable in the variable index.
 */
struct variable_entry {
	struct variable_info info;

	bool in_use;
	bool dirty;
};

/**
 * \brief variable_index structure definition
 *
 * Provides an index of stored variables to allow the uefi variable store
 * contents to be enumerated.
 */
struct variable_index {
	size_t max_variables;
	uint32_t counter;
	struct variable_entry *entries;
};

/**
 * @brief      Initialises a variable_index
 *
 * @param[in]  context variable_index
 * @param[in]  max_variables The maximum number of stored variables
 *
 * @return     EFI_SUCCESS if initialized successfully
 */
efi_status_t variable_index_init(struct variable_index *context, size_t max_variables);

/**
 * @brief      De-initialises a variable_index
 *
 * @param[in]  context variable_index
 */
void variable_index_deinit(struct variable_index *context);

/**
 * @brief      Returns the maximum dump size
 *
 * For a given maximum index size, returns the size of the
 * buffer that is needed to hold all serialized variable_info
 * objects.
 *
 * @param[in]  context variable_index
 */
size_t variable_index_max_dump_size(struct variable_index *context);

/**
 * @brief      Find info about a variable
 *
 * @param[in]  context variable_index
 * @param[in]  guid The variable's guid
 * @param[in]  name_size The name parameter's size
 * @param[in]  name The variable's name
 *
 * @return     Pointer to variable_info or NULL
 */
struct variable_info *variable_index_find(const struct variable_index *context,
					  const EFI_GUID *guid, size_t name_size,
					  const int16_t *name);

/**
 * @brief      Find the next variable in the index
 *
 * @param[in]  context variable_index
 * @param[in]  guid The variable's guid
 * @param[in]  name_size The name parameter's size
 * @param[in]  name The variable's name
 * @param[out] status Provides error status
 *
 * @return     Pointer to variable_info or NULL
 */
struct variable_info *variable_index_find_next(const struct variable_index *context,
					       const EFI_GUID *guid, size_t name_size,
					       const int16_t *name, efi_status_t *status);

/**
 * @brief      Add a new entry to the index
 *
 * An entry is needed either when a new variable is created or
 * when variable constraints are set for a variable that doesn't
 * yet exist.
 *
 * @param[in]  context variable_index
 * @param[in]  guid The variable's guid
 * @param[in]  name_size The name parameter's size
 * @param[in]  name The variable's name
 *
 * @return     Pointer to variable_info or NULL
 */
struct variable_info *variable_index_add_entry(const struct variable_index *context,
					       const EFI_GUID *guid, size_t name_size,
					       const int16_t *name);

/**
 * @brief      Remove an unused entry from the index
 *
 * Removes an entry if it is not in use.
 *
 * @param[in]  context variable_index
 * @param[in]  info The variable info corresponding to the entry to remove
 */
void variable_index_remove_unused_entry(const struct variable_index *context,
					struct variable_info *info);

/**
 * @brief      Set a variable to the index
 *
 * An entry for the variable must already exist.
 *
 * @param[in]  info variable info
 * @param[in]  attributes The variable's attributes
 */
void variable_index_set_variable(struct variable_info *info, uint32_t attributes);

/**
 * @brief      Clear a variable from the index
 *
 * Clears a variable from the index
 *
 * @param[in]  context variable_index
 * @param[in]  info The variable info corresponding to the variable to clear
 */
void variable_index_clear_variable(const struct variable_index *context,
				   struct variable_info *info);

/**
 * @brief      Set a check constraints object associated with a variavle
 *
 * @param[in]  info variable info
 * @param[in]  constraints The check constraints
 */
void variable_index_set_constraints(struct variable_info *info,
				    const struct variable_constraints *constraints);

/**
 * @brief      Dump the serialized index contents for persistent backup
 *
 * @param[in]  context variable_index
 * @param[in]  buffer_size Size of destination buffer
 * @param[in]  buffer Dump to this buffer
 * @param[out] data_len Length of serialized data
 * @param[out] any_dirty True if there is unsaved data
 *
 * @return     EFI_SUCCESS if all the changes are dumped successfully
 */
efi_status_t variable_index_dump(const struct variable_index *context, size_t buffer_size,
				 uint8_t *buffer, size_t *data_len, bool *any_dirty);

/**
 * @brief     Confirms the successful write of the variable index into the storage
 *            by stepping the counter.
 *
 * @param[in] context variable_index
 */
void variable_index_confirm_write(struct variable_index *context);

/**
 * @brief      Restore the serialized index contents
 *
 * Should be called straight after the variable index is initialized to
 * restore any NV variable info from persistent storage.
 *
 * @param[in]  context variable_index
 * @param[in]  data_len The length of the data to load
 * @param[in]  buffer Load from this buffer
 *
 * @return     Number of bytes loaded
 */
size_t variable_index_restore(struct variable_index *context, size_t data_len,
			      const uint8_t *buffer);

#ifdef __cplusplus
}
#endif

#endif /* VARIABLE_INDEX_H */
