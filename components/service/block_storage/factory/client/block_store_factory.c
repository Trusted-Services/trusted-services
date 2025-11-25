/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stddef.h>
#include <stdlib.h>
#include "service/block_storage/block_store/client/block_storage_client.h"
#include "protocols/rpc/common/packed-c/encoding.h"
#include "service_locator.h"

struct block_store_assembly {
	struct block_storage_client client;
	struct rpc_caller_session *rpc_session;
	struct service_context *service_context;
};

struct block_store *client_block_store_factory_create(const char *sn)
{
	struct block_store *product = NULL;
	struct block_store_assembly *assembly =
		(struct block_store_assembly *)calloc(1, sizeof(struct block_store_assembly));

	if (assembly) {

		assembly->rpc_session = NULL;
		assembly->service_context = NULL;

		service_locator_init();

		assembly->service_context = service_locator_query(sn);

		if (assembly->service_context) {

			assembly->rpc_session = service_context_open(assembly->service_context);

			if (assembly->rpc_session)
				product = block_storage_client_init(&assembly->client, assembly->rpc_session);
		}

		if (!product) {

			if (assembly->rpc_session != NULL) {
				service_context_close(assembly->service_context, assembly->rpc_session);
				assembly->rpc_session = NULL;
			}
			/* Something went wrong! */
			free(assembly);
		}
	}

	return product;
}

void client_block_store_factory_destroy(struct block_store *block_store)
{
	if (block_store) {

		size_t offset_into_assembly =
			offsetof(struct block_store_assembly, client) +
			offsetof(struct block_storage_client, base_block_store);

		struct block_store_assembly *assembly = (struct block_store_assembly *)
			((uint8_t *)block_store - offset_into_assembly);

		block_storage_client_deinit(&assembly->client);

		if (assembly->service_context) {

			if (assembly->rpc_session) {
				service_context_close(
					assembly->service_context, assembly->rpc_session);
				assembly->rpc_session = NULL;
			}

			service_context_relinquish(assembly->service_context);
			assembly->service_context = NULL;
		}

		free(assembly);
	}
}
