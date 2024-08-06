/*
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ps_service_context.h"
#include "service/block_storage/factory/client/block_store_factory.h"
#include "service/secure_storage/backend/secure_flash_store/secure_flash_store.h"
#include "service/secure_storage/frontend/secure_storage_provider/secure_storage_uuid.h"
#include "media/disk/guid.h"
#include <assert.h>
#include <compiler.h>

ps_service_context::ps_service_context(const char *sn) :
	standalone_service_context(sn),
	m_storage_provider(),
	m_sfs_flash_adapter(),
	m_block_store(NULL)
{

}

ps_service_context::~ps_service_context()
{

}

void ps_service_context::do_init()
{
	struct uuid_octets guid;
	const struct sfs_flash_info_t *flash_info = NULL;
	const struct rpc_uuid service_uuid = {.uuid = TS_PSA_PROTECTED_STORAGE_UUID };

	uuid_guid_octets_from_canonical(&guid,
		DISK_GUID_UNIQUE_PARTITION_PSA_PS);

	m_block_store = client_block_store_factory_create("sn:trustedfirmware.org:block-storage:0");
	assert(m_block_store != NULL);

	__maybe_unused psa_status_t status = sfs_flash_block_store_adapter_init(
		&m_sfs_flash_adapter,
		0,
		m_block_store,
		&guid,
		MIN_FLASH_BLOCK_SIZE,
		MAX_NUM_FILES,
		&flash_info);

	assert(status == PSA_SUCCESS);

	struct storage_backend *storage_backend = sfs_init(flash_info);
	struct rpc_service_interface *storage_ep = secure_storage_provider_init(
			&m_storage_provider, storage_backend, &service_uuid);

	standalone_service_context::set_rpc_interface(storage_ep);
}

void ps_service_context::do_deinit()
{
	secure_storage_provider_deinit(&m_storage_provider);
	client_block_store_factory_destroy(m_block_store);
	m_block_store = NULL;
}
