/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ENTROPY_H
#define ENTROPY_H

/*
 * This macro is only used for baremetal testing. It is set to a random value,
 * which is unrelated to mbedtls to avoid the need of keeping it syncronized.
 */
#define MBEDTLS_ERR_ENTROPY_SOURCE_FAILED (0xFFFF)

#endif /* ENTROPY_H */
