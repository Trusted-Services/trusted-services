/*
 * Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SP_HEADER_DEFINES_H
#define SP_HEADER_DEFINES_H

/* To get UUID definition */
#include "env_test.h"

#define OPTEE_SP_UUID             ENV_TEST_SP_UUID
#define OPTEE_SP_FLAGS				0

/* Provisioned stack size */
#define OPTEE_SP_STACK_SIZE			(64 * 1024)

/* Provisioned heap size */
#define OPTEE_SP_HEAP_SIZE			(32 * 1024)

#endif /* SP_HEADER_DEFINES_H */
