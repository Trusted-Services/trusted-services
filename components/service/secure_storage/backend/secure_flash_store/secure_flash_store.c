/*
 * Copyright (c) 2019-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "flash/sfs_flash.h"
#include "flash_fs/sfs_flash_fs.h"
#include <limits.h>
#include "sfs_utils.h"
#include "secure_flash_store.h"
#include <string.h>
#include <stddef.h>

#define SFS_MAX_ASSET_SIZE (4096) /* TODO: comes from flash layout */
#define SFS_CREATE_FLASH_LAYOUT /* TODO: move this to a proper place */

#define SFS_INVALID_UID 0 /* TODO: are there any invalid UID-s? */

/* Buffer to store asset data from the caller.
 * Note: size must be aligned to the max flash program unit to meet the
 * alignment requirement of the filesystem.
 */
static uint8_t asset_data[SFS_UTILS_ALIGN(SFS_MAX_ASSET_SIZE,
                                          SFS_FLASH_MAX_ALIGNMENT)];

static uint8_t g_fid[SFS_FILE_ID_SIZE];
static struct sfs_file_info_t g_file_info;

static sfs_flash_fs_ctx_t fs_ctx_sfs;

/**
 * \brief Maps a pair of client id and uid to a file id.
 *
 * \param[in]  client_id  Identifier of the asset's owner (client)
 * \param[in]  uid        Identifier for the data
 * \param[out] fid        Identifier of the file
 */
static void sfs_get_fid(uint32_t client_id,
                            uint64_t uid,
                            uint8_t *fid)
{
    memcpy(fid, (const void *)&client_id, sizeof(client_id));
    memcpy(fid + sizeof(client_id), (const void *)&uid, sizeof(uid));
}

static psa_status_t sfs_set(void *context,
                         uint32_t client_id,
                         uint64_t uid,
                         size_t data_length,
                         const void *p_data,
                         uint32_t create_flags)
{
    (void)context;

    psa_status_t status;
    size_t write_size;
    size_t offset;
    const uint8_t *data = (const uint8_t *)p_data;

    /* Check that the UID is valid */
    if (uid == SFS_INVALID_UID) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    /* Check that the create_flags does not contain any unsupported flags */
    if (create_flags & ~(PSA_STORAGE_FLAG_WRITE_ONCE |
                         PSA_STORAGE_FLAG_NO_CONFIDENTIALITY |
                         PSA_STORAGE_FLAG_NO_REPLAY_PROTECTION)) {
        return PSA_ERROR_NOT_SUPPORTED;
    }

    /* Set file id */
    sfs_get_fid(client_id, uid, g_fid);

    /* Read file info */
    status = sfs_flash_fs_file_get_info(&fs_ctx_sfs, g_fid, &g_file_info);
    if (status == PSA_SUCCESS) {
        /* If the object exists and has the write once flag set, then it
         * cannot be modified. Otherwise it needs to be removed.
         */
        if (g_file_info.flags & PSA_STORAGE_FLAG_WRITE_ONCE) {
            return PSA_ERROR_NOT_PERMITTED;
        } else {
            status = sfs_flash_fs_file_delete(&fs_ctx_sfs, g_fid);
            if (status != PSA_SUCCESS) {
                return status;
            }
        }
    } else if (status != PSA_ERROR_DOES_NOT_EXIST) {
        /* If the file does not exist, then do nothing.
         * If other error occurred, return it
         */
        return status;
    }

    /* Write as much of the data as will fit in the asset_data buffer */
    write_size = SFS_UTILS_MIN(data_length, sizeof(asset_data));

    /* Read asset data from the caller */
    memcpy(asset_data, data, write_size);
    data += write_size;

    /* Create the file in the file system */
    status = sfs_flash_fs_file_create(&fs_ctx_sfs, g_fid, data_length,
                                      write_size, (uint32_t)create_flags,
                                      asset_data);
    if (status != PSA_SUCCESS) {
        return status;
    }

    offset = write_size;
    data_length -= write_size;

    /* Iteratively read data from the caller and write it to the filesystem, in
     * chunks no larger than the size of the asset_data buffer.
     */
    while (data_length > 0) {
        write_size = SFS_UTILS_MIN(data_length, sizeof(asset_data));

        /* Read asset data from the caller */
        memcpy(asset_data, data, write_size);
        data += write_size;

        /* Write to the file in the file system */
        status = sfs_flash_fs_file_write(&fs_ctx_sfs, g_fid,
                                         write_size, offset, asset_data);
        if (status != PSA_SUCCESS) {
            /* Delete the file to avoid leaving partial data */
            (void)sfs_flash_fs_file_delete(&fs_ctx_sfs, g_fid);
            return status;
        }

        offset += write_size;
        data_length -= write_size;
    }

    return PSA_SUCCESS;
}

