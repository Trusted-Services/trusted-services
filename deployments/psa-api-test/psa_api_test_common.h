/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef COMMON_H
#define COMMON_H

#include "psa/error.h"

psa_status_t test_setup(const char *service_name_crypto, const char *service_name_iat,
			const char *service_name_ps, const char *service_name_its);
void test_teardown(void);

#endif /* COMMON_H */
