/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <CppUTest/TestHarness.h>

#include "common/uuid/uuid.h"
#include "media/disk/guid.h"
#include "service/fwu/installer/factory/installer_factory.h"
#include "service/fwu/installer/factory/locations.h"
#include "service/fwu/installer/installer_index.h"

TEST_GROUP(FwuDefaultInstallerFactoryTests){ void setup(){ installer_index_init();
}

void teardown()
{
	unsigned int i = 0;

	do {
		struct installer *installer = installer_index_get(i);

		if (!installer)
			break;

		installer_factory_destroy_installer(installer);
		++i;

	} while (1);

	installer_index_clear();
}
}
;

TEST(FwuDefaultInstallerFactoryTests, configureInstallersFlow)
{
	struct uuid_octets ap_fw_uuid;
	struct uuid_octets scp_fw_uuid;
	struct uuid_octets rse_fw_uuid;

	/* Check configuration operations that will be performed when
	 * constructing a set of installers for a platform. The platform
	 * consists of multiple firmware locations.  This sequence
	 * represents the configuration steps performed when iterating
	 * over a GPT to identify updatable partitions.
	 */

	/* Configure installers for updating AP firmware */
	uuid_guid_octets_from_canonical(&ap_fw_uuid, LOCATION_UUID_AP_FW);

	/* Expect no installer to initially be registered */
	CHECK_FALSE(installer_index_find_by_location_uuid(&ap_fw_uuid));

	/* Configure for whole volume and copy installation */
	struct installer *installer =
		installer_factory_create_installer(INSTALL_TYPE_WHOLE_VOLUME, 0, &ap_fw_uuid);
	CHECK_TRUE(installer);

	installer_index_register(installer);
	CHECK_TRUE(installer_index_find_by_location_uuid(&ap_fw_uuid));

	installer =
		installer_factory_create_installer(INSTALL_TYPE_WHOLE_VOLUME_COPY, 0, &ap_fw_uuid);
	CHECK_TRUE(installer);

	installer_index_register(installer);
	CHECK_TRUE(installer_index_find_by_location_uuid(&ap_fw_uuid));

	/* Configure installers for updating SCP firmware */
	uuid_guid_octets_from_canonical(&scp_fw_uuid, LOCATION_UUID_SCP_FW);

	/* Expect no installer to initially be registered */
	CHECK_FALSE(installer_index_find_by_location_uuid(&scp_fw_uuid));

	/* Configure for whole volume and copy installation */
	installer = installer_factory_create_installer(INSTALL_TYPE_WHOLE_VOLUME, 0, &scp_fw_uuid);
	CHECK_TRUE(installer);

	installer_index_register(installer);
	CHECK_TRUE(installer_index_find_by_location_uuid(&scp_fw_uuid));

	installer =
		installer_factory_create_installer(INSTALL_TYPE_WHOLE_VOLUME_COPY, 0, &scp_fw_uuid);
	CHECK_TRUE(installer);

	installer_index_register(installer);
	CHECK_TRUE(installer_index_find_by_location_uuid(&scp_fw_uuid));

	/* Configure installers for updating RSE firmware */
	uuid_guid_octets_from_canonical(&rse_fw_uuid, LOCATION_UUID_RSE_FW);

	/* Expect no installer to initially be registered */
	CHECK_FALSE(installer_index_find_by_location_uuid(&rse_fw_uuid));

	/* Configure for whole volume and copy installation */
	installer = installer_factory_create_installer(INSTALL_TYPE_WHOLE_VOLUME, 0, &rse_fw_uuid);
	CHECK_TRUE(installer);

	installer_index_register(installer);
	CHECK_TRUE(installer_index_find_by_location_uuid(&rse_fw_uuid));

	installer =
		installer_factory_create_installer(INSTALL_TYPE_WHOLE_VOLUME_COPY, 0, &rse_fw_uuid);
	CHECK_TRUE(installer);

	installer_index_register(installer);
	CHECK_TRUE(installer_index_find_by_location_uuid(&rse_fw_uuid));

	/* Now try and construct an installer for an unsupported partition type. */
	struct uuid_octets unsupported_location_uuid;

	uuid_guid_octets_from_canonical(&unsupported_location_uuid,
					DISK_GUID_PARTITION_TYPE_FWU_METADATA);

	installer = installer_factory_create_installer(INSTALL_TYPE_WHOLE_VOLUME, 0,
						       &unsupported_location_uuid);
	CHECK_FALSE(installer);

	CHECK_FALSE(installer_index_find_by_location_uuid(&unsupported_location_uuid));
}