static psa_status_t sfs_get(void *context,
                         uint32_t client_id,
                         uint64_t uid,
                         size_t data_offset,
                         size_t data_size,
                         void *p_data,
                         size_t *p_data_length)
{
    (void)context;

    psa_status_t status;
    size_t read_size;
    uint8_t *data = (uint8_t *)p_data;

    /* Check that the UID is valid */
    if (uid == SFS_INVALID_UID) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    /* Set file id */
    sfs_get_fid(client_id, uid, g_fid);

    /* Read file info */
    status = sfs_flash_fs_file_get_info(&fs_ctx_sfs, g_fid, &g_file_info);
    if (status != PSA_SUCCESS) {
        return status;
    }

    /* Boundary check the incoming request */
    if (data_offset > g_file_info.size_current) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    /* Copy the object data only from within the file boundary */
    data_size = SFS_UTILS_MIN(data_size,
                              g_file_info.size_current - data_offset);

    /* Update the size of the output data */
    *p_data_length = data_size;

    /* Iteratively read data from the filesystem and write it to the caller, in
     * chunks no larger than the size of the asset_data buffer.
     */
    do {
        /* Read as much of the data as will fit in the asset_data buffer */
        read_size = SFS_UTILS_MIN(data_size, sizeof(asset_data));

        /* Read file data from the filesystem */
        status = sfs_flash_fs_file_read(&fs_ctx_sfs, g_fid, read_size,
                                        data_offset, asset_data);
        if (status != PSA_SUCCESS) {
            *p_data_length = 0;
            return status;
        }

        /* Write asset data to the caller */
        memcpy(data, asset_data, read_size);
        data += read_size;

        data_offset += read_size;
        data_size -= read_size;
    } while (data_size > 0);

    return PSA_SUCCESS;
}

static psa_status_t sfs_get_info(void *context, uint32_t client_id, uint64_t uid,
                              struct psa_storage_info_t *p_info)
{
    (void)context;

    psa_status_t status;

    /* Check that the UID is valid */
    if (uid == SFS_INVALID_UID) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    /* Set file id */
    sfs_get_fid(client_id, uid, g_fid);

    /* Read file info */
    status = sfs_flash_fs_file_get_info(&fs_ctx_sfs, g_fid, &g_file_info);
    if (status != PSA_SUCCESS) {
        return status;
    }

    /* Copy file info to the PSA info struct */
    p_info->capacity = g_file_info.size_max;
    p_info->size = g_file_info.size_current;
    p_info->flags = g_file_info.flags;

    return PSA_SUCCESS;
}

static psa_status_t sfs_remove(void *context, uint32_t client_id, uint64_t uid)
{
    (void)context;

    psa_status_t status;

    /* Check that the UID is valid */
    if (uid == SFS_INVALID_UID) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    /* Set file id */
    sfs_get_fid(client_id, uid, g_fid);

    status = sfs_flash_fs_file_get_info(&fs_ctx_sfs, g_fid, &g_file_info);
    if (status != PSA_SUCCESS) {
        return status;
    }

    /* If the object exists and has the write once flag set, then it
     * cannot be deleted.
     */
    if (g_file_info.flags & PSA_STORAGE_FLAG_WRITE_ONCE) {
        return PSA_ERROR_NOT_PERMITTED;
    }

    /* Delete old file from the persistent area */
    return sfs_flash_fs_file_delete(&fs_ctx_sfs, g_fid);
}

