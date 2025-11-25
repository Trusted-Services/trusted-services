/*
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <string.h>
#include <common/endian/le.h>
#include <config/interface/config_store.h>
#include <config/interface/config_blob.h>
#include "event_log_claim_source.h"
#include "tcg.h"

static bool event_log_claim_source_get_claim(void *context, struct claim *claim);
static void create_event_log_iterator(const struct claim_collection_variant *variant,
								struct claim_iterator *iter);

static void event_log_iterator_first(struct claim_iterator *iter);
static bool event_log_iterator_next(struct claim_iterator *iter);
static bool event_log_iterator_is_done(struct claim_iterator *iter);
static bool event_log_iterator_current(struct claim_iterator *iter, struct claim *claim);

static size_t tcg_event2_digest_size(uint16_t algorithm_id);
static size_t tcg_event2_header_size(const void *header, const void *limit);
static size_t tcg_event1_record_size(const void *header, const void *limit);
static void tcg_event2_extract_digest(const void *header,
									struct measurement_claim_variant *measurement);
static void tcg_event2_extract_measurement_id(const void *event_data,
									struct measurement_claim_variant *measurement,
									const void *limit);



struct claim_source *event_log_claim_source_init(struct event_log_claim_source *instance,
	const uint8_t *event_log, size_t event_log_len)
{
	instance->base.get_claim = event_log_claim_source_get_claim;
	instance->base.context = instance;

	instance->event_log = event_log;
	instance->event_log_len = event_log_len;

	return &instance->base;
}

struct claim_source *event_log_claim_source_init_from_config(
	struct event_log_claim_source *instance)
{
	struct claim_source *claim_source = NULL;
	struct config_blob config_blob;

	if (config_store_query(CONFIG_CLASSIFIER_BLOB,
		"EVENT_LOG", 0,
		&config_blob, sizeof(config_blob))) {

		claim_source = event_log_claim_source_init(instance,
			config_blob.data, config_blob.data_len);
	}

	return claim_source;
}

static bool event_log_claim_source_get_claim(void *context, struct claim *claim)
{
	bool is_available = false;
	struct event_log_claim_source *instance = (struct event_log_claim_source*)context;

	/* The claim returned from a event_log_claim_source is always a claim collection,
	 * realized by the associated event log.  The event log may contain 0..*
	 * claims.
	 */
	if (instance->event_log && instance->event_log_len) {

		claim->subject_id = CLAIM_SUBJECT_ID_NONE;
		claim->variant_id = CLAIM_VARIANT_ID_COLLECTION;
		claim->raw_data = instance->event_log;

		claim->variant.collection.create_iterator = create_event_log_iterator;
		claim->variant.collection.begin_pos = instance->event_log;
		claim->variant.collection.end_pos = &instance->event_log[instance->event_log_len];

		is_available = true;
	}

	return is_available;
}

static void create_event_log_iterator(const struct claim_collection_variant *variant,
								struct claim_iterator *iter)
{
	/* Assign concrete methods */
	iter->first = event_log_iterator_first;
	iter->next = event_log_iterator_next;
	iter->is_done = event_log_iterator_is_done;
	iter->current = event_log_iterator_current;

	/* Initialize to start of collection */
	iter->begin_pos = variant->begin_pos;
	iter->end_pos = variant->end_pos;
	iter->cur_pos = variant->begin_pos;
}

static void event_log_iterator_first(struct claim_iterator *iter)
{
	iter->cur_pos = iter->begin_pos;
}

static bool event_log_iterator_next(struct claim_iterator *iter)
{
	const void *header = iter->cur_pos;
	size_t record_len;

	if (header == iter->begin_pos) {
		/* The first record must be in TCG EVENT-1 format */
		record_len = tcg_event1_record_size(header, iter->end_pos);
		if (!record_len) return false;      /* Problem in record */
	}
	else {
		/* All subsequent records are assumed to be in variable
		 * length TCG_PCR_EVENT2 format.
		 */
		record_len = tcg_event2_header_size(header, iter->end_pos);
		if (!record_len) return false;      /* Problem in header */

		/* Add the variable length space used for event data */
		const void *event_data = ((const uint8_t*)iter->cur_pos + record_len);
		record_len += sizeof(event2_data_t);
		record_len += load_u32_le(event_data, offsetof(event2_data_t, event_size));
	}

	/* Advance iterator to start of next record */
	iter->cur_pos = (const uint8_t*)iter->cur_pos + record_len;

	return !event_log_iterator_is_done(iter);
}

