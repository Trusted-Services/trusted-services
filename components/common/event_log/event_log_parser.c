/*
 * Copyright (c) 2026, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "event_log_parser.h"
#include "common/endian/le.h"
#include "common/trace/include/trace.h"
#include "tcg.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

size_t tcg_event2_digest_size(uint16_t algorithm_id)
{
	size_t size = 0;

	switch (algorithm_id) {
	case TPM_ALG_SHA1:
		size = SHA1_DIGEST_SIZE;
		break;
	case TPM_ALG_SHA256:
		size = SHA256_DIGEST_SIZE;
		break;
	case TPM_ALG_SHA384:
		size = SHA384_DIGEST_SIZE;
		break;
	case TPM_ALG_SHA512:
		size = SHA512_DIGEST_SIZE;
		break;
	default:
		break;
	}

	return size;
}

/**
 * Return the length of an TCG EVENT-1 record.
 */
static size_t tcg_event1_record_size(const uint8_t *header, const uint8_t *limit)
{
	if (limit - header < sizeof(tcg_pcr_event_t)) {
		EMSG("Buffer overrun detected while parsing input data.");
		return 0;
	}

	uint32_t event_size = load_u32_le(header, offsetof(tcg_pcr_event_t, event_size));

	const uint8_t *event = header + offsetof(tcg_pcr_event_t, event);

	if (limit - event < sizeof(id_event_struct_header_t)) {
		EMSG("Buffer overrun detected while parsing input data.");
		return 0;
	}

	uint32_t number_of_algorithms = load_u32_le(event,  offsetof(id_event_struct_header_t,
						    number_of_algorithms));
	if (number_of_algorithms < 1) {
		EMSG("Invalid ID record.");
		return 0;
	}

	const uint8_t *digests = event + offsetof(id_event_struct_header_t,  digest_size);

	if (limit - digests < number_of_algorithms * sizeof(id_event_algorithm_size_t)) {
		EMSG("Buffer overrun detected while parsing input data.");
		return 0;
	}

	const uint8_t *vendor = digests + number_of_algorithms * sizeof(id_event_algorithm_size_t);

	if (limit - vendor < sizeof(id_event_struct_data_t)) {
		EMSG("Buffer overrun detected while parsing input data.");
		return 0;
	}

	uint8_t vendor_info_size = load_u8_le(vendor, 0);

	const uint8_t *end = vendor + sizeof(id_event_struct_data_t);

	if (limit - end < vendor_info_size) {
		EMSG("Buffer overrun detected while parsing input data.");
		return 0;
	}

	end += vendor_info_size;

	if (event_size != end - event) {
		EMSG("Invalid event size.");
		return 0;
	}

	return end - header;
}

/**
 *  Return the length of a TCG EVENT-2 (variable length) record . Returns zero if there's a problem.
 */
static size_t tcg_event2_record_size(const uint8_t *header, const uint8_t *limit)
{
	uint32_t digest_count = 0;
	const uint8_t *pos = header;

	// Sanity check.
	if (header > limit) {
		DMSG("Invalid arguments");
		return 0;
	}

	/* Ensure that the header is within the limit of the event log */
	if ((limit - header < sizeof(event2_header_t))) {
		EMSG("Buffer overrun detected while parsing input data.");
		return 0;
	}

	digest_count = load_u32_le(pos, offsetof(event2_header_t, digests.count));
	pos += sizeof(event2_header_t);

	if (digest_count == 0) {
		EMSG("Invalid event2 record.");
		return 0;
	}

	/* Add the variable length space used for digests */
	for (unsigned int i = 0; i < digest_count; ++i) {
		// Ensure buffer is big enough to hold next algorithm id
		if (limit - pos < 2) {
			EMSG("Buffer overrun detected while parsing input data.");
			return 0;
		}

		uint16_t algorithm_id =	load_u16_le(pos, 0);
		size_t digest_size = tcg_event2_digest_size(algorithm_id);

		// Look for overflow of pos.
		if (limit - pos < 2 + digest_size) {
			EMSG("Buffer overrun detected while parsing input data.");
			return 0;
		}

		if (digest_size == 0) {
			EMSG("Unsupported digest type %i.", algorithm_id);
			return 0;
		}

		pos += 2 + digest_size;
	}

	if (limit - pos < 4) {
		EMSG("Buffer overrun detected while parsing input data.");
		return 0;
	}

	uint32_t event_size = load_u32_le(pos, 0);

	pos += 4;

	if (limit - pos < event_size) {
		EMSG("Buffer overrun detected while parsing input data.");
		return 0;
	}

	pos += event_size;

	return pos - header;
}

