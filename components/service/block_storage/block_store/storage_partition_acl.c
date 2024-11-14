/*
 * Copyright (c) 2022-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <string.h>
#include "storage_partition_acl.h"

void storage_partition_acl_init(
	struct storage_partition_acl *acl)
{
	memset(acl, 0, sizeof(struct storage_partition_acl));
}

bool storage_partition_acl_add(
	struct storage_partition_acl *acl,
	uint32_t client_id)
{
	if (acl->allowlist_len < STORAGE_PARTITION_ACL_ALLOWLIST_LEN) {

		acl->allowlist[acl->allowlist_len] = client_id;
		++acl->allowlist_len;

		return true;
	}

	return false;
}

bool storage_partition_acl_set_owner_id(
	struct storage_partition_acl *acl,
	const char *owner_id)
{
	size_t len = strlen(owner_id) + 1;

	if (len > sizeof(acl->owner_id))
		return false;

	memcpy(acl->owner_id, owner_id, len);
	return true;
}

bool storage_partition_acl_check(
	const struct storage_partition_acl *acl,
	uint32_t client_id)
{
	bool is_access_permitted = false;

	for (size_t i = 0; !is_access_permitted && (i < acl->allowlist_len); ++i)
		is_access_permitted = (client_id == acl->allowlist[i]);

	return is_access_permitted;
}

bool storage_partition_acl_authorize(
	struct storage_partition_acl *acl,
	uint32_t client_id,
	storage_partition_authorizer authorizer)
{
	bool is_access_permitted = storage_partition_acl_check(acl, client_id);

	if (!is_access_permitted) {

		/* Client ID is not currently allowlisted. Allow a deployment
		 * specific authorizer to implement its own policy or check that
		 * the present client ID resolves to the configured owner ID for
		 * the storage partition.
		 */
		if (authorizer) {

			if (authorizer(client_id, acl->owner_id))
				is_access_permitted = storage_partition_acl_add(acl, client_id);
		} else {

			/* If no authorizer is configured, and no owner_id string has been
			 * configured, allow access to the first client to request access.
			 * This allows for an access policy where clients request access during
			 * initialization and the permission granted on the initial request
			 * persists. This policy could be suitable for secure world clients
			 * where access permission to partitions can be claimed prior to
			 * normal world boot-up.
			 */
			if (!acl->allowlist_len && !acl->owner_id[0])
				is_access_permitted = storage_partition_acl_add(acl, client_id);
		}
	}

	return is_access_permitted;
}
