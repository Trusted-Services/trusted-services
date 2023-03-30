/*
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DIRECT_FWU_CLIENT_H
#define DIRECT_FWU_CLIENT_H

#include "service/fwu/test/fwu_client/fwu_client.h"

/* Public interface dependencies */
struct update_agent;

/*
 * An fwu_client that interfaces directly with an update_agent. Can be
 * used for component level testing where tests and update_agent are
 * combined in the same build.
 */
class direct_fwu_client : public fwu_client {
public:
	explicit direct_fwu_client(struct update_agent *update_agent);
	~direct_fwu_client();

	int begin_staging(void);

	int end_staging(void);

	int cancel_staging(void);

	int accept(const struct uuid_octets *image_type_uuid);

	int select_previous(void);

	int open(const struct uuid_octets *uuid, uint32_t *handle);

	int commit(uint32_t handle, bool accepted);

	int write_stream(uint32_t handle, const uint8_t *data, size_t data_len);

	int read_stream(uint32_t handle, uint8_t *buf, size_t buf_size, size_t *read_len,
			size_t *total_len);

private:
	static const size_t READ_BUF_SIZE = 512;

	struct update_agent *m_update_agent;
	uint8_t m_read_buf[READ_BUF_SIZE];
};

#endif /* DIRECT_FWU_CLIENT_H */
