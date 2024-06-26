/*
 * Copyright (c) 2018-2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "sfs_flash_fs_mblock.h"
#include <string.h>

/* Physical ID of the two metadata blocks */
/* NOTE: the earmarked area may not always start at block number 0.
 *       However, the flash interface can always add the required offset.
 */
#define SFS_METADATA_BLOCK0  0
#define SFS_METADATA_BLOCK1  1

/*!
 * \def SFS_OTHER_META_BLOCK
 *
 * \brief Macro to get the the swap metadata block.
 */
#define SFS_OTHER_META_BLOCK(metablock) \
(((metablock) == SFS_METADATA_BLOCK0) ? \
(SFS_METADATA_BLOCK1) : (SFS_METADATA_BLOCK0))

#define SFS_BLOCK_META_HEADER_SIZE  sizeof(struct sfs_metadata_block_header_t)
#define SFS_BLOCK_METADATA_SIZE     sizeof(struct sfs_block_meta_t)
#define SFS_FILE_METADATA_SIZE      sizeof(struct sfs_file_meta_t)

/* FIXME: Precompute these for each context */
/**
 * \brief Gets the physical block ID of the initial position of the scratch
 *        data block, for the current context.
 *
 * \param[in,out] fs_ctx  Filesystem context
 *
 * \return The physical block ID
 */
__attribute__((always_inline))
static inline uint32_t sfs_init_scratch_dblock(
                                              struct sfs_flash_fs_ctx_t *fs_ctx)
{
    /* When there are two blocks, the initial position of the scratch data block
     * is the scratch metadata block. Otherwise, the initial position of scratch
     * data block is immediately after the metadata blocks.
     */
    return fs_ctx->flash_info->num_blocks == 2 ? 1 : 2;
}

/**
 * \brief Gets the physical block ID of the start position of the data blocks,
 *        for the current context.
 *
 * \param[in,out] fs_ctx  Filesystem context
 *
 * \return The physical block ID
 */
__attribute__((always_inline))
static inline uint32_t sfs_init_dblock_start(struct sfs_flash_fs_ctx_t *fs_ctx)
{
    /* Metadata and data are always stored in the same block with two blocks.
     * Otherwise, one metadata block and two scratch blocks are reserved. One
     * scratch block for metadata operations and the other for data operations.
     */
    return fs_ctx->flash_info->num_blocks == 2 ? 0 : 3;
}

/**
 * \brief Gets the number of blocks that are dedicated wholely for data, for the
 *        current context.
 *
 * \param[in,out] fs_ctx  Filesystem context
 *
 * \return The number of dedicated datablocks
 */
static uint32_t sfs_num_dedicated_dblocks(struct sfs_flash_fs_ctx_t *fs_ctx)
{
    /* There are no dedicated data blocks when only two blocks are available.
     * Otherwise, the number of blocks dedicated just for data is the number of
     * blocks available beyond the initial datablock start index.
     */
     return fs_ctx->flash_info->num_blocks == 2 ? 0 :
                 fs_ctx->flash_info->num_blocks - sfs_init_dblock_start(fs_ctx);
}

/**
 * \brief Gets the number of active data blocks, for the current context.
 *
 * \param[in,out] fs_ctx  Filesystem context
 *
 * \return The number of active datablocks
 */
__attribute__((always_inline))
static inline uint32_t sfs_num_active_dblocks(struct sfs_flash_fs_ctx_t *fs_ctx)
{
    /* Total number of data blocks is the number of dedicated data blocks plus
     * logical data block 0 stored in the metadata block.
     */
    return sfs_num_dedicated_dblocks(fs_ctx) + 1;
}

/**
 * \brief Gets offset of a logical block's metadata in metadata block.
 *
 * \param[in] lblock  Logical block number
 *
 * \return Return offset value in metadata block
 */
static size_t sfs_mblock_block_meta_offset(uint32_t lblock)
{
    return SFS_BLOCK_META_HEADER_SIZE + (lblock * SFS_BLOCK_METADATA_SIZE);
}

/**
 * \brief Gets offset of an file metadata in metadata block.
 *
 * \param[in,out] fs_ctx  Filesystem context
 * \param[in]     idx     File metadata entry index
 *
 * \return Return offset value in metadata block
 */
static size_t sfs_mblock_file_meta_offset(struct sfs_flash_fs_ctx_t *fs_ctx,
                                          uint32_t idx)
{
    return SFS_BLOCK_META_HEADER_SIZE
           + (sfs_num_active_dblocks(fs_ctx) * SFS_BLOCK_METADATA_SIZE)
           + (idx * SFS_FILE_METADATA_SIZE);
}

/**
 * \brief Swaps metablocks. Scratch becomes active and active becomes scratch.
 *
 * \param[in,out] fs_ctx  Filesystem context
 */
static void sfs_mblock_swap_metablocks(struct sfs_flash_fs_ctx_t *fs_ctx)
{
    uint32_t tmp_block;

    tmp_block = fs_ctx->scratch_metablock;
    fs_ctx->scratch_metablock = fs_ctx->active_metablock;
    fs_ctx->active_metablock = tmp_block;
}

