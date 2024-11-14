/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "direct_fw_inspector.h"

#include "media/volume/index/volume_index.h"
#include "protocols/service/fwu/status.h"
#include "service/fwu/agent/fw_directory.h"
#include "service/fwu/fw_store/banked/volume_id.h"
#include "service/fwu/installer/installer.h"
#include "service/fwu/installer/installer_index.h"
#include "trace.h"

int direct_fw_inspector_inspect(struct fw_directory *fw_dir, unsigned int boot_index)
{
	int status = FWU_STATUS_SUCCESS;

	/* The set of registered installers determines which fw components are
	 * updatable as individual units. Each installer is able to provide
	 * a view of the set of images [0..n] that it is capable of updating using
	 * its enumerate method. This view is based on the state of the firmware
	 * at the most recent boot. The set of updatable images may change after
	 * an update because:
	 *  a) The set of images within the boot volume has changed
	 *  b) The set of supported installers has changed
	 *
	 * By iterating over all registered installers, a complete and fresh view
	 * of updatable images is obtained.
	 */
	unsigned int index = 0;

	do {
		struct installer *installer = installer_index_get(index);

		if (!installer)
			break;

		/* To enable an installer to inspect the firmware volume that
		 * was booted from, determined the correct volume_id based on the
		 * most recent boot_index.
		 */
		unsigned int volume_id =
			banked_volume_id(installer->location_id, banked_usage_id(boot_index));

		/* Delegate volume inspection to the installer that will have
		 * the necessary package format knowledge to extract information
		 * about images that it is capable of updating. An installer
		 * will add 0..* image entries to the fw_directory.
		 */
		status = installer_enumerate(installer, volume_id, fw_dir);

		if (status != FWU_STATUS_SUCCESS) {
			EMSG("Failed to enumerate contents of volume %d", volume_id);
			break;
		}

		++index;

	} while (1);

	return status;
}
