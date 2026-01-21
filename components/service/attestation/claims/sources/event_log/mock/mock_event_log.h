/*
 * Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCK_EVENT_LOG_H
#define MOCK_EVENT_LOG_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Provides the expected values for a measurement */
struct mock_event_log_measurement
{
    const char *id;
    uint8_t digest[32];
};

/* Provides info about an event log. The binary blob, and the interpretation. */
struct mock_event_log_info {
	const uint8_t *data;
	size_t data_length;
	const struct mock_event_log_measurement (*measurements)[];
	size_t n_measurements;
	size_t n_claims;
};

/**
 *  Get the number of event log blobs available.
 */
size_t mock_event_log_list_length(void);

/**
 * Get the descriptor of the nth event log blob.
 */
const struct mock_event_log_info *mock_event_log_list_get_at(size_t index);

/**
 * Returns the default event log descriptor. To be used where a "generic" event log blob is needed.
 */
const struct mock_event_log_info *mock_event_log_list_get_default(void);

/**
 * Returns the expected measurement at the specified index.
 */
const struct mock_event_log_measurement *mock_event_log_measurement(const struct mock_event_log_info *event_log_info, size_t i);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MOCK_EVENT_LOG_H */