/**
 * \brief Finds the potential most recent valid metablock.
 *
 * \param[in] h_meta0  Header metadata of meta block 0
 * \param[in] h_meta1  Header metadata of meta block 1
 *
 * \return most recent metablock
 */
static uint8_t sfs_mblock_latest_meta_block(
                              const struct sfs_metadata_block_header_t *h_meta0,
                              const struct sfs_metadata_block_header_t *h_meta1)
{
    uint8_t cur_meta;
    uint8_t meta0_swap_count = h_meta0->active_swap_count;
    uint8_t meta1_swap_count = h_meta1->active_swap_count;

    /* Logic: if the swap count is 0, then it has rolled over. The metadata
     * block with a swap count of 0 is the latest one, unless the other block
     * has a swap count of 1, in which case the roll over occurred in the
     * previous update. In all other cases, the block with the highest swap
     * count is the latest one.
     */
    if ((meta1_swap_count == 0) && (meta0_swap_count != 1)) {
        /* Metadata block 1 swap count has rolled over and metadata block 0
         * swap count has not, so block 1 is the latest.
         */
        cur_meta = SFS_METADATA_BLOCK1;

    } else if ((meta0_swap_count == 0) && (meta1_swap_count != 1)) {
        /* Metadata block 0 swap count has rolled over and metadata block 1
         * swap count has not, so block 0 is the latest.
         */
        cur_meta = SFS_METADATA_BLOCK0;

    } else if (meta1_swap_count > meta0_swap_count) {
        /* Neither swap count has just rolled over and metadata block 1 has a
         * higher swap count, so block 1 is the latest.
         */
        cur_meta = SFS_METADATA_BLOCK1;

    } else {
        /* Neither swap count has just rolled over and metadata block 0 has a
         * higher or equal swap count, so block 0 is the latest.
         */
        cur_meta = SFS_METADATA_BLOCK0;
    }

    return cur_meta;
}

#ifdef SFS_VALIDATE_METADATA_FROM_FLASH
/**
 * \brief Validates file metadata in order to guarantee that a corruption or
 *        malicious change in stored metadata doesn't result in an invalid
 *        access.
 *
 * \param[in,out] fs_ctx     Filesystem context
 * \param[in]     file_meta  Pointer to file meta structure
 *
 * \return Returns error code as specified in \ref psa_status_t
 */
__attribute__((always_inline))
static inline psa_status_t sfs_mblock_validate_file_meta(
                                        struct sfs_flash_fs_ctx_t *fs_ctx,
                                        const struct sfs_file_meta_t *file_meta)
{
    int32_t err;

    /* Logical block ID can not be bigger or equal than number of
     * active blocks.
     */
    if (file_meta->lblock >= sfs_num_active_dblocks(fs_ctx)) {
        return PSA_ERROR_DATA_CORRUPT;
    }

    /* meta->id can be 0 if the file is not in use. If it is in
     * use, check the metadata.
     */
    if (sfs_utils_validate_fid(file_meta->id) == PSA_SUCCESS) {
        /* validate files values if file is in use */
        if (file_meta->max_size > fs_ctx->flash_info->max_file_size) {
            return PSA_ERROR_DATA_CORRUPT;
        }

        /* The current file data size must be smaller or equal than
         * file data max size.
         */
        if (file_meta->cur_size > file_meta->max_size) {
            return PSA_ERROR_DATA_CORRUPT;
        }

        if (file_meta->lblock == SFS_LOGICAL_DBLOCK0) {
            /* In block 0, data index must be located after the metadata */
            if (file_meta->data_idx < sfs_mblock_file_meta_offset(fs_ctx,
                                           fs_ctx->flash_info->max_num_files)) {
                return PSA_ERROR_DATA_CORRUPT;
            }
        }

        /* Boundary check the incoming request */
        err = sfs_utils_check_contained_in(fs_ctx->flash_info->block_size,
                                           file_meta->data_idx,
                                           file_meta->max_size);
        if (err != PSA_SUCCESS) {
            return PSA_ERROR_DATA_CORRUPT;
        }
    }

    return PSA_SUCCESS;
}

/**
 * \brief Validates block metadata in order to guarantee that a corruption or
 *        malicious change in stored metadata doesn't result in an invalid
 *        access.
 *
 * \param[in,out] fs_ctx      Filesystem context
 * \param[in]     block_meta  Pointer to block meta structure
 *
 * \return Returns error code as specified in \ref psa_status_t
 */
