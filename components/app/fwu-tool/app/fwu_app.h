/*
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FWU_APP_H
#define FWU_APP_H

#include <stdint.h>
#include <vector>

#include "common/uuid/uuid.h"
#include "service/fwu/common/update_agent_interface.h"
#include "service/fwu/fw_store/banked/banked_fw_store.h"

/*
 * The fwu_app class is intended to provide the core for an application
 * that uses the update_agent for updating the contents of a disk image
 * file. The app uses standard fwu components, such as the update_agent,
 * to manage updates. To support additional update methods (e.g.
 * update_capsule), inherit from this class and add additional methods
 * to the derived class. This allows the core app to be reused for
 * different application areas without overloading the base app.
 */
class fwu_app {
public:
	fwu_app();
	virtual ~fwu_app();

	/**
	 * \brief Configure storage and installers
	 *
	 * Configures a set of storage volumes and installers to manage the
	 * contents of the specified disk image file.
	 *
	 * \param[in]  disk_img_filename   UEFI formatted disk image
	 *
	 * \return Status (0 on success)
	 */
	int configure(const char *disk_img_filename);

	/**
	 * \brief Get boot info from the FWU metadata
	 *
	 * \param[out]  active_index   The active index seen by the bootloader
	 * \param[out]  metadata_version Current metadata version
	 *
	 * \return Status (0 on success)
	 */
	int get_boot_info(unsigned int &active_index, unsigned int &metadata_version);

	/**
	 * \brief Initialise the update agent
	 *
	 * \param[in]  boot_index   The boot_index chosen by the bootloader
	 * \param[in]  metadata_version Current metadata version
	 *
	 * \return Status (0 on success)
	 */
	int init_update_agent(unsigned int boot_index, unsigned int metadata_version);

	/**
	 * \brief Update a single image
	 *
	 * Begins staging, writes the raw contents of the image file and ends
	 * staging.
	 *
	 * \param[in]  img_type_uuid   UUID of image to update
	 * \param[in]  img_data        Buffer containing image data
	 * \param[in]  img_size        Size in bytes of image
	 *
	 * \return Status (0 on success)
	 */
	int update_image(const struct uuid_octets &img_type_uuid, const uint8_t *img_data,
			 size_t img_size);

	/**
	 * \brief Read an object from the update agent
	 *
	 * \param[in]  object_uuid    UUID of object
	 * \param[out] data           Read object data
	 *
	 * \return Status (0 on success)
	 */
	int read_object(const struct uuid_octets &object_uuid, std::vector<uint8_t> &data);

protected:
	/**
	 * \brief Return pointer to update_agent struct
	 *
	 * \return Pointer to the core update_agent.
	 */
	struct update_agent *update_agent();

private:
	static const size_t MAX_STORAGE_DEVICES = 4;

	static const struct metadata_serializer *select_metadata_serializer(unsigned int version);

	struct update_agent *m_update_agent;
	struct fw_store m_fw_store;
};

#endif /* FWU_APP_H */
