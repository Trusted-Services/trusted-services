/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "banked_fw_store.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "bank_scheme.h"
#include "common/uuid/uuid.h"
#include "protocols/service/fwu/fwu_proto.h"
#include "protocols/service/fwu/status.h"
#include "service/fwu/installer/installer.h"
#include "service/fwu/installer/installer_index.h"
#include "volume_id.h"

static int activate_installer(struct fw_store *fw_store, struct installer *installer,
			      uint32_t location_id)
{
	int status = FWU_STATUS_UNKNOWN;

	/* An image has been presented that requires a new installer to
	 * be activated. This involves associating the installer with
	 * the correct storage volumes that reflect the current bank
	 * state. Active installers are held in a linked list during
	 * the installation phase of an update transaction.
	 */
	unsigned int update_volume_id =
		banked_volume_id(location_id, banked_usage_id(fw_store->update_index));

	unsigned int current_volume_id =
		banked_volume_id(location_id, banked_usage_id(fw_store->boot_index));

	status = installer_begin(installer, current_volume_id, update_volume_id);

	if (status == FWU_STATUS_SUCCESS) {
		/* Add to list of active installers */
		installer->next = fw_store->active_installers;
		fw_store->active_installers = installer;
	}

	return status;
}

static void copy_accepted_state_for_location(struct fw_store *fw_store, uint32_t location_id)
{
	size_t index = 0;

	while (1) {
		const struct image_info *image_info =
			fw_directory_get_image_info(fw_store->fw_directory, index);

		if (image_info) {
			if (image_info->location_id == location_id)
				bank_tracker_copy_accept(&fw_store->bank_tracker,
							 fw_store->boot_index,
							 fw_store->update_index,
							 image_info->image_index);
		} else
			break;

		++index;
	}
}

static int install_unchanged_images(struct fw_store *fw_store)
{
	int status = FWU_STATUS_SUCCESS;
	size_t num_locations = 0;
	const uint32_t *location_ids = installer_index_get_location_ids(&num_locations);

	/* Iterate over each of the location_ids that require updating for a viable update.
	 * If there are locations without an active installer, attempt to copy from the
	 * currently active bank.
	 */
	for (size_t i = 0; !status && i < num_locations; i++) {
		bool is_installer_found = false;
		uint32_t location_id = location_ids[i];
		struct installer *installer = fw_store->active_installers;

		while (installer) {
			if ((installer->location_id == location_id) &&
			    !installer_status(installer)) {
				/* There was an installer for this location and the installation
				 * went without errors.
				 */
				is_installer_found = true;
				break;
			}

			installer = installer->next;
		}

		if (!is_installer_found) {
			/* There is no installer for the location so assume the active bank
			 * should be copied. This relies on the platform integrator providing
			 * a suitable installer to do the copying. It's a legitimate platform
			 * configuration to not provide one and only allow updates that consist
			 * of images for all locations.
			 */
			struct installer *copy_installer =
				installer_index_find(INSTALL_TYPE_WHOLE_VOLUME_COPY, location_id);

			if (copy_installer) {
				/* A copy installer doesn't accept any external data. Instead, it
				 * copies from the current volume to the update volume during the
				 * finalize operation.
				 */
				unsigned int update_volume_id = banked_volume_id(
					location_id, banked_usage_id(fw_store->update_index));

				unsigned int current_volume_id = banked_volume_id(
					location_id, banked_usage_id(fw_store->boot_index));

				status = installer_begin(copy_installer, current_volume_id,
							 update_volume_id);

				if (status == FWU_STATUS_SUCCESS) {
					status = installer_finalize(copy_installer);

					/* If a whole volume image was successfully copied from
					 * the currently active bank to the update bank, also copy the
					 * corresponding accepted state for all images associated with the
					 * location. This saves a client from having to re-accept images
					 * that have already been accepted during a previous update.
					 */
					if (status == FWU_STATUS_SUCCESS)
						copy_accepted_state_for_location(fw_store,
										 location_id);
				}

			} else {
				/* Platform configuration doesn't include a suitable copy installer
				 * so treat this update attempt as not viable.
				 */
				status = FWU_STATUS_NOT_AVAILABLE;
			}
		}
	}

	return status;
}