__attribute__((always_inline))
static inline psa_status_t sfs_mblock_validate_block_meta(
                                      struct sfs_flash_fs_ctx_t *fs_ctx,
                                      const struct sfs_block_meta_t *block_meta)
{
    psa_status_t err;
    /* Data block's data start at position 0 */
    size_t valid_data_start_value = 0;

    if (block_meta->phy_id >= fs_ctx->flash_info->num_blocks) {
        return PSA_ERROR_DATA_CORRUPT;
    }

    /* Boundary check: block data start + free size can not be bigger
     * than max block size.
     */
    err = sfs_utils_check_contained_in(fs_ctx->flash_info->block_size,
                                       block_meta->data_start,
                                       block_meta->free_size);
    if (err != PSA_SUCCESS) {
        return PSA_ERROR_DATA_CORRUPT;
    }

    if (block_meta->phy_id == SFS_METADATA_BLOCK0 ||
        block_meta->phy_id == SFS_METADATA_BLOCK1) {

        /* For metadata + data block, data index must start after the
         * metadata area.
         */
        valid_data_start_value = sfs_mblock_file_meta_offset(fs_ctx,
                                             fs_ctx->flash_info->max_num_files);
    }

    if (block_meta->data_start != valid_data_start_value) {
        return PSA_ERROR_DATA_CORRUPT;
    }

    return PSA_SUCCESS;
}
#endif /* SFS_VALIDATE_METADATA_FROM_FLASH */

/**
 * \brief Gets a free file metadata table entry.
 *
 * \param[in,out] fs_ctx  Filesystem context
 *
 * \return Return index of a free file meta entry
 */
static uint32_t sfs_get_free_file_index(struct sfs_flash_fs_ctx_t *fs_ctx)
{
    int32_t err;
    uint32_t i;
    struct sfs_file_meta_t tmp_metadata;

    for (i = 0; i < fs_ctx->flash_info->max_num_files; i++) {
        err = sfs_flash_fs_mblock_read_file_meta(fs_ctx, i, &tmp_metadata);
        if (err != PSA_SUCCESS) {
            return SFS_METADATA_INVALID_INDEX;
        }

        /* Check if this entry is free by checking if ID values is an
         * invalid ID.
         */
        if (sfs_utils_validate_fid(tmp_metadata.id) != PSA_SUCCESS) {
            /* Found */
            return i;
        }
    }

    return SFS_METADATA_INVALID_INDEX;
}

/**
 * \brief Erases data and meta scratch blocks.
 *
 * \param[in,out] fs_ctx  Filesystem context
 *
 * \return Returns error code as specified in \ref psa_status_t
 */
static psa_status_t sfs_mblock_erase_scratch_blocks(
                                              struct sfs_flash_fs_ctx_t *fs_ctx)
{
    psa_status_t err;
    uint32_t scratch_datablock;

    /* For the atomicity of the data update process
     * and power-failure-safe operation, it is necessary that
     * metadata scratch block is erased before data block.
     */
    err = fs_ctx->flash_info->erase(fs_ctx->flash_info,
                                    fs_ctx->scratch_metablock);
    if (err != PSA_SUCCESS) {
        return err;
    }

    /* If the number of blocks is bigger than 2, the code needs to erase the
     * scratch block used to process any change in the data block which contains
     * only data. Otherwise, if the number of blocks is equal to 2, it means
     * that all data is stored in the metadata block.
     */
    if (fs_ctx->flash_info->num_blocks > 2) {
        scratch_datablock =
            sfs_flash_fs_mblock_cur_data_scratch_id(fs_ctx,
                                                    (SFS_LOGICAL_DBLOCK0 + 1));
        err = fs_ctx->flash_info->erase(fs_ctx->flash_info, scratch_datablock);
    }

    return err;
}

/**
 * \brief Updates scratch block meta.
 *
 * \param[in,out] fs_ctx      Filesystem context
 * \param[in]     lblock      Logical block number
 * \param[in]     block_meta  Pointer to the block metadata data to write in the
 *                            scratch block
 *
 * \return Returns error code as specified in \ref psa_status_t
 */
static psa_status_t sfs_mblock_update_scratch_block_meta(
                                      struct sfs_flash_fs_ctx_t *fs_ctx,
                                      uint32_t lblock,
                                      const struct sfs_block_meta_t *block_meta)
{
    size_t pos;

    /* Calculate the position */
    pos = sfs_mblock_block_meta_offset(lblock);
    return fs_ctx->flash_info->write(fs_ctx->flash_info,
                                     fs_ctx->scratch_metablock,
                                     (const uint8_t *)block_meta, pos,
                                     SFS_BLOCK_METADATA_SIZE);
}

/**
 * \brief Copies rest of the block metadata.
 *
 * \param[in,out] fs_ctx  Filesystem context
 * \param[in]     lblock  Logical block number to skip
 *
 * \return Returns error code as specified in \ref psa_status_t
 */
