/*
 * Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef EVENT_LOG_CLAIM_SOURCE_H
#define EVENT_LOG_CLAIM_SOURCE_H

#include "service/attestation/claims/claim_source.h"

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A claim source for accessing a TCG event log, stored in a buffer.
 * Use this claim source in deployments where the bootloader has peformed
 * boot measurements and recorded them in an event log.  Additional
 * claims such as the boot seed may also be recorded in the event log.
 */
struct event_log_claim_source {
	struct claim_source base;

	const uint8_t *event_log;
	size_t event_log_len;
};

/**
 * \brief Initializes a event_log_claim_source from buffer.
 *
 * Initializes an event_log_claim_source, taking the provided buffer
 * containing the TCG event log data.
 *
 * \param[in] instance      The event_log_claim_source instance to initialize
 * \param[in] event_log     Pointer to the event log.
 * \param[in] event_log_len Byte length of the event log
 *
 * \return The initialize base claim_source structure
 */
struct claim_source *event_log_claim_source_init(struct event_log_claim_source *instance,
						 const uint8_t *event_log, size_t event_log_len);

/**
 * \brief Initializes a event_log_claim_source from config store
 *
 * Initializes an event_log_claim_source using an event log configuration object
 * obtained from the config store.
 *
 * \param[in] instance      The event_log_claim_source instance to initialize
 *
 * \return The initialize base claim_source structure
 */
struct claim_source
*event_log_claim_source_init_from_config(struct event_log_claim_source *instance);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* EVENT_LOG_CLAIM_SOURCE_H */
