/*
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fwu_app.h"

#include <cassert>
#include <cstddef>
#include <cstring>
#include <errno.h>

#include "media/volume/factory/volume_factory.h"
#include "metadata_reader.h"
#include "service/block_storage/factory/file/block_store_factory.h"
#include "service/fwu/agent/update_agent.h"
#include "service/fwu/config/fwu_configure.h"
#include "service/fwu/fw_store/banked/bank_scheme.h"
#include "service/fwu/fw_store/banked/banked_fw_store.h"
#include "service/fwu/fw_store/banked/metadata_serializer/v1/metadata_serializer_v1.h"
#include "service/fwu/fw_store/banked/metadata_serializer/v2/metadata_serializer_v2.h"
#include "service/fwu/inspector/direct/direct_fw_inspector.h"
#include "service/fwu/installer/factory/installer_factory.h"

extern "C" {
#include "trace.h"
}

fwu_app::fwu_app()
	: m_update_agent()
	, m_fw_store()
{
	memset(&m_fw_store, 0, sizeof(m_fw_store));
}

fwu_app::~fwu_app()
{
	update_agent_deinit(m_update_agent);
	banked_fw_store_deinit(&m_fw_store);

	fwu_deconfigure();
	volume_factory_deinit();
}

int fwu_app::configure(const char *disk_img_filename)
{
	if (disk_img_filename)
		file_block_store_factory_set_filename(disk_img_filename);

	struct uuid_octets device_uuids[MAX_STORAGE_DEVICES];
	size_t num_storage_devices = 0;

	int status = volume_factory_init(device_uuids, MAX_STORAGE_DEVICES, &num_storage_devices);

	if (status) {
		EMSG("Failed to init volume factory: %d", status);
		return -EIO;
	}

	status = fwu_configure(device_uuids, num_storage_devices);

	if (status) {
		EMSG("Failed to setup FWU configuration: %d", status);
		return -EIO;
	}

	return 0;
}

int fwu_app::get_boot_info(unsigned int &active_index, unsigned int &metadata_version)
{
	return metadata_reader::instance()->get_boot_info(active_index, metadata_version);
}

int fwu_app::init_update_agent(unsigned int boot_index, unsigned int metadata_version)
{
	if (boot_index >= BANK_SCHEME_NUM_BANKS) {
		IMSG("Invalid boot index");
		return -1;
	}

	const struct metadata_serializer *serializer = select_metadata_serializer(metadata_version);

	if (!serializer) {
		IMSG("Unsupported FWU metadata version");
		return -1;
	}

	/* Initialise update_agent */
	int status = banked_fw_store_init(&m_fw_store, serializer);

	if (status) {
		IMSG("fw store initialisation error %d", status);
		return -1;
	}

	m_update_agent = update_agent_init(boot_index, direct_fw_inspector_inspect, &m_fw_store);

	if (!m_update_agent) {
		IMSG("update agent initialisation error %d", status);
		return -1;
	}

	/* Success */
	return 0;
}

int fwu_app::update_image(const struct uuid_octets &img_type_uuid, const uint8_t *img_data,
			  size_t img_size)
{
	int status = update_agent_begin_staging(m_update_agent, 0, 0, NULL);

	if (status)
		return status;

	uint32_t stream_handle = 0;
	uint32_t progress = 0;
	uint32_t total_work = 0;

	status = update_agent_open(m_update_agent, &img_type_uuid, FWU_OP_TYPE_WRITE,
				   &stream_handle);

	if (!status) {
		status = update_agent_write_stream(m_update_agent, stream_handle, img_data,
						   img_size);

		if (!status)
			status = update_agent_commit(m_update_agent, stream_handle, false, 0,
						     &progress, &total_work);
	}

	if (!status)
		status = update_agent_end_staging(m_update_agent);
	else
		update_agent_cancel_staging(m_update_agent);

	return status;
}

int fwu_app::read_object(const struct uuid_octets &object_uuid, std::vector<uint8_t> &data)
{
	uint32_t stream_handle = 0;
	int status = update_agent_open(m_update_agent, &object_uuid, FWU_OP_TYPE_READ,
				       &stream_handle);

	if (status)
		return status;

	/* Don't yet know how big the object will be so allocate some space to get
	 * started with the initial read.
	 */
	size_t reported_total_len = 0;
	size_t read_so_far = 0;
	size_t vector_capacity = 512;
	uint32_t progress = 0;
	uint32_t total_work = 0;

	data.resize(vector_capacity);

	do {
		size_t data_len_read = 0;
		size_t requested_read_len = vector_capacity - read_so_far;

		status = update_agent_read_stream(m_update_agent, stream_handle,
						  &data[read_so_far], requested_read_len,
						  &data_len_read, &reported_total_len);

		read_so_far += data_len_read;
		data.resize(read_so_far);

		if (reported_total_len > vector_capacity) {
			vector_capacity = reported_total_len;
			data.resize(vector_capacity);
		}

		assert(read_so_far <= reported_total_len);

		if (read_so_far == reported_total_len) {
			/* Read all the data */
			if (vector_capacity > reported_total_len)
				data.resize(reported_total_len);

			break;
		}

	} while (!status);

	status = update_agent_commit(m_update_agent, stream_handle, false, 0, &progress,
				     &total_work);

	return status;
}

struct update_agent *fwu_app::update_agent()
{
	return m_update_agent;
}

const struct metadata_serializer *fwu_app::select_metadata_serializer(unsigned int version)
{
	if (version == 1)
		return metadata_serializer_v1();

	if (version == 2)
		return metadata_serializer_v2();

	return NULL;
}
