/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef REF_PARTITION_CONFIGURATOR_H
#define REF_PARTITION_CONFIGURATOR_H

#include <stdbool.h>
#include <service/block_storage/block_store/partitioned/partitioned_block_store.h>
#include <media/disk/guid.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * To support test, a reference storage partition configuration is used with
 * a set of different sized partitions. The total backend block store size
 * is kept as small as possible to allow the reference configuration to be
 * used with a ram backed store in environments where available memory is
 * constrained.
 */
/* Make sure to keep ref_partition.c synchronized with these settings! */
#define REF_PARTITION_1_NUM_BLOCKS      (96)
#define REF_PARTITION_2_NUM_BLOCKS      (96)
#define REF_PARTITION_3_NUM_BLOCKS      (4)
#define REF_PARTITION_4_NUM_BLOCKS      (4)
#define REF_PARTITION_BLOCK_SIZE	(512)

#define REF_PARTITION_BACK_STORE_SIZE   (REF_PARTITION_1_NUM_BLOCKS + \
					 REF_PARTITION_2_NUM_BLOCKS + \
					 REF_PARTITION_3_NUM_BLOCKS + \
					 REF_PARTITION_4_NUM_BLOCKS)

/* GPT header size */
#define GPT_HEADER_BLOCKS		(34)

/* Number of blocks including the primary and backup GPT-s */
#define REF_PARTITION_FULL_SIZE		(2 * GPT_HEADER_BLOCKS + REF_PARTITION_BACK_STORE_SIZE)

/* About the right size for PSA storage */
#define REF_PARTITION_1_GUID            DISK_GUID_UNIQUE_PARTITION_PSA_ITS
#define REF_PARTITION_1_STARTING_LBA    (0)
#define REF_PARTITION_1_ENDING_LBA      (REF_PARTITION_1_STARTING_LBA + REF_PARTITION_1_NUM_BLOCKS - 1)

/* Also about the right size for PSA storage */
#define REF_PARTITION_2_GUID            DISK_GUID_UNIQUE_PARTITION_PSA_PS
#define REF_PARTITION_2_STARTING_LBA    (REF_PARTITION_1_ENDING_LBA + 1)
#define REF_PARTITION_2_ENDING_LBA      (REF_PARTITION_2_STARTING_LBA + REF_PARTITION_2_NUM_BLOCKS - 1)

#define REF_PARTITION_3_GUID            DISK_GUID_UNIQUE_PARTITION_PRIMARY_FWU_METADATA
#define REF_PARTITION_3_STARTING_LBA    (REF_PARTITION_2_ENDING_LBA + 1)
#define REF_PARTITION_3_ENDING_LBA      (REF_PARTITION_3_STARTING_LBA + REF_PARTITION_3_NUM_BLOCKS - 1)

#define REF_PARTITION_4_GUID            DISK_GUID_UNIQUE_PARTITION_BACKUP_FWU_METADATA
#define REF_PARTITION_4_STARTING_LBA    (REF_PARTITION_3_ENDING_LBA + 1)
#define REF_PARTITION_4_ENDING_LBA      (REF_PARTITION_4_STARTING_LBA + REF_PARTITION_4_NUM_BLOCKS - 1)

/**
 * \brief Configures a partitioned_block_store with the reference configuration
 *
 * \param[in]  subject  The subject partitioned_block_store
 */
bool ref_partition_configure(struct partitioned_block_store *subject);

#ifdef __cplusplus
}
#endif

#endif /* REF_PARTITION_CONFIGURATOR_H */
