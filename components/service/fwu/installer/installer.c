/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "installer.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

#include "protocols/service/fwu/status.h"

void installer_init(struct installer *installer, enum install_type install_type,
		    uint32_t location_id, const struct uuid_octets *location_uuid, void *context,
		    const struct installer_interface *interface)
{
	assert(installer);
	assert(location_uuid);
	assert(context);
	assert(interface);

	installer->install_type = install_type;
	installer->location_id = location_id;
	installer->location_uuid = *location_uuid;
	installer->context = context;
	installer->interface = interface;

	installer->install_status = FWU_STATUS_SUCCESS;
	installer->is_active = false;
	installer->next = NULL;
}

int installer_begin(struct installer *installer, uint32_t current_volume_id,
		    uint32_t update_volume_id)
{
	assert(installer);
	assert(installer->interface);
	assert(installer->interface->begin);

	installer->install_status = FWU_STATUS_SUCCESS;
	installer->is_active = true;

	return installer->interface->begin(installer->context, current_volume_id, update_volume_id);
}

int installer_finalize(struct installer *installer)
{
	assert(installer);
	assert(installer->interface);
	assert(installer->interface->finalize);

	installer->is_active = false;

	return installer->interface->finalize(installer->context);
}

void installer_abort(struct installer *installer)
{
	assert(installer);
	assert(installer->interface);
	assert(installer->interface->abort);

	installer->is_active = false;

	installer->interface->abort(installer->context);
}

int installer_open(struct installer *installer, const struct image_info *image_info)
{
	assert(installer);
	assert(installer->interface);
	assert(installer->interface->open);

	int status = installer->interface->open(installer->context, image_info);

	if (status && !installer->install_status)
		installer->install_status = status;

	return status;
}

int installer_commit(struct installer *installer)
{
	assert(installer);
	assert(installer->interface);
	assert(installer->interface->commit);

	int status = installer->interface->commit(installer->context);

	if (status && !installer->install_status)
		installer->install_status = status;

	return status;
}

int installer_write(struct installer *installer, const uint8_t *data, size_t data_len)
{
	assert(installer);
	assert(installer->interface);
	assert(installer->interface->write);

	int status = installer->interface->write(installer->context, data, data_len);

	if (status && !installer->install_status)
		installer->install_status = status;

	return status;
}

int installer_enumerate(struct installer *installer, uint32_t volume_id,
			struct fw_directory *fw_directory)
{
	assert(installer);
	assert(installer->interface);
	assert(installer->interface->enumerate);

	return installer->interface->enumerate(installer->context, volume_id, fw_directory);
}
