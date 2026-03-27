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
#include "components/common/event_log/tcg.h"
#include "components/common/event_log/event_log_parser.h"


static bool event_log_claim_source_get_claim(void *context, struct claim *claim);
static void create_event_log_iterator(const struct claim_collection_variant *variant,
								struct claim_iterator *iter);

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

static void event_log_iterator_first(struct claim_iterator *iter)
{
	iter->cur_pos = iter->begin_pos;
}

static bool event_log_iterator_next(struct claim_iterator *iter)
{
	struct evl_context evl_context = {
		.begin = iter->begin_pos,
		.end = iter->end_pos,
		.pos = iter->cur_pos
	};

	(void)event_log_parser_next(&evl_context);

	iter->cur_pos = evl_context.pos;

	return event_log_parser_is_done(&evl_context);
}

static bool event_log_iterator_is_done(struct claim_iterator *iter)
{
	struct evl_context evl_context = {
		.begin = iter->begin_pos,
		.end = iter->end_pos,
		.pos = iter->cur_pos
	};
	return event_log_parser_is_done(&evl_context);
}

static bool event_log_iterator_current(struct claim_iterator *iter, struct claim *claim)
{
	bool success = false;
	struct evl_context evl_context = {
		.begin = iter->begin_pos,
		.end = iter->end_pos,
		.pos = iter->cur_pos
	};

	struct evl_record_data data = {0};

	if (!event_log_parser_get(&evl_context, &data)) {
		if (data.data_type == rct_pcr_record && data.data.pcr_record.header.event_type == EV_POST_CODE) {
			claim->category = CLAIM_CATEGORY_BOOT_MEASUREMENT;
			claim->subject_id = CLAIM_SUBJECT_ID_SW_COMPONENT;
			claim->variant_id = CLAIM_VARIANT_ID_MEASUREMENT;
			if (event_log_parser_get_measurement_digest(&data, TPM_ALG_SHA256,
				&claim->variant.measurement.digest.bytes,
				&claim->variant.measurement.digest.len)) {
					goto error;
				}
			if (event_log_parser_get_measurement_id(&data,
							  &claim->variant.measurement.id.string)) {
				goto error;
			}
		} else {
			/* Unsupported event type */
			claim->category = CLAIM_CATEGORY_NONE;
			claim->subject_id = CLAIM_SUBJECT_ID_NONE;
			claim->variant_id = CLAIM_VARIANT_ID_UNSUPPORTED;
		}
		success = true;
	}

	return success;

error:
	memset(claim, 0, sizeof(*claim));
	return false;
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