static psa_status_t sfs_mblock_copy_remaining_block_meta(
                                              struct sfs_flash_fs_ctx_t *fs_ctx,
                                              uint32_t lblock)
{
    struct sfs_block_meta_t block_meta;
    psa_status_t err;
    uint32_t meta_block;
    size_t pos;
    uint32_t scratch_block;
    size_t size;

    scratch_block = fs_ctx->scratch_metablock;
    meta_block = fs_ctx->active_metablock;

    if (lblock != SFS_LOGICAL_DBLOCK0) {
        /* The file data in the logical block 0 is stored in same physical
         * block where the metadata is stored. A change in the metadata requires
         * a swap of physical blocks. So, the physical block ID of logical block
         * 0 needs to be updated to reflect this change, if the file processed
         * is not located in logical block 0. If it is located in block 0,
         * the physical block ID has been updated while processing the file
         * data.
         */
        err = sfs_flash_fs_mblock_read_block_metadata(fs_ctx,
                                                      SFS_LOGICAL_DBLOCK0,
                                                      &block_meta);
        if (err != PSA_SUCCESS) {
            return PSA_ERROR_GENERIC_ERROR;
        }

        /* Update physical ID for logical block 0 to match with the
         * metadata block physical ID.
         */
        block_meta.phy_id = scratch_block;
        err = sfs_mblock_update_scratch_block_meta(fs_ctx, SFS_LOGICAL_DBLOCK0,
                                                   &block_meta);
        if (err != PSA_SUCCESS) {
            return PSA_ERROR_GENERIC_ERROR;
        }

        /* Copy the rest of metadata blocks between logical block 0 and
         * the logical block provided in the function.
         */
        if (lblock > 1) {
            pos = sfs_mblock_block_meta_offset(SFS_LOGICAL_DBLOCK0 + 1);

            size = sfs_mblock_block_meta_offset(lblock) - pos;

            /* Copy rest of the block data from previous block */
            /* Data before updated content */
            err = sfs_flash_block_to_block_move(fs_ctx->flash_info,
                                                scratch_block, pos, meta_block,
                                                pos, size);
            if (err != PSA_SUCCESS) {
                return err;
            }
        }
    }

    /* Move meta blocks data after updated content */
    pos = sfs_mblock_block_meta_offset(lblock+1);

    size = sfs_mblock_file_meta_offset(fs_ctx, 0) - pos;

    return sfs_flash_block_to_block_move(fs_ctx->flash_info, scratch_block, pos,
                                         meta_block, pos, size);
}

/**
 * \brief Checks the validity of the metadata block's swap count.
 *
 * \param[in,out] fs_ctx      Filesystem context
 * \param[in]     swap_count  Swap count to validate
 *
 * \return Returns error code as specified in \ref psa_status_t
 */
__attribute__((always_inline))
static inline psa_status_t sfs_mblock_validate_swap_count(
                                              struct sfs_flash_fs_ctx_t *fs_ctx,
                                              uint8_t swap_count)
{
    /* When a flash block is erased, the default value
     * is usually 0xFF (i.e. all 1s). Since the swap count
     * is updated last (when encryption is disabled), it is
     * possible that due to a power failure, the swap count
     * value in metadata header is 0xFFFF..., which mean
     * it will appear to be most recent block. Which isn't
     * a problem in itself, as the rest of the metadata is fully
     * valid (as it would have been written before swap count).
     * However, this also means that previous update process
     * wasn't complete. So, if the value is 0xFF..., revert
     * back to previous metablock instead.
     */
    return (swap_count == fs_ctx->flash_info->erase_val)
           ? PSA_ERROR_GENERIC_ERROR
           : PSA_SUCCESS;
}

/**
 * \brief Checks the validity of FS version.
 *
 * \param[in] fs_version  File system version.
 *
 * \return Returns error code as specified in \ref psa_status_t
 */
__attribute__((always_inline))
static inline psa_status_t sfs_mblock_validate_fs_version(uint8_t fs_version)
{
    /* Looks for exact version number.
     * FIXME: backward compatibility could be considered in future revisions.
     */
    return (fs_version != SFS_SUPPORTED_VERSION) ? PSA_ERROR_GENERIC_ERROR
                                                 : PSA_SUCCESS;
}

/**
 * \brief Validates header metadata in order to guarantee that a corruption or
 *        malicious change in stored metadata doesn't result in an invalid
 *        access and the header version is correct.
 *
 * \param[in,out] fs_ctx  Filesystem context
 * \param[in]     h_meta  Pointer to metadata block header
 *
 * \return Returns error code as specified in \ref psa_status_t
 */
static psa_status_t sfs_mblock_validate_header_meta(
                               struct sfs_flash_fs_ctx_t *fs_ctx,
                               const struct sfs_metadata_block_header_t *h_meta)
{
    psa_status_t err;

    err = sfs_mblock_validate_fs_version(h_meta->fs_version);
    if (err == PSA_SUCCESS) {
        err = sfs_mblock_validate_swap_count(fs_ctx, h_meta->active_swap_count);
    }

    return err;
}

/**
 * \brief Writes the scratch metadata's header.
 *
 * \param[in,out] fs_ctx  Filesystem context
 *
 * \return Returns error code as specified in \ref psa_status_t
 */
static psa_status_t sfs_mblock_write_scratch_meta_header(
                                              struct sfs_flash_fs_ctx_t *fs_ctx)
{
    psa_status_t err;

