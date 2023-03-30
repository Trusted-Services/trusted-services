/*
 * Copyright (c) 2022-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef METADATA_CHECKER_V1_H
#define METADATA_CHECKER_V1_H

#include "metadata_checker.h"

/*
 * A metadata_checker for FWU-A V1 metadata
 */
class metadata_checker_v1 : public metadata_checker {
public:
	metadata_checker_v1(metadata_fetcher *metadata_fetcher, unsigned int num_images);

	virtual ~metadata_checker_v1();

	void get_active_indices(uint32_t *active_index, uint32_t *previous_active_index) override;

	void check_regular(unsigned int boot_index) override;
	void check_ready_for_staging(unsigned int boot_index) override;
	void check_ready_to_activate(unsigned int boot_index) override;
	void check_trial(unsigned int boot_index) override;
	void check_fallback_to_previous(unsigned int boot_index) override;

private:
	static const size_t MAX_FWU_METADATA_SIZE;

	bool is_all_accepted(unsigned int boot_index) const;

	unsigned int m_num_images;
};

#endif /* METADATA_CHECKER_V1_H */