int banked_fw_store_init(struct fw_store *fw_store, const struct metadata_serializer *serializer)
{
	fw_store->fw_directory = NULL;
	fw_store->active_installers = NULL;

	fw_store->update_index = 0;
	fw_store->boot_index = 0;

	bank_tracker_init(&fw_store->bank_tracker);

	return metadata_manager_init(&fw_store->metadata_manager, serializer);
}

void banked_fw_store_deinit(struct fw_store *fw_store)
{
	bank_tracker_deinit(&fw_store->bank_tracker);
	metadata_manager_deinit(&fw_store->metadata_manager);
}

int fw_store_synchronize(struct fw_store *fw_store, struct fw_directory *fw_dir,
			 unsigned int boot_index)
{
	int status = FWU_STATUS_UNKNOWN;

	/* Associate with the fw_directory */
	fw_store->fw_directory = fw_dir;

	/* Note the boot index decision made by the boot loader and choose an
	 * alternative index for a prospective update.
	 */
	fw_store->boot_index = boot_index;
	fw_store->update_index = bank_scheme_next_index(boot_index);

	/* Start building a view of the boot_info that will be advertised by the fw_directory */
	struct boot_info boot_info = { 0 };

	boot_info.boot_index = boot_index;

	/* Ensure that FWU metadata is in a good state. Corruption can occur
	 * due to power failures. The following steps will repair a corrupted
	 * metadata copy or generate fresh metadata if necessary.
	 */
	status = metadata_manager_check_and_repair(&fw_store->metadata_manager, fw_dir);

	if (status != FWU_STATUS_SUCCESS) {
		/* No viable metadata exists so a fresh version needs to be created. We
		 * can only assume that the boot_index is good. Also assume that it is
		 * fully accepted as there is no knowledge that a rollback to a previous
		 * bank is possible.
		 */
		boot_info.active_index = boot_index;
		boot_info.previous_active_index = boot_index;

		bank_tracker_set_holds_accepted_content(&fw_store->bank_tracker,
							fw_store->boot_index);

		status = metadata_manager_update(&fw_store->metadata_manager,
						 boot_info.active_index,
						 boot_info.previous_active_index, fw_dir,
						 &fw_store->bank_tracker);

	} else {
		/* Metadata was successfully loaded from storage so synchronize local
		 * state to the NV state reflected by the metadata.
		 */
		status = metadata_manager_get_active_indices(&fw_store->metadata_manager,
							     &boot_info.active_index,
							     &boot_info.previous_active_index);

		/* Synchronize image approval state with NV information contained in the metadata */
		metadata_manager_preload_bank_tracker(&fw_store->metadata_manager,
						      &fw_store->bank_tracker);

		/* Check for the case where the bootloader has not booted from the active bank.
		 * This will occur when boot loader conditions for a successful boot are not
		 * met and it was necessary to fallback to a previous bank. To prevent
		 * repeated failed boots, the metadata is updated in-line with the
		 * bootloader's decision.
		 */
		if ((status == FWU_STATUS_SUCCESS) && (boot_index != boot_info.active_index)) {
			boot_info.active_index = boot_index;
			boot_info.previous_active_index = boot_index;

			status = metadata_manager_update(&fw_store->metadata_manager,
							 boot_info.active_index,
							 boot_info.previous_active_index, fw_dir,
							 &fw_store->bank_tracker);
		}
	}

	/* Synchronize the fw_directory's view of the boot info */
	fw_directory_set_boot_info(fw_dir, &boot_info);

	return status;
}