    /* Increment the swap count */
    fs_ctx->meta_block_header.active_swap_count += 1;

    err = sfs_mblock_validate_swap_count(fs_ctx,
                                   fs_ctx->meta_block_header.active_swap_count);
    if (err != PSA_SUCCESS) {
        /* Reset the swap count to 0 */
        fs_ctx->meta_block_header.active_swap_count = 0;
    }

    /* Write the metadata block header */
    return fs_ctx->flash_info->write(fs_ctx->flash_info,
                                     fs_ctx->scratch_metablock,
                                     (uint8_t *)(&fs_ctx->meta_block_header),
                                     0, SFS_BLOCK_META_HEADER_SIZE);
}

/**
 * \brief Reads the active metadata block header into sfs_system_ctx.
 *
 * \param[in,out] fs_ctx  Filesystem context
 *
 * \return Returns error code as specified in \ref psa_status_t
 */
static psa_status_t sfs_mblock_read_meta_header(
                                              struct sfs_flash_fs_ctx_t *fs_ctx)
{
    psa_status_t err;

    err = fs_ctx->flash_info->read(fs_ctx->flash_info, fs_ctx->active_metablock,
                                   (uint8_t *)&fs_ctx->meta_block_header, 0,
                                   SFS_BLOCK_META_HEADER_SIZE);
    if (err != PSA_SUCCESS) {
        return err;
    }

    return sfs_mblock_validate_header_meta(fs_ctx, &fs_ctx->meta_block_header);
}

/**
 * \brief Reserves space for an file.
 *
 * \param[in,out] fs_ctx      Filesystem context
 * \param[in]     fid         File ID
 * \param[in]     size        Size of the file for which space is reserve
 * \param[in]     flags       Flags set when the file is created
 * \param[out]    file_meta   File metadata entry
 * \param[out]    block_meta  Block metadata entry
 *
 * \return Returns error code as specified in \ref psa_status_t
 */
static psa_status_t sfs_mblock_reserve_file(struct sfs_flash_fs_ctx_t *fs_ctx,
                                            const uint8_t *fid, size_t size,
                                            uint32_t flags,
                                            struct sfs_file_meta_t *file_meta,
                                            struct sfs_block_meta_t *block_meta)
{
    psa_status_t err;
    uint32_t i;

    for (i = 0; i < sfs_num_active_dblocks(fs_ctx); i++) {
        err = sfs_flash_fs_mblock_read_block_metadata(fs_ctx, i, block_meta);
        if (err != PSA_SUCCESS) {
            return PSA_ERROR_GENERIC_ERROR;
        }

        if (block_meta->free_size >= size) {
            /* Set file metadata */
            file_meta->lblock = i;
            file_meta->data_idx = fs_ctx->flash_info->block_size
                                  - block_meta->free_size;
            file_meta->max_size = size;
            memcpy(file_meta->id, fid, SFS_FILE_ID_SIZE);
            file_meta->cur_size = 0;
            file_meta->flags = flags;

            /* Update block metadata */
            block_meta->free_size -= size;
            return PSA_SUCCESS;
        }
    }

    /* No block has large enough space to fit the requested file */
    return PSA_ERROR_INSUFFICIENT_STORAGE;
}

/**
 * \brief Validates and find the valid-active metablock
 *
 * \param[in,out] fs_ctx  Filesystem context
 *
 * \return Returns value as specified in \ref psa_status_t
 */
static psa_status_t sfs_init_get_active_metablock(
                                              struct sfs_flash_fs_ctx_t *fs_ctx)
{
    uint32_t cur_meta_block = SFS_BLOCK_INVALID_ID;
    psa_status_t err;
    struct sfs_metadata_block_header_t h_meta0;
    struct sfs_metadata_block_header_t h_meta1;
    uint8_t num_valid_meta_blocks = 0;

    /* First two blocks are reserved for metadata */

    /* Read the header of both the metdata blocks. If the read succeeds, then
     * attempt to validate the metadata header, otherwise assume that the block
     * update was incomplete
     */
    err = fs_ctx->flash_info->read(fs_ctx->flash_info, SFS_METADATA_BLOCK0,
                                   (uint8_t *)&h_meta0, 0,
                                   SFS_BLOCK_META_HEADER_SIZE);
    if (err == PSA_SUCCESS) {
        if (sfs_mblock_validate_header_meta(fs_ctx, &h_meta0) == PSA_SUCCESS) {
            num_valid_meta_blocks++;
            cur_meta_block = SFS_METADATA_BLOCK0;
        }
    }

    err = fs_ctx->flash_info->read(fs_ctx->flash_info, SFS_METADATA_BLOCK1,
                                   (uint8_t *)&h_meta1, 0,
                                   SFS_BLOCK_META_HEADER_SIZE);
    if (err == PSA_SUCCESS) {
        if (sfs_mblock_validate_header_meta(fs_ctx, &h_meta1) == PSA_SUCCESS) {
            num_valid_meta_blocks++;
            cur_meta_block = SFS_METADATA_BLOCK1;
        }
    }