bool event_log_parser_is_done(struct evl_context *context)
{
	return (context->pos >= context->end) || (context->pos < context->begin);
}

bool event_log_parser_init(struct evl_context *context, const uint8_t *event_log,
			   size_t event_log_length)
{
	struct evl_record_data data;

	context->begin = event_log;
	context->end = event_log + event_log_length;
	context->pos = context->begin;
	context->uintn_size = uintns_unknown;

	if (event_log_parser_get(context, &data)) {
		EMSG("Failed to read event log specification ID record.");
		goto error;
	}
	if (data.data_type != rct_id_record) {
		EMSG("The fist event log entry is not an ID record.");
		goto error;
	}

	if (data.data.id_record.version_major != 2 ||
	    data.data.id_record.version_minor != 0 ||
	    data.data.id_record.spec_revision != 2) {
		EMSG("Unsupported event log version.");
		goto error;
	}

	context->uintn_size = data.data.id_record.uintn_is_64_bit ? uintns_64 : uintns_32;

	return false;

error:
	context->pos = context->end;
	return true;
}

bool event_log_parser_next(struct evl_context *context)
{
	size_t record_len;

	if (context->pos == context->begin) {
		/* The first record must be in TCG EVENT-1 format */
		record_len = tcg_event1_record_size(context->pos, context->end);

		if (record_len == 0)
			goto error;

		context->pos += record_len;
	} else {
		/* All subsequent records are assumed to be in variable
		 * length TCG_PCR_EVENT2 format.
		 */
		record_len = tcg_event2_record_size(context->pos, context->end);
		if (record_len == 0)
			goto error;

		context->pos += record_len;
	}

	return !event_log_parser_is_done(context);

error:
	context->pos = context->end;
	return false;
}

static bool is_buffer_zero_filled(const uint8_t *buffer, size_t buf_len)
{
	const uint8_t *end = buffer + buf_len - 1;

	while (end >= buffer) {
		if (*end-- != 0)
			return false;
	}
	return true;
}

static bool unpack_version_id_record(struct evl_context *context, struct evl_spec_id_record *data)
{
	uint32_t record_len = tcg_event1_record_size(context->pos, context->end);

	if (record_len == 0)
		return true;

	uint32_t pcr_index = load_u32_le(context->pos, offsetof(tcg_pcr_event_t, pcr_index));
	uint32_t event_type = load_u32_le(context->pos, offsetof(tcg_pcr_event_t, event_type));

	if (pcr_index != 0 || event_type != EV_NO_ACTION) {
		EMSG("Invalid ID record.");
		return true;
	}

	if (!is_buffer_zero_filled(context->pos + offsetof(tcg_pcr_event_t, digest),
				   sizeof(((tcg_pcr_event_t *)0)->digest))) {
		EMSG("Invalid ID record.");
		return true;
	}

	const uint8_t *event = context->pos + offsetof(tcg_pcr_event_t, event);

	if (memcmp(TCG_ID_EVENT_SIGNATURE_03, event, 16) != 0) {
		EMSG("Invalid ID record.");
		return true;
	}

	uint8_t uintn_size = load_u8_le(event,  offsetof(id_event_struct_header_t,  uintn_size));

	if (uintn_size != 1 && uintn_size != 2) {
		EMSG("Invalid ID record.");
		return true;
	}

	uint32_t number_of_algorithms = load_u32_le(event,  offsetof(id_event_struct_header_t,
						    number_of_algorithms));

	const uint8_t *digests = event + offsetof(id_event_struct_header_t,  digest_size);

	 const uint8_t *vendor_info = digests +
					(number_of_algorithms * sizeof(id_event_algorithm_size_t));

	uint8_t vendor_info_size = load_u8_le(vendor_info, 0);

	vendor_info += 1;

	memset(data, 0, sizeof(*data));

	data->platform_class = load_u32_le(event, offsetof(id_event_struct_header_t,
							   platform_class));
	data->version_major = load_u8_le(event,  offsetof(id_event_struct_header_t,
							  spec_version_major));
	data->version_minor = load_u8_le(event,  offsetof(id_event_struct_header_t,
							  spec_version_minor));
	data->spec_revision = load_u8_le(event,  offsetof(id_event_struct_header_t, spec_errata));
	data->uintn_is_64_bit = uintn_size == 2 ? 1 : 0;

	data->number_of_algorithms = number_of_algorithms;

	for (unsigned int x = 0; x < data->number_of_algorithms; x++) {
		data->digest_size_list[x].algorithm_id =
			load_u16_le(digests, offsetof(id_event_algorithm_size_t, algorithm_id));
		data->digest_size_list[x].digest_size =
			load_u16_le(digests, offsetof(id_event_algorithm_size_t, digest_size));
		digests += 4;

		if (data->digest_size_list[x].digest_size !=
				tcg_event2_digest_size(data->digest_size_list[x].algorithm_id)) {
			EMSG("Invalid ID record.");
			return true;
		}
	}

	data->vendor_info_size = vendor_info_size;
	data->vendor_info = vendor_info_size ? vendor_info : NULL;

	return false;
}