int fw_store_begin_install(struct fw_store *fw_store)
{
	assert(!fw_store->active_installers);

	/* Begin the update transaction with the update bank marked as holding
	 * no content and all updatable images in the unaccepted state. Installed
	 * images may be committed as accepted after installation or accepted
	 * during the trial state.
	 */
	bank_tracker_set_no_content(&fw_store->bank_tracker, fw_store->update_index);

	/* Protect the update bank from use while the installation is in
	 * progress by setting the active and previous active indices to be equal.
	 * If a system restart occurs during installation, this prevents the
	 * possibility of the boot loader attempting to boot from a bank in a partially
	 * installed state.
	 */
	int status = metadata_manager_update(&fw_store->metadata_manager, fw_store->boot_index,
					     fw_store->boot_index, fw_store->fw_directory,
					     &fw_store->bank_tracker);

	return status;
}

void fw_store_cancel_install(struct fw_store *fw_store)
{
	/* Abort all active installers - each installer will do its best to
	 * clean-up to a state where a follow-in installation is possible.
	 */
	while (fw_store->active_installers) {
		struct installer *installer = fw_store->active_installers;

		fw_store->active_installers = installer->next;

		installer_abort(installer);
	}
}

int fw_store_finalize_install(struct fw_store *fw_store)
{
	int status = FWU_STATUS_NOT_AVAILABLE;

	/* Treat no active installers as an error. This would occur if no
	 * images where actually installed during the transaction.
	 */
	bool is_error = !fw_store->active_installers;

	/* If there are firmware locations that are unchanged after
	 * installation, it may be necessary to copy active images to
	 * the update bank to ensure that the update bank is fully
	 * populated. This will be needed if an incoming update package
	 * only contains images for some locations.
	 */
	if (!is_error) {
		status = install_unchanged_images(fw_store);
		is_error = (status != FWU_STATUS_SUCCESS);
	}

	/* Finalize all active installers - each installer will perform any
	 * actions needed to make installed images ready for use. For example,
	 * for a component installer where only a subset of images were updated,
	 * the finalize step can be used to copy any images that weren't updated
	 * from the currently active storage volume.
	 */
	while (fw_store->active_installers) {
		struct installer *installer = fw_store->active_installers;

		fw_store->active_installers = installer->next;

		if (!is_error) {
			status = installer_finalize(installer);
			is_error = (status != FWU_STATUS_SUCCESS);

		} else
			installer_abort(installer);
	}

	if (is_error) {
		return (status != FWU_STATUS_SUCCESS) ? status : FWU_STATUS_NOT_AVAILABLE;
	}

	/* All installers finalized successfully so mark update bank as holding
	 * content and signal to boot loader that the update is ready for a trial
	 * by promoting the update bank to being the active bank.
	 */
	bank_tracker_set_holds_content(&fw_store->bank_tracker, fw_store->update_index);

	status = metadata_manager_update(&fw_store->metadata_manager, fw_store->update_index,
					 fw_store->boot_index, fw_store->fw_directory,
					 &fw_store->bank_tracker);

	return status;
}

int fw_store_select_installer(struct fw_store *fw_store, const struct image_info *image_info,
			      struct installer **installer)
{
	int status = FWU_STATUS_UNKNOWN;

	*installer = NULL;

	struct installer *selected_installer =
		installer_index_find(image_info->install_type, image_info->location_id);

	if (selected_installer) {
		/* An installer is available to handle the incoming image. An installer
		 * will potentially handle multiple images as part of an update transaction.
		 * If this is the first, the installer needs to be activated.
		 */
		if (installer_is_active(selected_installer) ||
		    (status = activate_installer(fw_store, selected_installer,
						 image_info->location_id),
		     status == FWU_STATUS_SUCCESS)) {
			status = installer_open(selected_installer, image_info);

			if (status == FWU_STATUS_SUCCESS)
				*installer = selected_installer;
		}
	}

	return status;
}

int fw_store_write_image(struct fw_store *fw_store, struct installer *installer,
			 const uint8_t *data, size_t data_len)
{
	if (!installer_is_active(installer))
		/* Attempting to write to an inactive installer */
		return FWU_STATUS_DENIED;

	int status = installer_write(installer, data, data_len);

	return status;
}