static bool event_log_iterator_is_done(struct claim_iterator *iter)
{
	return (iter->cur_pos >= iter->end_pos) || (iter->cur_pos < iter->begin_pos);
}

static bool event_log_iterator_current(struct claim_iterator *iter, struct claim *claim)
{
	bool success = false;

	if (!event_log_iterator_is_done(iter)) {

		uint32_t event_type = EV_NO_ACTION;
		const void *event_data = NULL;
		const void *header = iter->cur_pos;
		claim->raw_data = (const uint8_t*)header;

		if (header != iter->begin_pos) {
			/* Initial TSG EVENT-1 record is not supported */
			size_t header_len = tcg_event2_header_size(header, iter->end_pos);
			if (!header_len) return false;      /* Problem in header */

			event_type = load_u32_le(header, offsetof(event2_header_t, event_type));
			event_data = (const uint8_t*)header + header_len;
		}

		switch (event_type)
		{
			case EV_POST_CODE:
				/* A measurement claim */
				claim->category = CLAIM_CATEGORY_BOOT_MEASUREMENT;
				claim->subject_id = CLAIM_SUBJECT_ID_SW_COMPONENT;
				claim->variant_id = CLAIM_VARIANT_ID_MEASUREMENT;
				tcg_event2_extract_digest(header, &claim->variant.measurement);
				tcg_event2_extract_measurement_id(event_data, &claim->variant.measurement,
										iter->end_pos);
				break;

			default:
				/* Unsupported event type */
				claim->category = CLAIM_CATEGORY_NONE;
				claim->subject_id = CLAIM_SUBJECT_ID_NONE;
				claim->variant_id = CLAIM_VARIANT_ID_UNSUPPORTED;
				break;
		}

		success = true;
	}

	return success;
}

static size_t tcg_event2_digest_size(uint16_t algorithm_id)
{
	size_t size = 0;

	switch (algorithm_id)
	{
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

static size_t tcg_event2_header_size(const void *header, const void *limit)
{
	/* Return the length of the variable length header.  Returns zero if there's
	 * a problem.
	 */
	size_t header_len = 0;

	/* Ensure that the header is within the limit of the event log */
	if (((const uint8_t*)limit - sizeof(event2_header_t)) >= (const uint8_t*)header) {

		uint32_t digest_count = load_u32_le(header, offsetof(event2_header_t, digests.count));
		header_len = sizeof(event2_header_t);

		/* Add the variable length space used for digests */
		for (unsigned int i = 0; i < digest_count; ++i) {

			uint16_t algorithm_id =
				load_u16_le(header, offsetof(event2_header_t, digests.digests[i].algorithm_id));
			size_t digest_size =
				tcg_event2_digest_size(algorithm_id);

			if (digest_size) header_len += sizeof(tpmt_ha) + digest_size;
		}
	}

	return header_len;
}

static void tcg_event2_extract_digest(const void *header,
									struct measurement_claim_variant *measurement)
{
	uint32_t digest_count = load_u32_le(header, offsetof(event2_header_t, digests.count));

	measurement->digest.len = 0;
	measurement->digest.bytes = NULL;

	if (digest_count > 0) {

		uint16_t algorithm_id =
			load_u16_le(header, offsetof(event2_header_t, digests.digests[0].algorithm_id));
		size_t digest_size =
			tcg_event2_digest_size(algorithm_id);

		if (digest_size) {

			measurement->digest.len =
				digest_size;
			measurement->digest.bytes =
				(const uint8_t*)header + offsetof(event2_header_t, digests.digests[0].digest);
		}
	}
}

static size_t tcg_event1_record_size(const void *header, const void *limit)
{
	(void)limit;

	size_t record_len = load_u32_le(header, offsetof(tcg_pcr_event_t, event_size));
	record_len += sizeof(tcg_pcr_event_t);
	return record_len;
}

static void tcg_event2_extract_measurement_id(const void *event_data,
									struct measurement_claim_variant *measurement,
									const void *limit)
{
	measurement->id.string = NULL;

	if (((const uint8_t*)limit - sizeof(event2_data_t)) >= (const uint8_t*)event_data) {

		size_t id_size =  load_u32_le(event_data, offsetof(event2_data_t, event_size));

		if (id_size) {

			measurement->id.string = (const char*)event_data + offsetof(event2_data_t, event);
		}
	}
}
