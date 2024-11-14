/*
 * Copyright (c) 2020-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PSA_INTERNAL_TRUSTED_STORAGE_H
#define PSA_INTERNAL_TRUSTED_STORAGE_H

#include <psa/error.h>
#include <psa/storage_common.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef EXPORT_PUBLIC_INTERFACE_PSA_ITS
#define PSA_ITS_EXPORTED __attribute__((__visibility__("default")))
#else
#define PSA_ITS_EXPORTED
#endif

/**
 * The major version number of the PSA ITS API. It will be incremented on
 * significant updates that may include breaking changes.
 */
#define PSA_ITS_API_VERSION_MAJOR 1

/**
 * The minor version number of the PSA ITS API. It will be incremented in
 * small updates that are unlikely to include breaking changes.
 */
#define PSA_ITS_API_VERSION_MINOR 0

/**
 * @brief      Create a new, or modify an existing, uid /value pair.
 *
 * @param[in]  uid           The identifier for the data
 * @param[in]  data_length   The size in bytes of the data in p_data
 * @param[in]  p_data        A buffer containing the data
 * @param[in]  create_flags  The flags that the data will be stored with
 *
 * @return     A status indicating the success/failure of the operation
 */
PSA_ITS_EXPORTED psa_status_t psa_its_set(psa_storage_uid_t uid, size_t data_length,
					  const void *p_data,
					  psa_storage_create_flags_t create_flags);

/**
 * @brief      Retrieve data associated with a provided UID.
 *
 * @param[in]  uid            The identifier for the data
 * @param[in]  data_offset    The starting offset of the data requested
 * @param[in]  data_size      The amount of data requested
 * @param      p_data         On success, the buffer where the data will be
 *                            placed
 * @param      p_data_length  On success, this will contain size of the data
 *                            placed in p_data
 *
 * @return     A status indicating the success/failure of the operation
 */
PSA_ITS_EXPORTED psa_status_t psa_its_get(psa_storage_uid_t uid, size_t data_offset,
					  size_t data_size, void *p_data, size_t *p_data_length);

/**
 * @brief      Retrieve the metadata about the provided uid.
 *
 * @param[in]  uid     The identifier for the data
 * @param      p_info  A pointer to the psa_storage_info_t struct that will
 *                     be populated with the metadata
 *
 * @return     A status indicating the success/failure of the operation
 */
PSA_ITS_EXPORTED psa_status_t psa_its_get_info(psa_storage_uid_t uid,
					       struct psa_storage_info_t *p_info);

/**
 * @brief      Remove the provided key and its associated data from the storage
 *
 * @param[in]  uid   The identifier for the data
 *
 * @return     A status indicating the success/failure of the operation
 */
PSA_ITS_EXPORTED psa_status_t psa_its_remove(psa_storage_uid_t uid);

#ifdef __cplusplus
}
#endif

#endif /* PSA_INTERNAL_TRUSTED_STORAGE_H */