int fw_store_commit_image(struct fw_store *fw_store, struct installer *installer,
			  const struct image_info *image_info, bool accepted)
{
	if (!installer_is_active(installer))
		/* Attempting to commit an inactive installer */
		return FWU_STATUS_DENIED;

	int status = installer_commit(installer);

	if ((status == FWU_STATUS_SUCCESS) && accepted)
		bank_tracker_accept(&fw_store->bank_tracker, fw_store->update_index,
				    image_info->image_index);

	return status;
}

bool fw_store_notify_accepted(struct fw_store *fw_store, const struct image_info *image_info)
{
	unsigned int num_images = fw_directory_num_images(fw_store->fw_directory);

	bank_tracker_accept(&fw_store->bank_tracker, fw_store->boot_index, image_info->image_index);

	int status = metadata_manager_update(&fw_store->metadata_manager, fw_store->boot_index,
					     fw_store->update_index, fw_store->fw_directory,
					     &fw_store->bank_tracker);

	return (status == FWU_STATUS_SUCCESS) &&
	       bank_tracker_is_all_accepted(&fw_store->bank_tracker, fw_store->boot_index,
					    num_images);
}

bool fw_store_is_accepted(const struct fw_store *fw_store, const struct image_info *image_info)
{
	return bank_tracker_is_accepted(&fw_store->bank_tracker, fw_store->boot_index,
					image_info->image_index);
}

bool fw_store_is_trial(const struct fw_store *fw_store)
{
	const struct boot_info *boot_info = fw_directory_get_boot_info(fw_store->fw_directory);
	unsigned int num_images = fw_directory_num_images(fw_store->fw_directory);

	return (boot_info->boot_index == boot_info->active_index) &&
	       !bank_tracker_is_all_accepted(&fw_store->bank_tracker, fw_store->boot_index,
					     num_images);
}

int fw_store_commit_to_update(struct fw_store *fw_store)
{
	(void)fw_store;

	/* Currently, the final commitment to an update is made by the boot loader
	 * when anti-rollback counters are advanced after a reboot. For deployments
	 * where anti-rollback counters are managed by the update agent, the necessary
	 * management logic should be add here.
	 */
	return FWU_STATUS_SUCCESS;
}

int fw_store_revert_to_previous(struct fw_store *fw_store)
{
	uint32_t active_index;
	uint32_t previous_active_index;

	int status = metadata_manager_get_active_indices(&fw_store->metadata_manager, &active_index,
							 &previous_active_index);

	if (status)
		return status;

	if (active_index == fw_store->boot_index) {
		/* Update has been activated via a reboot */
		active_index = previous_active_index;
		previous_active_index = fw_store->boot_index;
		fw_store->update_index = bank_scheme_next_index(active_index);

	} else {
		/* Update has not yet been activated */
		previous_active_index = active_index;
		active_index = fw_store->boot_index;
		fw_store->update_index = bank_scheme_next_index(active_index);
	}

	/* Ensure all images for the new active bank are marked as accepted */
	bank_tracker_set_holds_accepted_content(&fw_store->bank_tracker, active_index);

	/* Update the FWU metadata to the pre-update state */
	status = metadata_manager_update(&fw_store->metadata_manager, active_index,
					 previous_active_index, fw_store->fw_directory,
					 &fw_store->bank_tracker);

	return status;
}

bool fw_store_export(struct fw_store *fw_store, const struct uuid_octets *uuid,
		     const uint8_t **data, size_t *data_len, int *status)
{
	struct uuid_octets target_uuid;

	/* Check for request to export the FWU metadata */
	uuid_guid_octets_from_canonical(&target_uuid, FWU_METADATA_CANONICAL_UUID);

	if (uuid_is_equal(uuid->octets, target_uuid.octets)) {
		bool is_dirty;

		*status = metadata_manager_fetch(&fw_store->metadata_manager, data, data_len,
						 &is_dirty);

		/* is_dirty value is not yet returned when exporting the FWU Metadata but this
		 * may be added to allow for deployments where metadata is written by the client
		 * to allow unnecessary writes to be avoided.
		 */

		return true;
	}

	return false;
}
