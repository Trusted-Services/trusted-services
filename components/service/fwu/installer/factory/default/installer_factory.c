/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "service/fwu/installer/factory/installer_factory.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "common/uuid/uuid.h"
#include "service/fwu/installer/copy/copy_installer.h"
#include "service/fwu/installer/factory/locations.h"
#include "service/fwu/installer/installer.h"
#include "service/fwu/installer/raw/raw_installer.h"

/**
 * An installer factory for constructing installers for a platform
 * that conforms to the following constraints:
 *
 * 1. AP firmware is contained within a FIP that resides within a separate
 *    storage volume (* n banks). A volume will normally map to a flash partition.
 * 2. SCP firmware, if present, is always updated as a single unit and resides in
 *    a separate storage volume.
 * 3. RSE firmware, if present, is always updated as a single unit and resides in
 *    a separate storage volume.
 */

static bool check_supported_locations(const char *const supported_uuids[],
				      const struct uuid_octets *location_uuid)
{
	bool is_supported = false;
	unsigned int i = 0;

	while (supported_uuids[i]) {
		struct uuid_octets comparison_uuid;

		uuid_guid_octets_from_canonical(&comparison_uuid, supported_uuids[i]);

		if (uuid_is_equal(comparison_uuid.octets, location_uuid->octets)) {
			is_supported = true;
			break;
		}

		++i;
	}

	return is_supported;
}

struct installer *installer_factory_create_installer(enum install_type installation_type,
						     unsigned int location_id,
						     const struct uuid_octets *location_uuid)
{
	assert(location_uuid);

	struct installer *product = NULL;

	if (installation_type == INSTALL_TYPE_WHOLE_VOLUME) {
#ifdef RAW_INSTALLER_AVAILABLE
		static const char *const raw_installer_compatibility[] = {
			LOCATION_UUID_AP_FW, LOCATION_UUID_SCP_FW, LOCATION_UUID_RSE_FW, NULL
		};

		if (check_supported_locations(raw_installer_compatibility, location_uuid)) {
			struct raw_installer *raw_installer =
				(struct raw_installer *)malloc(sizeof(struct raw_installer));

			if (raw_installer) {
				raw_installer_init(raw_installer, location_uuid, location_id);
				product = &raw_installer->base_installer;
			}
		}
#endif

	} else if (installation_type == INSTALL_TYPE_WHOLE_VOLUME_COPY) {
#ifdef COPY_INSTALLER_AVAILABLE
		static const char *const copy_installer_compatibility[] = {
			LOCATION_UUID_AP_FW, LOCATION_UUID_SCP_FW, LOCATION_UUID_RSE_FW, NULL
		};

		if (check_supported_locations(copy_installer_compatibility, location_uuid)) {
			struct copy_installer *copy_installer =
				(struct copy_installer *)malloc(sizeof(struct copy_installer));

			if (copy_installer) {
				copy_installer_init(copy_installer, location_uuid, location_id);
				product = &copy_installer->base_installer;
			}
		}
#endif
	}

	return product;
}

void installer_factory_destroy_installer(struct installer *installer)
{
	if (installer)
		free(installer->context);
}
