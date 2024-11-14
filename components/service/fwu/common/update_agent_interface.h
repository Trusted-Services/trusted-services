/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UPDATE_AGENT_INTERFACE_H
#define UPDATE_AGENT_INTERFACE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "common/uuid/uuid.h"
#include "protocols/service/fwu/status.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FWU_OP_TYPE_READ	(0)
#define FWU_OP_TYPE_WRITE	(1)

#define FWU_FLAG_PARTIAL_UPDATE	(0x01)

struct fwu_discovery_result {
	int16_t service_status;
	uint8_t version_major;
	uint8_t version_minor;
	uint64_t max_payload_size;
	uint32_t flags;
	uint32_t vendor_specific_flags;
};

struct update_agent_interface {
	/**
	 * \brief Discovery
	 *
	 * \param[in]  context         The update_agent context
	 * \param[out] result          Discovery result structure
	 *
	 * \return 0 on success
	 */
	int (*discover)(void *context, struct fwu_discovery_result *result);

	/**
	 * \brief Begin staging
	 *
	 * \param[in]  context         The update_agent context
	 * \param[in]  vendor_flags    Vendor specific staging flags
	 * \param[in]  partial_update_count     Number of update_guid elements
	 * \param[in]  update_guid     Images to update
	 *
	 *
	 * \return 0 on successfully transitioning to the STAGING state
	 */
	int (*begin_staging)(void *context, uint32_t vendor_flags, uint32_t partial_update_count,
			     const struct uuid_octets *update_guid);

	/**
	 * \brief End staging
	 *
	 * \param[in]  context         The update_agent context
	 *
	 * \return 0 on successfully transitioning to the TRIAL state
	 */
	int (*end_staging)(void *context);

	/**
	 * \brief Cancel staging
	 *
	 * \param[in]  context         The update_agent context
	 *
	 * \return 0 on successfully transitioning to the REGULAR state
	 */
	int (*cancel_staging)(void *context);

	/**
	 * \brief Open a stream for accessing an fwu stream
	 *
	 * Used for reading or writing data for accessing images or other fwu
	 * related objects.
	 *
	 * \param[in]  context         The update_agent context
	 * \param[in]  uuid            Identifies the object to access
	 * \param[in]  op_type         Read/write operation, use FWU_OP_TYPE_* macros
	 * \param[out] handle          For subsequent read/write operations
	 *
	 * \return 0 on success
	 */
	int (*open)(void *context, const struct uuid_octets *uuid, uint8_t op_type,
		    uint32_t *handle);

	/**
	 * \brief Write to a previously opened stream
	 *
	 * \param[in]  context         The update_agent context
	 * \param[in]  handle          The handle returned by open
	 * \param[in]  data            Pointer to data
	 * \param[in]  data_len        The data length
	 *
	 * \return 0 on success
	 */
	int (*write_stream)(void *context, uint32_t handle, const uint8_t *data, size_t data_len);

	/**
	 * \brief Read from a previously opened stream
	 *
	 * \param[in]  context         The update_agent context
	 * \param[in]  handle          The handle returned by open
	 * \param[in]  buf             Pointer to buffer to copy to
	 * \param[in]  buf_size        The size of the buffer
	 * \param[out] read_len        The length of data read
	 * \param[out] total_len       The total length of the object to read
	 *
	 * \return 0 on success
	 */
	int (*read_stream)(void *context, uint32_t handle, uint8_t *buf, size_t buf_size,
			   size_t *read_len, size_t *total_len);

	/**
	 * \brief Close a stream and commit any writes to the stream
	 *
	 * \param[in]  context         The update_agent context
	 * \param[in]  handle          The handle returned by open
	 * \param[in]  accepted        Initial accepted state of an image
	 * \param[in]  max_atomic_len  Hint, maximum time (in ns) that the Update Agent can execute
	 *                             continuously without yielding back to the Client. A value of
	 *                             0 means that the Update Agent can execute for an unbounded
	 *                             time.
	 * \param[in]  progress        Units of work already completed by the Update Agent
	 * \param[in]  total_work      Units of work the Update Agent must perform until fwu_commit
	 *                             returns successfully
	 *
	 * \return 0 on success
	 */
	int (*commit)(void *context, uint32_t handle, bool accepted, uint32_t max_atomic_len, uint32_t *progress,
		      uint32_t *total_work);


	/**
	 * \brief Accept an updated image
	 *
	 * \param[in]  context         The update_agent context
	 * \param[in]  image_type_uuid Identifies the image to accept
	 *
	 * \return 0 on success
	 */
	int (*accept_image)(void *context, const struct uuid_octets *image_type_uuid);

	/**
	 * \brief Select previous version
	 *
	 *  Revert to a previous good version (if possible).
	 *
	 * \param[in]  context         The update_agent context
	 *
	 * \return 0 on success
	 */
	int (*select_previous)(void *context);
};

/**
 * \brief Common update agent instance
 *
 * Used by the FWU provider make an association with an agent.
 */
struct update_agent {
	/**
	 * \brief The update_agent context
	 *
	 * Points to backend specific instance data.
	 */
	void *context;

	/**
	 * \brief The update agent interface
	 *
	 * A concrete agent provides an implementation of this interface.
	 */
	const struct update_agent_interface *interface;
};

int update_agent_discover(struct update_agent *update_agent, struct fwu_discovery_result *result);

int update_agent_begin_staging(struct update_agent *update_agent, uint32_t vendor_flags,
			       uint32_t partial_update_count,
			       const struct uuid_octets *update_guid);

int update_agent_end_staging(struct update_agent *update_agent);

int update_agent_cancel_staging(struct update_agent *update_agent);

int update_agent_open(struct update_agent *update_agent, const struct uuid_octets *uuid,
		      uint8_t op_type, uint32_t *handle);

int update_agent_write_stream(struct update_agent *update_agent, uint32_t handle,
			      const uint8_t *data, size_t data_len);

int update_agent_read_stream(struct update_agent *update_agent, uint32_t handle, uint8_t *buf,
			     size_t buf_size, size_t *read_len, size_t *total_len);

int update_agent_commit(struct update_agent *update_agent, uint32_t handle, bool accepted,
			uint32_t max_atomic_len, uint32_t *progress, uint32_t *total_work);

int update_agent_accept_image(struct update_agent *update_agent,
			      const struct uuid_octets *image_type_uuid);

int update_agent_select_previous(struct update_agent *update_agent);

#ifdef __cplusplus
}
#endif

#endif /* UPDATE_AGENT_INTERFACE_H */