static psa_status_t sfs_create(void *context,
                            uint32_t client_id,
                            uint64_t uid,
                            size_t capacity,
                            uint32_t create_flags)
{
    (void)context;

    psa_status_t status;

    /* Check that the UID is valid */
    if (uid == SFS_INVALID_UID)
        return PSA_ERROR_INVALID_ARGUMENT;

    /* Check that the create_flags does not contain any unsupported flags */
    if (create_flags & PSA_STORAGE_FLAG_WRITE_ONCE)
        return PSA_ERROR_NOT_SUPPORTED;

    /* Set file id */
    sfs_get_fid(client_id, uid, g_fid);

    /* Read file info */
    status = sfs_flash_fs_file_get_info(&fs_ctx_sfs, g_fid, &g_file_info);

    if (status == PSA_SUCCESS) {
        return PSA_ERROR_ALREADY_EXISTS;
    } else if (status == PSA_ERROR_DOES_NOT_EXIST) {
        /* Create the file in the file system */
        status = sfs_flash_fs_file_create(&fs_ctx_sfs, g_fid, capacity,
                    0, (uint32_t)create_flags,
                    NULL);
    }

    return status;
}

static psa_status_t sfs_set_extended(void *context,
                            uint32_t client_id,
                            uint64_t uid,
                            size_t data_offset,
                            size_t data_length,
                            const void *p_data)
{
    psa_status_t status = PSA_SUCCESS;
    const uint8_t *data = (const uint8_t *)p_data;

    /* Check that the UID is valid */
    if (uid == SFS_INVALID_UID)
        return PSA_ERROR_INVALID_ARGUMENT;

    /* Get file id */
    sfs_get_fid(client_id, uid, g_fid);

    /* Read file info */
    status = sfs_flash_fs_file_get_info(&fs_ctx_sfs, g_fid, &g_file_info);
    if (status != PSA_SUCCESS)
        return status;

    if (g_file_info.flags & PSA_STORAGE_FLAG_WRITE_ONCE)
        return PSA_ERROR_NOT_PERMITTED;

    /* Avoid overflow of the inputs */
    if (SIZE_MAX - data_offset < data_length)
        return PSA_ERROR_INVALID_ARGUMENT;

    if (sizeof(asset_data) < data_length)
        return PSA_ERROR_INVALID_ARGUMENT;

    /* Data write must not exceed the file capacity */
    if (g_file_info.size_max < data_offset + data_length)
        return PSA_ERROR_INVALID_ARGUMENT;

    /* Write must not create gaps */
    if (g_file_info.size_current < data_offset)
        return PSA_ERROR_INVALID_ARGUMENT;

    /* Read asset data from the caller */
    memcpy(asset_data, data, data_length);

    /* Write to the file in the file system */
    status = sfs_flash_fs_file_write(&fs_ctx_sfs, g_fid,
                                     data_length, data_offset, asset_data);

    return status;
}

static uint32_t sfs_get_support(void *context, uint32_t client_id)
{
    (void)context;
    (void)client_id;

    return PSA_STORAGE_SUPPORT_SET_EXTENDED;
}

struct storage_backend *sfs_init(const struct sfs_flash_info_t *flash_binding)
{
    psa_status_t status;

    /* Initialise the SFS context */
    status = sfs_flash_fs_prepare(&fs_ctx_sfs, flash_binding);

#ifdef SFS_CREATE_FLASH_LAYOUT
    /* If SFS_CREATE_FLASH_LAYOUT is set, it indicates that it is required to
     * create a SFS flash layout. SFS service will generate an empty and valid
     * SFS flash layout to store assets. It will erase all data located in the
     * assigned SFS memory area before generating the SFS layout.
     * This flag is required to be set if the SFS memory area is located in
     * non-persistent memory.
     * This flag can be set if the SFS memory area is located in persistent
     * memory without a previous valid SFS flash layout in it. That is the case
     * when it is the first time in the device life that the SFS service is
     * executed.
     */
    if (status != PSA_SUCCESS) {
        /* Remove all data in the SFS memory area and create a valid SFS flash
         * layout in that area.
         */
        status = sfs_flash_fs_wipe_all(&fs_ctx_sfs);
        if (status != PSA_SUCCESS) {
            return NULL;
        }

        /* Attempt to initialise again */
        status = sfs_flash_fs_prepare(&fs_ctx_sfs, flash_binding);

        if (status != PSA_SUCCESS) {
            return NULL;
        }
    }
#endif /* SFS_CREATE_FLASH_LAYOUT */

    static const struct storage_backend_interface interface =
    {
        sfs_set,
        sfs_get,
        sfs_get_info,
        sfs_remove,
        sfs_create,
        sfs_set_extended,
        sfs_get_support
    };

    static struct storage_backend backend;

    backend.context = NULL;
    backend.interface = &interface;

    return &backend;
}
