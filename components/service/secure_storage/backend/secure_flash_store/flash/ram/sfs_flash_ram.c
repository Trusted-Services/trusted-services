/*
 * Copyright (c) 2019-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "sfs_flash_ram.h"
#include <string.h>

/**
 * \brief Gets physical address of the given block ID.
 *
 * \param[in] info      Flash device information
 * \param[in] block_id  Block ID
 * \param[in] offset    Offset position from the init of the block
 *
 * \returns Returns physical address for the given block ID.
 */
static uint32_t get_phys_address(const struct sfs_flash_info_t *info,
                                 uint32_t block_id, size_t offset)
{
    return (block_id * info->block_size) + offset;
}

psa_status_t sfs_flash_ram_init(const struct sfs_flash_info_t *info)
{
    /* Nothing needs to be done in case of flash emulated in RAM */
    (void)info;
    return PSA_SUCCESS;
}

psa_status_t sfs_flash_ram_read(const struct sfs_flash_info_t *info,
                                uint32_t block_id, uint8_t *buff, size_t offset,
                                size_t size)
{
    uint32_t idx = get_phys_address(info, block_id, offset);

    (void)memcpy(buff, (uint8_t *)info->flash_dev + idx, size);

    return PSA_SUCCESS;
}

psa_status_t sfs_flash_ram_write(const struct sfs_flash_info_t *info,
                                 uint32_t block_id, const uint8_t *buff,
                                 size_t offset, size_t size)
{
    uint32_t idx = get_phys_address(info, block_id, offset);

    (void)memcpy((uint8_t *)info->flash_dev + idx, buff, size);

    return PSA_SUCCESS;
}

psa_status_t sfs_flash_ram_flush(const struct sfs_flash_info_t *info)
{
    /* Nothing needs to be done for flash emulated in RAM, as writes are
     * commited immediately.
     */
    (void)info;
    return PSA_SUCCESS;
}

psa_status_t sfs_flash_ram_erase(const struct sfs_flash_info_t *info,
                                 uint32_t block_id)
{
    uint32_t idx = get_phys_address(info, block_id, 0);

    (void)memset((uint8_t *)info->flash_dev + idx, info->erase_val,
                     info->block_size);

    return PSA_SUCCESS;
}