    /* If there are more than 1 potential metablocks, the previous
     * update operation was interrupted by power failure. In which case,
     * need to find out which one is potentially latest metablock.
     */
    if (num_valid_meta_blocks > 1) {
        cur_meta_block = sfs_mblock_latest_meta_block(&h_meta0, &h_meta1);
    } else if (num_valid_meta_blocks == 0) {
        return PSA_ERROR_GENERIC_ERROR;
    }

    fs_ctx->active_metablock = cur_meta_block;
    fs_ctx->scratch_metablock = SFS_OTHER_META_BLOCK(cur_meta_block);

    return PSA_SUCCESS;
}

psa_status_t sfs_flash_fs_mblock_cp_remaining_file_meta(
                                              struct sfs_flash_fs_ctx_t *fs_ctx,
                                              uint32_t idx)
{
    psa_status_t err;
    size_t end;
    uint32_t meta_block;
    size_t pos;
    uint32_t scratch_block;

    scratch_block = fs_ctx->scratch_metablock;
    meta_block = fs_ctx->active_metablock;
    /* Calculate the position */
    pos = sfs_mblock_file_meta_offset(fs_ctx, 0);
    /* Copy rest of the block data from previous block */
    /* Data before updated content */
    err = sfs_flash_block_to_block_move(fs_ctx->flash_info, scratch_block, pos,
                                        meta_block, pos,
                                        (idx * SFS_FILE_METADATA_SIZE));
    if (err != PSA_SUCCESS) {
        return err;
    }

    /* Data after updated content */
    pos = sfs_mblock_file_meta_offset(fs_ctx, idx + 1);

    /* Get end of file meta position which is the position after the last
     * byte of file meta.
     */
    end = sfs_mblock_file_meta_offset(fs_ctx,
                                      fs_ctx->flash_info->max_num_files);
    if (end > pos) {
        err = sfs_flash_block_to_block_move(fs_ctx->flash_info, scratch_block,
                                            pos, meta_block, pos, (end - pos));
    }

    return err;
}

uint32_t sfs_flash_fs_mblock_cur_data_scratch_id(
                                              struct sfs_flash_fs_ctx_t *fs_ctx,
                                              uint32_t lblock)
{
    if (lblock == SFS_LOGICAL_DBLOCK0) {
        /* Scratch logical data block 0 physical IDs */
        return fs_ctx->scratch_metablock;
    }

    return fs_ctx->meta_block_header.scratch_dblock;
}

psa_status_t sfs_flash_fs_mblock_get_file_idx(struct sfs_flash_fs_ctx_t *fs_ctx,
                                              const uint8_t *fid,
                                              uint32_t *idx)
{
    psa_status_t err;
    uint32_t i;
    struct sfs_file_meta_t tmp_metadata;

    for (i = 0; i < fs_ctx->flash_info->max_num_files; i++) {
        err = sfs_flash_fs_mblock_read_file_meta(fs_ctx, i, &tmp_metadata);
        if (err != PSA_SUCCESS) {
            return PSA_ERROR_GENERIC_ERROR;
        }

        /* ID with value 0x00 means end of file meta section */
        if (!memcmp(tmp_metadata.id, fid, SFS_FILE_ID_SIZE)) {
            /* Found */
            *idx = i;
            return PSA_SUCCESS;
        }
    }

    return PSA_ERROR_DOES_NOT_EXIST;
}

psa_status_t sfs_flash_fs_mblock_init(struct sfs_flash_fs_ctx_t *fs_ctx)
{
    psa_status_t err;

    /* Initialize Flash Interface */
    err = fs_ctx->flash_info->init(fs_ctx->flash_info);
    if (err != PSA_SUCCESS) {
        return err;
    }

    err = sfs_init_get_active_metablock(fs_ctx);
    if (err != PSA_SUCCESS) {
        return PSA_ERROR_GENERIC_ERROR;
    }

    err = sfs_mblock_read_meta_header(fs_ctx);
    if (err != PSA_SUCCESS) {
        return PSA_ERROR_GENERIC_ERROR;
    }

    /* Erase the other scratch metadata block */
    return sfs_mblock_erase_scratch_blocks(fs_ctx);
}

psa_status_t sfs_flash_fs_mblock_meta_update_finalize(
                                              struct sfs_flash_fs_ctx_t *fs_ctx)
{
    psa_status_t err;

    /* Write the metadata block header to flash */
    err = sfs_mblock_write_scratch_meta_header(fs_ctx);
    if (err != PSA_SUCCESS) {
        return err;
    }

    /* Commit metadata block modifications to flash */
    err = fs_ctx->flash_info->flush(fs_ctx->flash_info);
    if (err != PSA_SUCCESS) {
        return err;
    }

    /* Update the running context */
    sfs_mblock_swap_metablocks(fs_ctx);

