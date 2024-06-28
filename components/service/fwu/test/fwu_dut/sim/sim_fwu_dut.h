/*
 * Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SIM_FWU_DUT_H
#define SIM_FWU_DUT_H

#include <cstddef>
#include <string>

#include "common/uuid/uuid.h"
#include "media/volume/block_volume/block_volume.h"
#include "service/block_storage/block_store/device/ram/ram_block_store.h"
#include "service/block_storage/block_store/partitioned/partitioned_block_store.h"
#include "service/fwu/agent/fw_directory.h"
#include "service/fwu/common/update_agent_interface.h"
#include "service/fwu/fw_store/banked/bank_scheme.h"
#include "service/fwu/fw_store/banked/banked_fw_store.h"
#include "service/fwu/installer/copy/copy_installer.h"
#include "service/fwu/installer/raw/raw_installer.h"
#include "service/fwu/provider/fwu_provider.h"
#include "service/fwu/test/fwu_client/fwu_client.h"
#include "service/fwu/test/fwu_client/direct/direct_fwu_client.h"
#include "service/fwu/test/fwu_dut/fwu_dut.h"
#include "service/fwu/test/metadata_checker/metadata_checker.h"

/*
 * An sim_fwu_dut is an aggregate of components that simulates
 * an updatable device that receives updates via the interface presented
 * by an fwu_client. A certain amount of construction-time configuration is
 * supported to allow for testing with alternative firmware store
 * realizations. As much as possible, the set of components that forms
 * the DUT is the same as what is used in real deployments.
 */
class sim_fwu_dut : public fwu_dut {
public:
	/**
	 * \brief sim_fwu_dut constructor
	 *
	 * \param[in]  num_locations  The number of updatable fw locations
	 * \param[in]  metadata_version  FWU metadata version supported by bootloader
	 * \param[in]  allow_partial_updates True if updating a subset of locations is permitted
	 */
	sim_fwu_dut(unsigned int num_locations, unsigned int metadata_version,
		    bool allow_partial_updates = false);

	~sim_fwu_dut();

	void boot(bool from_active_bank = true);
	void shutdown(void);

	struct boot_info get_boot_info(void) const;

	metadata_checker *create_metadata_checker(bool is_primary = true) const;
	fwu_client *create_fwu_client(void);

	struct rpc_service_interface *get_service_interface(void);

private:
	/* Maximum locations supported */
	static const unsigned int MAX_LOCATIONS = 4;

	/* Volumes needed for fwu metadata access */
	static const unsigned int FWU_METADATA_VOLUMES = 2;

	/* Boot index on first boot before valid FWU metadata exists */
	static const unsigned int FIRST_BOOT_BANK_INDEX = 0;

	/* Platform storage configuration */
	static const size_t FLASH_BLOCK_SIZE = 512;
	static const size_t FW_VOLUME_NUM_BLOCKS = 20;
	static const size_t METADATA_VOLUME_NUM_BLOCKS = 4;

	void fw_partition_guid(unsigned int location_index, unsigned int bank_index,
			       struct uuid_octets *uuid) const;

	void fwu_metadata_partition_guid(bool is_primary, struct uuid_octets *uuid) const;

	void disk_guid(struct uuid_octets *uuid) const;

	void construct_storage(unsigned int num_locations);
	void destroy_storage(void);

	void construct_fw_volumes(unsigned int num_locations);
	void destroy_fw_volumes(void);

	void construct_installers(unsigned int num_locations, bool allow_partial_updates);
	void destroy_installers(void);

	void install_factory_images(unsigned int num_locations);
	void verify_boot_images(unsigned int boot_index);

	static void verify_image(struct volume *volume);

	const struct metadata_serializer *select_metadata_serializer(void) const;

	bool m_is_booted;
	bool m_is_first_boot;
	struct boot_info m_boot_info;
	metadata_checker *m_metadata_checker;
	unsigned int m_num_locations;
	struct rpc_service_interface *m_service_iface;

	/* Firmware storage */
	struct ram_block_store m_fw_flash;
	struct partitioned_block_store m_partitioned_block_store;
	struct block_store *m_block_store;

	/* Pools of volume objects */
	size_t m_fw_volume_used_count;
	struct block_volume
		m_fw_volume_pool[MAX_LOCATIONS * BANK_SCHEME_NUM_BANKS + FWU_METADATA_VOLUMES];

	/* Pools of different types of installer */
	size_t m_raw_installer_used_count;
	struct raw_installer m_raw_installer_pool[MAX_LOCATIONS];
	size_t m_copy_installer_used_count;
	struct copy_installer m_copy_installer_pool[MAX_LOCATIONS];

	/* The core fwu service components */
	struct update_agent *m_update_agent;
	struct fw_store m_fw_store;
	struct fwu_provider m_fwu_provider;
};

#endif /* SIM_FWU_DUT_H */
