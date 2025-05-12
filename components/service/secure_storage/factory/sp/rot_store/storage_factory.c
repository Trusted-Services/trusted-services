/*
 * Copyright (c) 2021-2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/**
 * A storage factory that creates storage backends that may be used
 * to access a secure storage partition from a separate SP within the
 * device RoT.  Defaults to using PSA storage partitions if no runtime
 * configuration overrides the target service endpoint to use.  If multiple
 * candidate storage SPs are available, the one that matches the
 * requested storage class is used.  The availability of Internal Trusted
 * and Protected stores will depend on the platform.
 */
#include "rpc/ts_rpc/caller/sp/ts_rpc_caller_sp.h"
#include "rpc/common/caller/rpc_caller_session.h"
#include <protocols/rpc/common/packed-c/status.h>
#include <service/secure_storage/backend/secure_storage_client/secure_storage_client.h>
#include <service/secure_storage/backend/null_store/null_store.h>
#include <service/secure_storage/factory/storage_factory.h>
#include "service/secure_storage/frontend/secure_storage_provider/secure_storage_uuid.h"
#include <ffa_api.h>
#include "sp_discovery.h"
#include <stdbool.h>
#include <stddef.h>

#ifndef STORAGE_CLIENT_RPC_BUF_SIZE
#define STORAGE_CLIENT_RPC_BUF_SIZE (4096)
#endif

/* Defaults to using PSA storage partitions if no external configuration specified */
#define MAX_CANDIDATE_UUIDS		(2)

static const struct rpc_uuid default_internal_store_uuid = {
	.uuid = TS_PSA_INTERNAL_TRUSTED_STORAGE_UUID
};

static const struct rpc_uuid default_protected_store_uuid = {
	.uuid = TS_PSA_PROTECTED_STORAGE_UUID
};

/* The storage backed specialization constructed by this factory */
struct rot_store
{
	struct secure_storage_client secure_storage_client;
	struct rpc_caller_interface caller;
	struct rpc_caller_session session;
	bool in_use;
};

/* Only supports construction of a single instance */
static struct rot_store backend_instance = { .in_use = false };

/* Used on failure if no association with a storage provider is established */
static struct null_store null_store;

static int select_candidate_uuids(const struct rpc_uuid *candidates[], int max_candidates,
				  enum storage_factory_security_class security_class);


struct storage_backend *storage_factory_create(
			enum storage_factory_security_class security_class)
{
	struct rot_store *new_backend = &backend_instance;
	const struct rpc_uuid *candidate_uuids[MAX_CANDIDATE_UUIDS];
	int num_candidate_uuids = select_candidate_uuids(candidate_uuids, MAX_CANDIDATE_UUIDS,
							 security_class);

	struct storage_backend *result = NULL;

	if (num_candidate_uuids && !new_backend->in_use) {
		rpc_status_t rpc_status = RPC_ERROR_INTERNAL;

		rpc_status = ts_rpc_caller_sp_init(&new_backend->caller);
		if (rpc_status != RPC_SUCCESS)
			return NULL;

		for (int i = 0; i < num_candidate_uuids; i++) {
			rpc_status = rpc_caller_session_find_and_open(&new_backend->session,
								      &new_backend->caller,
								      candidate_uuids[i],
								      STORAGE_CLIENT_RPC_BUF_SIZE);

			if (rpc_status == RPC_SUCCESS) {
				result = secure_storage_client_init(
					&new_backend->secure_storage_client,
					&new_backend->session);
				break;
			}
		}

		new_backend->in_use = (result != NULL);
	}

	if (!result) {
		ts_rpc_caller_sp_deinit(&new_backend->caller);

		/**
		 * Errors during SP initialisation can be difficult to handle so
		 * returns a valid storage_backend, albeit one that just returns
		 * an appropriate status code if any methods are called.  This
		 * allows an error to be reported to a requesting client where
		 * it may be easier to handle.
		 */
		result = null_store_init(&null_store);
	}

	return result;
}

void storage_factory_destroy(struct storage_backend *backend)
{
	if (backend) {

		secure_storage_client_deinit(&backend_instance.secure_storage_client);
		rpc_caller_session_close(&backend_instance.session);
		ts_rpc_caller_sp_deinit(&backend_instance.caller);
		backend_instance.in_use = false;
	}
}

static int select_candidate_uuids(const struct rpc_uuid *candidates[], int max_candidates,
				  enum storage_factory_security_class security_class)
{
	/* Runtime configuration not yet supported so fallback to using default UUIDs */
	int num_candidates = 0;

	if (max_candidates >= 2) {
		if (security_class == storage_factory_security_class_INTERNAL_TRUSTED) {
			candidates[0] = &default_internal_store_uuid;
			candidates[1] = &default_protected_store_uuid;
		} else {
			candidates[0] = &default_protected_store_uuid;
			candidates[1] = &default_internal_store_uuid;
		}

		num_candidates = 2;
	}

	return num_candidates;
}