    /* Erase meta block and current scratch block */
    return sfs_mblock_erase_scratch_blocks(fs_ctx);
}

psa_status_t sfs_flash_fs_mblock_migrate_lb0_data_to_scratch(
                                              struct sfs_flash_fs_ctx_t *fs_ctx)
{
    struct sfs_block_meta_t block_meta;
    size_t data_size;
    psa_status_t err;

    err = sfs_flash_fs_mblock_read_block_metadata(fs_ctx, SFS_LOGICAL_DBLOCK0,
                                                  &block_meta);
    if (err != PSA_SUCCESS) {
        return err;
    }

    /* Calculate data size stored in the B0 block */
    data_size = (fs_ctx->flash_info->block_size - block_meta.data_start)
                - block_meta.free_size;

    return sfs_flash_block_to_block_move(fs_ctx->flash_info,
                                         fs_ctx->scratch_metablock,
                                         block_meta.data_start,
                                         fs_ctx->active_metablock,
                                         block_meta.data_start,
                                         data_size);
}

psa_status_t sfs_flash_fs_mblock_read_file_meta(
                                              struct sfs_flash_fs_ctx_t *fs_ctx,
                                              uint32_t idx,
                                              struct sfs_file_meta_t *file_meta)
{
    psa_status_t err;
    size_t offset;

    offset = sfs_mblock_file_meta_offset(fs_ctx, idx);
    err = fs_ctx->flash_info->read(fs_ctx->flash_info, fs_ctx->active_metablock,
                                   (uint8_t *)file_meta, offset,
                                   SFS_FILE_METADATA_SIZE);

#ifdef SFS_VALIDATE_METADATA_FROM_FLASH
    if (err == PSA_SUCCESS) {
        err = sfs_mblock_validate_file_meta(fs_ctx, file_meta);
    }
#endif

    return err;
}

psa_status_t sfs_flash_fs_mblock_read_block_metadata(
                                            struct sfs_flash_fs_ctx_t *fs_ctx,
                                            uint32_t lblock,
                                            struct sfs_block_meta_t *block_meta)
{
    psa_status_t err;
    size_t pos;

    pos = sfs_mblock_block_meta_offset(lblock);
    err = fs_ctx->flash_info->read(fs_ctx->flash_info, fs_ctx->active_metablock,
                                   (uint8_t *)block_meta, pos,
                                   SFS_BLOCK_METADATA_SIZE);

#ifdef SFS_VALIDATE_METADATA_FROM_FLASH
    if (err == PSA_SUCCESS) {
        err = sfs_mblock_validate_block_meta(fs_ctx, block_meta);
    }
#endif

    return err;
}

psa_status_t sfs_flash_fs_mblock_reserve_file(
                                            struct sfs_flash_fs_ctx_t *fs_ctx,
                                            const uint8_t *fid,
                                            size_t size,
                                            uint32_t flags,
                                            uint32_t *idx,
                                            struct sfs_file_meta_t *file_meta,
                                            struct sfs_block_meta_t *block_meta)
{
    psa_status_t err;

    err = sfs_mblock_reserve_file(fs_ctx, fid, size, flags, file_meta,
                                  block_meta);

    *idx = sfs_get_free_file_index(fs_ctx);
    if ((err != PSA_SUCCESS) ||
        (*idx == SFS_METADATA_INVALID_INDEX)) {
        return PSA_ERROR_INSUFFICIENT_STORAGE;
    }

    return PSA_SUCCESS;
}

psa_status_t sfs_flash_fs_mblock_reset_metablock(
                                              struct sfs_flash_fs_ctx_t *fs_ctx)
{
    struct sfs_block_meta_t block_meta = { 0 };
    psa_status_t err;
    uint32_t i;
    uint32_t metablock_to_erase_first = SFS_METADATA_BLOCK0;
    struct sfs_file_meta_t file_metadata;

    /* Erase both metadata blocks. If at least one metadata block is valid,
     * ensure that the active metadata block is erased last to prevent rollback
     * in the case of a power failure between the two erases.
     */
    if (sfs_init_get_active_metablock(fs_ctx) == PSA_SUCCESS) {
        metablock_to_erase_first = fs_ctx->scratch_metablock;
    }

    err = fs_ctx->flash_info->erase(fs_ctx->flash_info,
                                    metablock_to_erase_first);
    if (err != PSA_SUCCESS) {
        return err;
    }

    err = fs_ctx->flash_info->erase(fs_ctx->flash_info,
                                SFS_OTHER_META_BLOCK(metablock_to_erase_first));
    if (err != PSA_SUCCESS) {
        return err;
    }

    fs_ctx->meta_block_header.active_swap_count = 0;
    fs_ctx->meta_block_header.scratch_dblock = sfs_init_scratch_dblock(fs_ctx);
    fs_ctx->meta_block_header.fs_version = SFS_SUPPORTED_VERSION;
    fs_ctx->scratch_metablock = SFS_METADATA_BLOCK1;
    fs_ctx->active_metablock = SFS_METADATA_BLOCK0;