static size_t unpack_tpmt_ha(const uint8_t *start, const uint8_t *limit,
			     struct evl_digest_record *record)
{
	record->algorithm_id = load_u16_le(start, 0);
	start += 2;

	record->digest_size = tcg_event2_digest_size(record->algorithm_id);
	record->digest = start;

	return 2 + record->digest_size;
}

bool unpack_event2_record(struct evl_context *context, struct evl_event2_record *data)
{
	size_t len = tcg_event2_record_size(context->pos, context->end);

	if (len == 0)
		return true;

	memset(data, 0, sizeof(*data));

	data->header.pcr_index = load_u32_le(context->pos, offsetof(event2_header_t, pcr_index));
	data->header.event_type = load_u32_le(context->pos, offsetof(event2_header_t, event_type));

	data->header.n_digests = load_u32_le(context->pos, offsetof(event2_header_t,
								    digests.count));

	const uint8_t *digests = context->pos + offsetof(event2_header_t,  digests.digests);

	for (unsigned int x = 0; x < data->header.n_digests; x++) {
		len = unpack_tpmt_ha(digests, context->end, &data->header.digests[x]);
		if (len == 0)
			return true;

		digests += len;
	}

	data->data_size = load_u32_le(digests, 0);
	digests += 4;
	if (data->data_size > 0) {
		if (context->end - digests < data->data_size) {
			EMSG("Buffer too small.");
			return true;
		}
		data->event_data = digests;
	} else {
		data->event_data = NULL;
	}
	return false;
}

bool event_log_parser_get(struct evl_context *context, struct evl_record_data *data)
{
	bool rv = true;

	if (context->pos == context->begin) {
		/* The first record must be in TCG EVENT-1 format */
		data->data_type = rct_id_record;
		rv = unpack_version_id_record(context, &data->data.id_record);
	} else {
		data->data_type = rct_pcr_record;
		rv = unpack_event2_record(context, &data->data.pcr_record);
	}
	return rv;
}

bool event_log_parser_get_measurement_id(struct evl_record_data *data, const char **id)
{
	if (data->data_type != rct_pcr_record ||
	    data->data.pcr_record.header.event_type != EV_POST_CODE)
		return true;

	if (!memchr(data->data.pcr_record.event_data, 0, data->data.pcr_record.data_size)) {
		EMSG("Event ID string not 0 terminated.");
		return true;
	}

	*id = (char *)data->data.pcr_record.event_data;
	return false;
}

bool event_log_parser_get_measurement_digest(struct evl_record_data *data, uint8_t algorithm_id,
					     const uint8_t **digest, size_t *digest_size)
{
	if (data->data_type != rct_pcr_record ||
	    data->data.pcr_record.header.event_type != EV_POST_CODE)
		return true;

	struct evl_event2_record *record = &data->data.pcr_record;

	for (int x = 0; x < record->header.n_digests; x++) {
		if (record->header.digests[x].algorithm_id == algorithm_id) {
			*digest = record->header.digests[x].digest;
			*digest_size = tcg_event2_digest_size(algorithm_id);
			return false;
		}
	}

	return true;
}
