/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "block_store_factory.h"
#include "service/block_storage/block_store/device/rpmb/rpmb_block_store.h"
#include "service/block_storage/block_store/partitioned/partitioned_block_store.h"
#include "service/block_storage/config/gpt/gpt_partition_configurator.h"
#include "service/block_storage/config/ref/ref_partition_configurator.h"
#include "service/rpmb/frontend/platform/default/rpmb_platform_default.h"
#include "service/rpmb/client/rpmb_client.h"
#include "service/locator/interface/service_locator.h"
#include <media/volume/index/volume_index.h>
#include <media/volume/block_volume/block_volume.h>
#include <media/disk/disk_images/ref_partition.h>
#include <media/disk/formatter/disk_formatter.h>

struct block_store_assembly {
	struct rpmb_block_store rpmb_block_store;
	struct service_context *service_context;
	struct rpc_caller_session *rpc_session;
	struct rpmb_client rpmb_client;

	struct rpmb_frontend rpmb_frontend;
	struct rpmb_platform_default rpmb_platform;
	struct rpmb_backend rpmb_backend;

	struct partitioned_block_store partitioned_block_store;
	struct block_volume volume;
};

struct block_store *rpmb_block_store_factory_create(void)
{
	struct block_store *product = NULL;
	struct block_store *rpmb_store = NULL;
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;
	struct rpmb_platform *platform = NULL;
	struct rpmb_backend *backend = NULL;
	struct block_store_assembly *assembly = NULL;
	struct uuid_octets back_store_guid = { 0 };
	struct volume *volume = NULL;
	struct uuid_octets disk_guid = { 0 };

	assembly = (struct block_store_assembly *)calloc(1, sizeof(struct block_store_assembly));
	if (!assembly)
		return NULL;

	service_locator_init();

	assembly->service_context = service_locator_query("sn:trustedfirmware.org:rpmb:0");
	if (!assembly->service_context)
		goto error5;

	assembly->rpc_session = service_context_open(assembly->service_context);
	if (!assembly->rpc_session)
		goto error5;

	backend = rpmb_client_init(&assembly->rpmb_client, assembly->rpc_session);
	if (!backend)
		goto error4;

	platform = rpmb_platform_default_init(&assembly->rpmb_platform);
	if (!platform)
		goto error3;

	status = rpmb_frontend_create(&assembly->rpmb_frontend, platform, backend, 0);
	if (status != PSA_SUCCESS)
		goto error3;

	status = rpmb_frontend_init(&assembly->rpmb_frontend);
	if (status != PSA_SUCCESS)
		goto error3;

	rpmb_store = rpmb_block_store_init(&assembly->rpmb_block_store, &back_store_guid,
					   &assembly->rpmb_frontend, REF_PARTITION_BLOCK_SIZE);
	if (!rpmb_store)
		goto error2;

	volume_index_init();

	if (block_volume_init(&assembly->volume, rpmb_store, &disk_guid, &volume) ||
	    disk_formatter_clone(volume->dev_handle, volume->io_spec, ref_partition_data,
				 ref_partition_data_length))
		goto error1;

	if (volume_index_add(VOLUME_ID_SECURE_FLASH, volume))
		goto error1;

	product = partitioned_block_store_init(&assembly->partitioned_block_store, 0,
					       &disk_guid, rpmb_store, NULL);
	if (!product)
		goto error1;

	if (!gpt_partition_configure(&assembly->partitioned_block_store, VOLUME_ID_SECURE_FLASH))
		goto error1;

	return product;

error1:
	rpmb_block_store_deinit(&assembly->rpmb_block_store);

error2:
	rpmb_frontend_destroy(&assembly->rpmb_frontend);

error3:
	rpmb_client_deinit(&assembly->rpmb_client);

error4:
	service_context_close(assembly->service_context, assembly->rpc_session);

error5:
	free(assembly);

	return NULL;
}

void rpmb_block_store_factory_destroy(struct block_store *block_store)
{
	if (!block_store)
		return;

	size_t offset_into_assembly =
		offsetof(struct block_store_assembly, partitioned_block_store) +
		offsetof(struct partitioned_block_store, base_block_store);

	struct block_store_assembly *assembly = (struct block_store_assembly *)
		((uint8_t *)block_store - offset_into_assembly);

	partitioned_block_store_deinit(&assembly->partitioned_block_store);

	rpmb_block_store_deinit(&assembly->rpmb_block_store);
	rpmb_frontend_destroy(&assembly->rpmb_frontend);
	rpmb_client_deinit(&assembly->rpmb_client);
	service_context_close(assembly->service_context, assembly->rpc_session);

	free(assembly);
}
