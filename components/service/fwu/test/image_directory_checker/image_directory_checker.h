/*
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef IMAGE_DIRECTORY_CHECKER_H
#define IMAGE_DIRECTORY_CHECKER_H

#include <cstddef>
#include <cstdint>

#include "common/uuid/uuid.h"
#include "protocols/service/fwu/fwu_proto.h"
#include "service/fwu/test/fwu_client/fwu_client.h"

/*
 * Provides check methods for checking the contents of the image
 * directory fetched from the update agent.
 */
class image_directory_checker {
public:
	image_directory_checker();
	~image_directory_checker();

	int fetch_image_directory(fwu_client *fwu_client);

	size_t num_images(void) const;

	bool is_contents_equal(const image_directory_checker &rhs) const;

	const struct fwu_image_directory *get_header(void) const;
	const struct fwu_image_info_entry *
	find_entry(const struct uuid_octets *img_type_uuid) const;

private:
	static const size_t MAX_IMAGES = 50;

	void alloc_buffer(void);

	uint8_t *m_buf;
	size_t m_buf_size;
	size_t m_total_read_len;
};

#endif /* IMAGE_DIRECTORY_CHECKER_H */
