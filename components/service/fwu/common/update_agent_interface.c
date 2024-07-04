/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "update_agent_interface.h"
#include "protocols/service/fwu/status.h"

int update_agent_discover(struct update_agent *update_agent, struct fwu_discovery_result *result)
{
	if (!update_agent->interface->discover)
		return FWU_STATUS_NOT_AVAILABLE;

	return update_agent->interface->discover(update_agent->context, result);
}

int update_agent_begin_staging(struct update_agent *update_agent, uint32_t vendor_flags,
			       uint32_t partial_update_count,
			       const struct uuid_octets *update_guid)
{
	if (!update_agent->interface->begin_staging)
		return FWU_STATUS_NOT_AVAILABLE;

	return update_agent->interface->begin_staging(update_agent->context, vendor_flags,
						      partial_update_count, update_guid);
}

int update_agent_end_staging(struct update_agent *update_agent)
{
	if (!update_agent->interface->end_staging)
		return FWU_STATUS_NOT_AVAILABLE;

	return update_agent->interface->end_staging(update_agent->context);
}

int update_agent_cancel_staging(struct update_agent *update_agent)
{
	if (!update_agent->interface->cancel_staging)
		return FWU_STATUS_NOT_AVAILABLE;

	return update_agent->interface->cancel_staging(update_agent->context);
}

int update_agent_open(struct update_agent *update_agent, const struct uuid_octets *uuid,
		      uint8_t op_type, uint32_t *handle)
{
	if (!update_agent->interface->open)
		return FWU_STATUS_NOT_AVAILABLE;

	return update_agent->interface->open(update_agent->context, uuid, op_type, handle);
}

int update_agent_write_stream(struct update_agent *update_agent, uint32_t handle,
			      const uint8_t *data, size_t data_len)
{
	if (!update_agent->interface->write_stream)
		return FWU_STATUS_NOT_AVAILABLE;

	return update_agent->interface->write_stream(update_agent->context, handle, data, data_len);
}

int update_agent_read_stream(struct update_agent *update_agent, uint32_t handle, uint8_t *buf,
			     size_t buf_size, size_t *read_len, size_t *total_len)
{
	if (!update_agent->interface->read_stream)
		return FWU_STATUS_NOT_AVAILABLE;

	return update_agent->interface->read_stream(update_agent->context, handle, buf, buf_size,
						    read_len, total_len);
}

int update_agent_commit(struct update_agent *update_agent, uint32_t handle, bool accepted,
			uint32_t max_atomic_len, uint32_t *progress, uint32_t *total_work)
{
	if (!update_agent->interface->commit)
		return FWU_STATUS_NOT_AVAILABLE;

	return update_agent->interface->commit(update_agent->context, handle, accepted,
					       max_atomic_len, progress, total_work);
}

int update_agent_accept_image(struct update_agent *update_agent,
			      const struct uuid_octets *image_type_uuid)
{
	if (!update_agent->interface->accept_image)
		return FWU_STATUS_NOT_AVAILABLE;

	return update_agent->interface->accept_image(update_agent->context, image_type_uuid);
}

int update_agent_select_previous(struct update_agent *update_agent)
{
	if (!update_agent->interface->select_previous)
		return FWU_STATUS_NOT_AVAILABLE;

	return update_agent->interface->select_previous(update_agent->context);
}
