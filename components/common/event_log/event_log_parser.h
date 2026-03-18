/*
 * Copyright (c) 2026, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _EVENT_LOG_PARSER_H_
#define _EVENT_LOG_PARSER_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_PCR_BANKS
#define MAX_PCR_BANKS	8
#endif

enum uintn_size {
	uintns_unknown = 0,
	uintns_32,
	uintns_64
};

struct evl_context {
	const uint8_t *begin;
	const uint8_t *end;
	const uint8_t *pos;
	enum uintn_size uintn_size;
};

struct evl_digest_record {
	uint16_t algorithm_id;
	size_t digest_size;
	const uint8_t *digest;
};

struct evl_event2_header {
	uint32_t pcr_index;
	uint32_t event_type;
	uint32_t n_digests;
	struct evl_digest_record digests[MAX_PCR_BANKS];
};

struct evl_event2_record {
	struct evl_event2_header header;
	uint32_t data_size;
	const uint8_t *event_data;
};

struct evl_spec_id_record {
	uint32_t platform_class;
	uint8_t version_major;
	uint8_t version_minor;
	uint8_t spec_revision;
	uint8_t uintn_is_64_bit;
	uint8_t number_of_algorithms;
	struct {
		uint16_t algorithm_id;
		uint16_t digest_size;
	} digest_size_list[MAX_PCR_BANKS];
	uint8_t vendor_info_size;
	const uint8_t *vendor_info;
};

enum evl_record_type {
	rct_id_record,
	rct_pcr_record,
	rct_skip
};

struct evl_record_data {
	enum evl_record_type data_type;
	union {
		struct evl_spec_id_record id_record;
		struct evl_event2_record pcr_record;
	} data;
};

size_t tcg_event2_digest_size(uint16_t algorithm_id);

bool event_log_parser_init(struct evl_context *context, const uint8_t *event_log,
			   size_t event_log_length);

bool event_log_parser_next(struct evl_context *context);
bool event_log_parser_is_done(struct evl_context *context);
bool event_log_parser_get(struct evl_context *context, struct evl_record_data *data);

bool event_log_parser_get_measurement_id(struct evl_record_data *data, const char **id);
bool event_log_parser_get_measurement_digest(struct evl_record_data *data, uint8_t algorithm_id,
					     const uint8_t **digest, size_t *digest_size);

#ifdef __cplusplus
}
#endif

#endif