    /* Fill the block metadata for logical datablock 0, which has the physical
     * id of the active metadata block. For this datablock, the space available
     * for data is from the end of the metadata to the end of the block.
     */
    block_meta.data_start =
        sfs_mblock_file_meta_offset(fs_ctx, fs_ctx->flash_info->max_num_files);
    block_meta.free_size = fs_ctx->flash_info->block_size
                           - block_meta.data_start;
    block_meta.phy_id = SFS_METADATA_BLOCK0;
    err = sfs_mblock_update_scratch_block_meta(fs_ctx, SFS_LOGICAL_DBLOCK0,
                                               &block_meta);
    if (err != PSA_SUCCESS) {
        return err;
    }

    /* Fill the block metadata for the dedicated datablocks, which have logical
     * ids beginning from 1 and physical ids initially beginning from
     * SFS_INIT_DBLOCK_START. For these datablocks, the space available for
     * data is the entire block.
     */
    block_meta.data_start = 0;
    block_meta.free_size = fs_ctx->flash_info->block_size;
    for (i = 0; i < sfs_num_dedicated_dblocks(fs_ctx); i++) {
        /* If a flash error is detected, the code erases the rest
         * of the blocks anyway to remove all data stored in them.
         */
        err |= fs_ctx->flash_info->erase(fs_ctx->flash_info,
                                         i + sfs_init_dblock_start(fs_ctx));
    }

    /* If an error is detected while erasing the flash, then return a
     * system error to abort core wipe process.
     */
    if (err != PSA_SUCCESS) {
        return PSA_ERROR_STORAGE_FAILURE;
    }

    for (i = 0; i < sfs_num_dedicated_dblocks(fs_ctx); i++) {
        block_meta.phy_id = i + sfs_init_dblock_start(fs_ctx);
        err = sfs_mblock_update_scratch_block_meta(fs_ctx, i + 1, &block_meta);
        if (err != PSA_SUCCESS) {
            return PSA_ERROR_GENERIC_ERROR;
        }
    }

    /* Initialize file metadata table */
    (void)memset(&file_metadata, SFS_DEFAULT_EMPTY_BUFF_VAL,
                     SFS_FILE_METADATA_SIZE);
    for (i = 0; i < fs_ctx->flash_info->max_num_files; i++) {
        /* In the beginning phys id is same as logical id */
        /* Update file metadata to reflect new attributes */
        err = sfs_flash_fs_mblock_update_scratch_file_meta(fs_ctx, i,
                                                           &file_metadata);
        if (err != PSA_SUCCESS) {
            return PSA_ERROR_GENERIC_ERROR;
        }
    }

    err = sfs_mblock_write_scratch_meta_header(fs_ctx);
    if (err != PSA_SUCCESS) {
        return PSA_ERROR_GENERIC_ERROR;
    }

    /* Commit metadata block modifications to flash */
    err = fs_ctx->flash_info->flush(fs_ctx->flash_info);
    if (err != PSA_SUCCESS) {
        return err;
    }

    /* Swap active and scratch metablocks */
    sfs_mblock_swap_metablocks(fs_ctx);

    return PSA_SUCCESS;
}

void sfs_flash_fs_mblock_set_data_scratch(struct sfs_flash_fs_ctx_t *fs_ctx,
                                          uint32_t phy_id, uint32_t lblock)
{
    if (lblock != SFS_LOGICAL_DBLOCK0) {
        fs_ctx->meta_block_header.scratch_dblock = phy_id;
    }
}

psa_status_t sfs_flash_fs_mblock_update_scratch_block_meta(
                                            struct sfs_flash_fs_ctx_t *fs_ctx,
                                            uint32_t lblock,
                                            struct sfs_block_meta_t *block_meta)
{
    psa_status_t err;

    /* If the file is the logical block 0, then update the physical ID to the
     * current scratch metadata block so that it is correct after the metadata
     * blocks are swapped.
     */
    if (lblock == SFS_LOGICAL_DBLOCK0) {
        block_meta->phy_id = fs_ctx->scratch_metablock;
    }

    err = sfs_mblock_update_scratch_block_meta(fs_ctx, lblock, block_meta);
    if (err != PSA_SUCCESS) {
        return PSA_ERROR_GENERIC_ERROR;
    }

    return sfs_mblock_copy_remaining_block_meta(fs_ctx, lblock);
}

psa_status_t sfs_flash_fs_mblock_update_scratch_file_meta(
                                        struct sfs_flash_fs_ctx_t *fs_ctx,
                                        uint32_t idx,
                                        const struct sfs_file_meta_t *file_meta)
{
    size_t pos;

    /* Calculate the position */
    pos = sfs_mblock_file_meta_offset(fs_ctx, idx);
    return fs_ctx->flash_info->write(fs_ctx->flash_info,
                                     fs_ctx->scratch_metablock,
                                     (const uint8_t *)file_meta, pos,
                                     SFS_FILE_METADATA_SIZE);
}
