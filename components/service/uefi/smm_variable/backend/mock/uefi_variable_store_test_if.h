/*
 * Copyright (c) 2026, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef UEFI_VARIABLE_STORE_IF_H
#define UEFI_VARIABLE_STORE_IF_H

#include <protocols/common/efi/efi_status.h>
#include <psa/error.h>
#include <uefi_variable_store.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void expect_get_active_variable_uid(psa_status_t result,
				    uint64_t *active_index_uid,
				    uint32_t *counter);
void expect_variable_index_confirm_write(struct variable_index *context);
void expect_variable_index_dump(size_t *data_len, bool *any_dirty, efi_status_t result);
void expect_variable_index_restore(struct variable_index *context, size_t data_len, size_t result);

psa_status_t get_active_variable_uid(struct uefi_variable_store *context, uint64_t *active_index_uid, uint32_t *counter);
efi_status_t psa_to_efi_storage_status(psa_status_t psa_status);
efi_status_t uefi_variable_store_init(struct uefi_variable_store *context, uint32_t owner_id,
    size_t max_variables,
    struct storage_backend *persistent_store,
    struct storage_backend *volatile_store);
void purge_orphan_index_entries(const struct uefi_variable_store *context);
efi_status_t load_variable_index(struct uefi_variable_store *context);
efi_status_t sync_variable_index(struct uefi_variable_store *context);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // UEFI_VARIABLE_STORE_IF_H
