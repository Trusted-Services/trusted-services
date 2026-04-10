/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <string.h>
#include "storage_partition.h"


void storage_partition_init(
	struct storage_partition *partition,
	const struct uuid_octets *partition_guid,
	size_t num_blocks,
	size_t block_size)
{
	memset(partition, 0, sizeof(struct storage_partition));

	if (partition_guid)
		partition->partition_guid = *partition_guid;
	else
		memset(&partition->partition_guid, 0xff, sizeof(partition->partition_guid));

	partition->block_size = block_size;
	partition->num_blocks = num_blocks;

	partition->base_lba = 0;
	storage_partition_acl_init(&partition->acl);
}

void storage_partition_deinit(
	struct storage_partition *partition)
{
	memset(partition, 0, sizeof(struct storage_partition));
}

bool storage_partition_grant_access(
	struct storage_partition *partition,
	uint32_t client_id)
{
	return storage_partition_acl_add(&partition->acl, client_id);
}

bool storage_partition_assign_ownership(
	struct storage_partition *partition,
	const char *owner_id)
{
	return storage_partition_acl_set_owner_id(&partition->acl, owner_id);
}

bool storage_partition_is_guid_matched(
	const struct storage_partition *partition,
	const struct uuid_octets *partition_guid)
{
	return (memcmp(&partition->partition_guid, partition_guid, sizeof(struct uuid_octets)) == 0);
}

bool storage_partition_is_open_permitted(
	struct storage_partition *partition,
	uint32_t client_id,
	storage_partition_authorizer authorizer)
{
	return storage_partition_acl_authorize(&partition->acl, client_id, authorizer);
}

bool storage_partition_is_access_permitted(
	const struct storage_partition *partition,
	uint32_t client_id)
{
	return storage_partition_acl_check(&partition->acl, client_id);
}

bool storage_partition_is_lba_legal(
	const struct storage_partition *partition,
	uint64_t lba)
{
	return lba < partition->num_blocks;
}

size_t storage_partition_clip_length(
	const struct storage_partition *partition,
	uint64_t lba,
	size_t offset,
	size_t req_len)
{
	size_t clipped_len = 0;

	if (lba < partition->num_blocks) {

		size_t remaining_len = (partition->num_blocks - lba) * partition->block_size;

		remaining_len = (offset < remaining_len) ? remaining_len - offset : 0;
		clipped_len = (req_len > remaining_len) ? remaining_len : req_len;
	}

	return clipped_len;
}

size_t storage_partition_clip_num_blocks(
	const struct storage_partition *partition,
	uint64_t lba,
	size_t num_blocks)
{
	size_t clipped_num_blocks = 0;

	if (lba < partition->num_blocks) {

		size_t remaining_blocks = partition->num_blocks - lba;

		clipped_num_blocks = (num_blocks > remaining_blocks) ? remaining_blocks : num_blocks;
	}

	return clipped_num_blocks;
}
