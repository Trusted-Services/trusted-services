/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2023-2026, Arm Limited and Contributors. All rights reserved.
 */

#ifndef LOG_FACTORY_H
#define LOG_FACTORY_H

#include <stdbool.h>

#include "components/service/log/backend/log_backend.h"
#include "components/service/log/common/log_status.h"

#ifdef __cplusplus
extern "C" {
#endif

struct logger;

void log_factory_create(void);
struct log_backend *log_factory_get_backend_instance(void);
void log_factory_destroy(struct logger *backend);

#ifdef __cplusplus
}
#endif

#endif /* LOG_FACTORY_H */
