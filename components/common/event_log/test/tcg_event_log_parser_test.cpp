/*
 * Copyright (c) 2026, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "components/common/event_log/event_log_parser.h"
#include "components/common/event_log/tcg.h"
#include "components/service/attestation/claims/sources/event_log/mock/mock_event_log.h"

#include <CppUTest/TestHarness.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <vector>


static size_t digest_type2size(uint16_t algorithm_id)
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

class SpecIDVersionEvent {
public:
	std::vector<uint8_t> buffer;
	uint32_t n_algorithm_sizes;

	static const id_event_algorithm_size_t algorithm_size_defs[4];
	static const size_t default_size;

	SpecIDVersionEvent(uint8_t ver_major = 2, uint8_t ver_minor = 0, uint8_t spec_rev = 2,
			 uint8_t uintn_size = 1, uint32_t n_alg_sizes = 1)
	{
		size_t new_len = default_size + ((n_alg_sizes - 1) * sizeof(id_event_algorithm_size_t));
		n_algorithm_sizes = n_alg_sizes;

		buffer.resize(new_len);

		uint8_t *buffer_pos = buffer.data();

		*(tcg_pcr_event_t *)buffer_pos = {
			.pcr_index = 0,
			.event_type = EV_NO_ACTION,
			.digest = { 0 },
			.event_size =
				(uint32_t)(sizeof(id_event_struct_header_t) +
					   sizeof(id_event_algorithm_size_t) * n_algorithm_sizes +
					   sizeof(id_event_struct_data_t) + 10)
		};

		buffer_pos += sizeof(tcg_pcr_event_t);
		*(id_event_struct_header_t *)(buffer_pos) = {
			.signature = TCG_ID_EVENT_SIGNATURE_03,
			.platform_class = 0,
			.spec_version_minor = ver_minor,
			.spec_version_major = ver_major,
			.spec_errata = spec_rev,
			.uintn_size = uintn_size,
			.number_of_algorithms = n_algorithm_sizes
		};

		buffer_pos += sizeof(id_event_struct_header_t);
		memcpy(buffer_pos, algorithm_size_defs,
		       sizeof(id_event_algorithm_size_t) * n_algorithm_sizes);

		buffer_pos += sizeof(id_event_algorithm_size_t) * n_algorithm_sizes;

		*(id_event_struct_data_t *)(buffer_pos) = {
			.vendor_info_size = 10,
		};
		buffer_pos += sizeof(id_event_struct_data_t);
		memcpy(buffer_pos, "123456789", 10);
	}

	~SpecIDVersionEvent()
	{
	}

	uint8_t get_ver_major()
	{
		id_event_struct_header_t *header =
			(id_event_struct_header_t *)(buffer.data() + sizeof(tcg_pcr_event_t));
		return header->spec_version_major;
	}

	uint8_t get_ver_minor()
	{
		id_event_struct_header_t *header =
			(id_event_struct_header_t *)(buffer.data() + sizeof(tcg_pcr_event_t));
		return header->spec_version_minor;
	}

	uint8_t get_spec_rev()
	{
		id_event_struct_header_t *header =
			(id_event_struct_header_t *)(buffer.data() + sizeof(tcg_pcr_event_t));
		return header->spec_errata;
	}

	uint8_t get_uintn_size()
	{
		id_event_struct_header_t *header =
			(id_event_struct_header_t *)(buffer.data() + sizeof(tcg_pcr_event_t));
		return header->uintn_size;
	}

	uint8_t get_platform_class()
	{
		id_event_struct_header_t *header =
			(id_event_struct_header_t *)(buffer.data() + sizeof(tcg_pcr_event_t));
		return header->platform_class;
	}

	const uint8_t *get_signature()
	{
		id_event_struct_header_t *header =
			(id_event_struct_header_t *)(buffer.data() + sizeof(tcg_pcr_event_t));
		return (const uint8_t *)header->signature;
	}

	uint8_t get_vendor_info_size()
	{
		id_event_struct_data_t *vend_info =
			(id_event_struct_data_t *)(buffer.data() + sizeof(tcg_pcr_event_t) +
						   sizeof(id_event_struct_header_t) +
						   sizeof(id_event_algorithm_size_t) *
							   n_algorithm_sizes);
		return vend_info->vendor_info_size;
	}

	const uint8_t *get_vendor_info()
	{
		id_event_struct_data_t *vend_info =
			(id_event_struct_data_t *)(buffer.data() + sizeof(tcg_pcr_event_t) +
						   sizeof(id_event_struct_header_t) +
						   sizeof(id_event_algorithm_size_t) *
							   n_algorithm_sizes);
		return vend_info->vendor_info;
	}

	void set_version(uint8_t ver_major = 2, uint8_t ver_minor = 0, uint8_t spec_rev = 2)
	{
		id_event_struct_header_t *header =
			(id_event_struct_header_t *)(buffer.data() + sizeof(tcg_pcr_event_t));
		header->spec_version_major = ver_major;
		header->spec_version_minor = ver_minor;
		header->spec_errata = spec_rev;
	}

	void set_uintn_size(uint8_t uintn_size = 1)
	{
		id_event_struct_header_t *header =
			(id_event_struct_header_t *)(buffer.data() + sizeof(tcg_pcr_event_t));
		header->uintn_size = uintn_size;
	}

	void set_n_alg(uint8_t n_alg = 1)
	{
		id_event_struct_header_t *header =
			(id_event_struct_header_t *)(buffer.data() + sizeof(tcg_pcr_event_t));
		header->number_of_algorithms = n_alg;
	}

	void set_first_alg(uint16_t algorithm_id, uint16_t digest_size)
	{
		id_event_algorithm_size_t *alg_info =
			(id_event_algorithm_size_t *)(buffer.data() + sizeof(tcg_pcr_event_t) +
						      sizeof(id_event_struct_header_t));
		alg_info->algorithm_id = algorithm_id;
		alg_info->digest_size = digest_size;
	}

	void set_vendor_info(uint8_t size, uint8_t *data)
	{
		id_event_struct_data_t *vend_info =
			(id_event_struct_data_t *)(buffer.data() + sizeof(tcg_pcr_event_t) +
						   sizeof(id_event_struct_header_t) +
						   sizeof(id_event_algorithm_size_t) *
							   n_algorithm_sizes);
		vend_info->vendor_info_size = size;
		if (data) {
			memcpy(vend_info->vendor_info, data, size);
		}
	}

	void set_event_size(uint32_t new_size)
	{
		tcg_pcr_event_t *header = (tcg_pcr_event_t *) buffer.data();
		header->event_size = new_size;
	}


	uint16_t get_algorithm_id(size_t index)
	{
		id_event_algorithm_size_t *alg_info =
			(id_event_algorithm_size_t *)(buffer.data() + sizeof(tcg_pcr_event_t) +
						      sizeof(id_event_struct_header_t));
		assert(index < n_algorithm_sizes);

		alg_info += index;
		return alg_info->algorithm_id;
	}

	uint16_t get_algorithm_size(size_t index)
	{
		id_event_algorithm_size_t *alg_info =
			(id_event_algorithm_size_t *)(buffer.data() + sizeof(tcg_pcr_event_t) +
						      sizeof(id_event_struct_header_t));
		assert(index < n_algorithm_sizes);

		alg_info += index;
		return alg_info->digest_size;
	}

	bool check_equal(evl_spec_id_record &record)
	{
		LONGS_EQUAL(n_algorithm_sizes, record.number_of_algorithms);
		for (size_t x = 0; x < n_algorithm_sizes; x++) {
			LONGS_EQUAL(get_algorithm_id(x), record.digest_size_list[x].algorithm_id);
			LONGS_EQUAL(get_algorithm_size(x), record.digest_size_list[x].digest_size);
		}

		LONGS_EQUAL(get_platform_class(), record.platform_class);
		CHECK_TRUE((get_uintn_size() == 2 ? 1 : 0) == record.uintn_is_64_bit);
		LONGS_EQUAL(get_vendor_info_size(), record.vendor_info_size);
		MEMCMP_EQUAL(get_vendor_info(), (const char *)record.vendor_info,
			     get_vendor_info_size());
		LONGS_EQUAL(get_spec_rev(), record.spec_revision);
		LONGS_EQUAL(get_ver_major(), record.version_major);
		LONGS_EQUAL(get_ver_minor(), record.version_minor);

		return true;
	}
};

const id_event_algorithm_size_t SpecIDVersionEvent::algorithm_size_defs[4] = {
	{ .algorithm_id = TPM_ALG_SHA1, .digest_size = SHA1_DIGEST_SIZE },
	{ .algorithm_id = TPM_ALG_SHA256, .digest_size = SHA256_DIGEST_SIZE },
	{ .algorithm_id = TPM_ALG_SHA384, .digest_size = SHA384_DIGEST_SIZE },
	{ .algorithm_id = TPM_ALG_SHA512, .digest_size = SHA512_DIGEST_SIZE },
};

const size_t SpecIDVersionEvent::default_size =
	sizeof(tcg_pcr_event_t) + sizeof(id_event_struct_header_t) +
	sizeof(id_event_algorithm_size_t) + sizeof(id_event_struct_data_t) + 10;


class PcrEvent2Record {
private:
	event2_data_t *get_event_pos()
	{
		uint8_t *pos = buffer.data();
		uint32_t count = get_digest_count();

		pos += sizeof(event2_header_t);
		for (unsigned int x = 0; x < count; x++) {
			size_t ds = digest_type2size(((tpmt_ha *)pos)->algorithm_id);
			assert(ds != 0);
			pos += ds + sizeof(tpmt_ha);
		}

		return reinterpret_cast<event2_data_t *>(pos);
	}

public:
	std::vector<uint8_t> buffer;

	PcrEvent2Record(uint32_t pcr_index, uint32_t event_type)
	{
		buffer.resize(sizeof(event2_header_t) + sizeof(event2_data_t));

		uint8_t *pos = buffer.data();

		((event2_header_t *)pos)->pcr_index = pcr_index;
		((event2_header_t *)pos)->event_type = event_type;
		((event2_header_t *)pos)->digests.count = 0;
		pos += sizeof(event2_header_t);
		((event2_data *)pos)->event_size = 0;

		add_digest(TPM_ALG_SHA256);
	}

	uint32_t get_pcr_index()
	{
		uint8_t *pos = buffer.data();

		return ((event2_header_t *)pos)->pcr_index;
	}

	uint32_t get_event_type()
	{
		uint8_t *pos = buffer.data();

		return ((event2_header_t *)pos)->event_type;
	}

	uint32_t get_digest_count()
	{
		uint8_t *pos = buffer.data();
		return ((event2_header_t *)pos)->digests.count;
	}

	tpmt_ha *get_digest(size_t index)
	{
		uint8_t *pos = buffer.data();

		uint32_t count = get_digest_count();

		if (count <= index)
			return 0;

		pos += sizeof(event2_header_t);
		for (unsigned int x = 0; x < index; x++) {
			size_t ds = digest_type2size(((tpmt_ha *)pos)->algorithm_id);
			assert(ds != 0);
			pos += ds + sizeof(tpmt_ha);
		}

		return reinterpret_cast<tpmt_ha *>(pos);
	}

	uint16_t get_digest_size(size_t index)
	{
		return digest_type2size(get_digest(index)->algorithm_id);
	}

	uint16_t get_digest_algorithm(size_t index)
	{
		return get_digest(index)->algorithm_id;
	}

	uint8_t *get_digest_value(size_t index)
	{
		return get_digest(index)->digest;
	}

	void add_digest(uint16_t algorithm_id, size_t digest_size, uint8_t *data)
	{
		uint8_t *pos = buffer.data();

		uint32_t count = ((event2_header_t *)pos)->digests.count;

		std::vector<uint8_t> digest(sizeof(tpmt_ha) + digest_size);
		((tpmt_ha *)digest.data())->algorithm_id = algorithm_id;
		memcpy(&((tpmt_ha *)digest.data())->digest, data, digest_size);

		pos += sizeof(event2_header_t);
		for (unsigned int x = 0; x < count; x++) {
			pos += digest_type2size(((tpmt_ha *)pos)->algorithm_id);
			pos += sizeof(tpmt_ha);
		}

		size_t index = pos - buffer.data();
		buffer.insert(buffer.begin() + index, digest.begin(), digest.end());

		pos = buffer.data();
		((event2_header_t *)pos)->digests.count++;
	}

	void add_digest(uint16_t algorithm_id)
	{
		size_t digest_size = digest_type2size(algorithm_id);
		std::vector<uint8_t> digest(digest_size);
		digest.assign(reinterpret_cast<uint8_t *>(&algorithm_id),
			      reinterpret_cast<uint8_t *>(&algorithm_id) + sizeof(algorithm_id));
		add_digest(algorithm_id, digest_size, digest.data());
	}

	void add_event(uint32_t size, uint8_t *data)
	{
		event2_data_t *pos = get_event_pos();

		int increment = size - pos->event_size;
		buffer.resize(buffer.size() + increment);

		pos = get_event_pos();
		pos->event_size = size;
		memcpy(pos->event, data, size);
	}

	void set_event_size(uint32_t size)
	{
		event2_data_t *pos = get_event_pos();
		pos->event_size = size;
	}

	void set_digest_count(uint32_t count)
	{
		((event2_header_t *)buffer.data())->digests.count = count;
	}

	uint32_t get_event_size()
	{
		event2_data_t *pos = get_event_pos();

		return pos->event_size;
	}

	uint8_t *get_event()
	{
		event2_data_t *pos = get_event_pos();
		return pos->event;
	}

	bool check_equal(evl_event2_record &record)
	{
		LONGS_EQUAL(get_event_type(), record.header.event_type);
		LONGS_EQUAL(get_digest_count(), record.header.n_digests);
		for (size_t x = 0; x < get_digest_count(); x++) {
			LONGS_EQUAL(get_digest_algorithm(x), record.header.digests[x].algorithm_id);
			MEMCMP_EQUAL(get_digest_value(x), record.header.digests[x].digest,
				     get_digest_size(x));
		}
		LONGS_EQUAL(get_pcr_index(), record.header.pcr_index);
		LONGS_EQUAL(get_event_size(), record.data_size);
		MEMCMP_EQUAL(get_event(), record.event_data, record.data_size);

		return true;
	}
};

class StartupLocalityRecord {
public:
	PcrEvent2Record record;

	StartupLocalityRecord()
		: record(0, EV_NO_ACTION)
	{
		startup_locality_event_t event = { .signature = "StartupLocality",
						   .startup_locality = 0 };

		record.add_event(sizeof(event), (uint8_t *)&event);
	}

	uint8_t get_locality()
	{
		return record.get_event()[16];
	}

	uint8_t *get_signature()
	{
		return record.get_event();
	}

	size_t get_signature_length()
	{
		return record.get_event_size();
	}
};

class PostCodeRecord {
public:
	PcrEvent2Record record;

	PostCodeRecord(const char *string)
		: record(0, EV_POST_CODE)
	{
		uint32_t length = strlen(string) + 1;
		record.add_event(length, (uint8_t*) string);
	}

	uint8_t *get_signature()
	{
		return record.get_event();
	}

	size_t get_signature_length()
	{
		return record.get_event_size();
	}
};

TEST_GROUP(TcgEventLogParserTests){

};

#define CHECK_EVENT_LOG_INIT_OK(context, data, data_len)                                      \
	CHECK_FALSE(event_log_parser_init((context), (data), (data_len)));                           \
	POINTERS_EQUAL_TEXT((data), (context)->begin, "Incorrect 'begin' pointer.");          \
	POINTERS_EQUAL_TEXT((data) + (data_len), (context)->end, "Incorrect 'end' pointer."); \
	POINTERS_EQUAL_TEXT((data), (context)->pos, "Incorrect 'pos' pointer.");              \
	CHECK_FALSE(event_log_parser_is_done(context));

#define CHECK_EVENT_LOG_INIT_FAIL(context, data, data_len)                                    \
	CHECK_TRUE(event_log_parser_init((context), (data), (data_len)));                            \
	POINTERS_EQUAL_TEXT((data), (context)->begin, "Incorrect 'begin' pointer.");          \
	POINTERS_EQUAL_TEXT((data) + (data_len), (context)->end, "Incorrect 'end' pointer."); \
	POINTERS_EQUAL_TEXT((data) + (data_len), (context)->pos, "Incorrect 'pos' pointer."); \
	LONGS_EQUAL(uintns_unknown, (context)->uintn_size);                                   \
	CHECK_TRUE(event_log_parser_is_done(context));

TEST(TcgEventLogParserTests, iterateCapturedEventLogData)
{
	struct evl_context context = { 0 };
	struct evl_record_data record = { .data_type = rct_id_record };

	for (size_t x = 0; x < mock_event_log_list_length(); x++) {
		unsigned int measurement_count = 0;
		const struct mock_event_log_info *event_log_info = mock_event_log_list_get_at(x);

		CHECK_EVENT_LOG_INIT_OK(&context, event_log_info->data,
					event_log_info->data_length);

		/* Iterate over all claims in the collection */
		while (!event_log_parser_is_done(&context)) {
			CHECK_FALSE(event_log_parser_get(&context, &record));
			CHECK_EQUAL(record.data_type,
				    measurement_count == 0 ? rct_id_record : rct_pcr_record);

			++measurement_count;

			if (measurement_count < event_log_info->n_claims) {
				CHECK_TRUE(event_log_parser_next(&context));
			} else {
				CHECK_FALSE(event_log_parser_next(&context));
			}

		}

		UNSIGNED_LONGS_EQUAL(event_log_info->n_claims, measurement_count);
	}
}

TEST(TcgEventLogParserTests, ZeroLengthLog)
{
	struct evl_context context = { 0 };
	uint32_t fake_event_buf = 0;

	event_log_parser_init(&context, (const uint8_t *)&fake_event_buf, 0);
}

TEST(TcgEventLogParserTests, specIdEvent_BadPcrValue)
{
	struct evl_context context = { 0 };
	tcg_pcr_event_t fake_event_buf = { .pcr_index = 0,
					   .event_type = EV_NO_ACTION,
					   .digest = { 0 },
					   .event_size = 0 };

	CHECK_EVENT_LOG_INIT_FAIL(&context, (const uint8_t *)&fake_event_buf,
				  sizeof(fake_event_buf));
}

TEST(TcgEventLogParserTests, specIdEvent_BadEventType)
{
	struct evl_context context = { 0 };
	tcg_pcr_event_t fake_event_buf = { .pcr_index = 0,
					   .event_type = EV_SEPARATOR,
					   .digest = { 0 },
					   .event_size = 0 };

	CHECK_EVENT_LOG_INIT_FAIL(&context, (const uint8_t *)&fake_event_buf,
				  sizeof(fake_event_buf));
}

TEST(TcgEventLogParserTests, specIdEvent_BadDigest)
{
	struct evl_context context = { 0 };
	tcg_pcr_event_t fake_event_buf = { .pcr_index = 0,
					   .event_type = EV_NO_ACTION,
					   .digest = { 0 },
					   .event_size = 0 };

	fake_event_buf.digest[2] = 3;

	CHECK_EVENT_LOG_INIT_FAIL(&context, (const uint8_t *)&fake_event_buf,
				  sizeof(fake_event_buf));
}

TEST(TcgEventLogParserTests, specIdEvent_BadSignature)
{
	struct evl_context context = { 0 };
	uint8_t fake_event_buf[sizeof(tcg_pcr_event_t) + 16];

	*(tcg_pcr_event_t *)(&fake_event_buf[0]) = { .pcr_index = 0,
						     .event_type = EV_NO_ACTION,
						     .digest = { 0 },
						     .event_size = 16 };
	memcpy(&fake_event_buf[sizeof(tcg_pcr_event_t)], TCG_ID_EVENT_SIGNATURE_03, 16);
	fake_event_buf[sizeof(tcg_pcr_event_t) + 3] = '5';

	CHECK_EVENT_LOG_INIT_FAIL(&context, (const uint8_t *)&fake_event_buf,
				  sizeof(fake_event_buf));
}

TEST(TcgEventLogParserTests, specIdEvent_IncorrectVersion)
{
	struct evl_context context = { 0 };
	SpecIDVersionEvent event_log;

	event_log.set_version(2, 1, 2);
	CHECK_EVENT_LOG_INIT_FAIL(&context, event_log.buffer.data(), event_log.buffer.size());

	event_log.set_version(1, 0, 2);
	CHECK_EVENT_LOG_INIT_FAIL(&context, event_log.buffer.data(), event_log.buffer.size());

	event_log.set_version(2, 0, 1);
	CHECK_EVENT_LOG_INIT_FAIL(&context, event_log.buffer.data(), event_log.buffer.size());

	event_log.set_version();
	CHECK_EVENT_LOG_INIT_OK(&context, event_log.buffer.data(), event_log.buffer.size());
	LONGS_EQUAL(context.uintn_size, uintns_32);
}

TEST(TcgEventLogParserTests, specIdEvent_UintnSize)
{
	struct evl_context context = { 0 };
	SpecIDVersionEvent event_log;

	event_log.set_uintn_size(1);
	CHECK_EVENT_LOG_INIT_OK(&context, event_log.buffer.data(), event_log.buffer.size());
	LONGS_EQUAL(context.uintn_size, uintns_32);

	event_log.set_uintn_size(2);
	CHECK_EVENT_LOG_INIT_OK(&context, event_log.buffer.data(), event_log.buffer.size());
	LONGS_EQUAL(context.uintn_size, uintns_64);

	event_log.set_uintn_size(0);
	CHECK_EVENT_LOG_INIT_FAIL(&context, event_log.buffer.data(), event_log.buffer.size());
}

TEST(TcgEventLogParserTests, specIdEvent_InvalidAlgorithmArraySize)
{
	struct evl_context context = { 0 };
	SpecIDVersionEvent event_log;

	event_log.set_n_alg(0);
	CHECK_EVENT_LOG_INIT_FAIL(&context, event_log.buffer.data(), event_log.buffer.size());

	event_log.set_n_alg(2);
	CHECK_EVENT_LOG_INIT_FAIL(&context, event_log.buffer.data(), event_log.buffer.size());
}

TEST(TcgEventLogParserTests, specIdEvent_BadSHASize)
{
	struct evl_context context = { 0 };
	SpecIDVersionEvent event_log;

	event_log.set_first_alg(TPM_ALG_SHA1, 3);
	CHECK_EVENT_LOG_INIT_FAIL(&context, event_log.buffer.data(), event_log.buffer.size());

	event_log.set_first_alg(TPM_ALG_SHA256, 3);
	CHECK_EVENT_LOG_INIT_FAIL(&context, event_log.buffer.data(), event_log.buffer.size());

	event_log.set_first_alg(TPM_ALG_SHA384, 3);
	CHECK_EVENT_LOG_INIT_FAIL(&context, event_log.buffer.data(), event_log.buffer.size());

	event_log.set_first_alg(TPM_ALG_SHA512, 3);
	CHECK_EVENT_LOG_INIT_FAIL(&context, event_log.buffer.data(), event_log.buffer.size());
}

TEST(TcgEventLogParserTests, specIdEvent_unknownSHAType)
{
	struct evl_context context = { 0 };
	SpecIDVersionEvent event_log;

	event_log.set_first_alg(33, 3);
	CHECK_EVENT_LOG_INIT_FAIL(&context, event_log.buffer.data(), event_log.buffer.size());
}

TEST(TcgEventLogParserTests, specIdEvent_missingVendorInfoBytes)
{
	struct evl_context context = { 0 };
	SpecIDVersionEvent event_log;

	event_log.set_vendor_info(15, NULL);
	CHECK_EVENT_LOG_INIT_FAIL(&context, event_log.buffer.data(), event_log.buffer.size());
}

TEST(TcgEventLogParserTests, specIdEvent_eventSizeTooBig)
{
	struct evl_context context = { 0 };
	SpecIDVersionEvent event_log;

	event_log.set_event_size(66);
	CHECK_EVENT_LOG_INIT_FAIL(&context, event_log.buffer.data(), event_log.buffer.size());
}

TEST(TcgEventLogParserTests, specIdEvent_invalidBufferSize)
{
	struct evl_context context = { 0 };
	struct evl_record_data data = {};

	SpecIDVersionEvent id_record;
	StartupLocalityRecord st_locality_record;

	size_t skip_list[] = {
		5000
	};
	size_t* skip_pos=&skip_list[0];

	id_record.set_event_size(0);

	std::vector<uint8_t> event_log = id_record.buffer;
	event_log.insert(event_log.end(), st_locality_record.record.buffer.begin(),
			 st_locality_record.record.buffer.end());

	for(size_t size=0; size < id_record.buffer.size(); size++) {

		CHECK_EVENT_LOG_INIT_FAIL(&context, event_log.data(), size);
		if (skip_pos < &skip_list[sizeof(skip_list) / sizeof(skip_list[0])] &&
		    *skip_pos == size) {
			skip_pos++;
			continue;
		}
		CHECK_TRUE(event_log_parser_get(&context, &data));
		memset(&context, 0, sizeof(context));
	}
}

TEST(TcgEventLogParserTests, event2_invalidBufferSize)
{
	struct evl_context context = { 0 };
	struct evl_record_data data = {};

	SpecIDVersionEvent id_record;
	StartupLocalityRecord st_locality_record;

	size_t skip_list[] = {
		5000
	};
	size_t* skip_pos=&skip_list[0];

	std::vector<uint8_t> event_log = id_record.buffer;
	event_log.insert(event_log.end(), st_locality_record.record.buffer.begin(),
			 st_locality_record.record.buffer.end());

	for(size_t size=id_record.buffer.size(); size < event_log.size(); size++) {

		CHECK_EVENT_LOG_INIT_OK(&context, event_log.data(), size);
		if (skip_pos < &skip_list[sizeof(skip_list) / sizeof(skip_list[0])] &&
		    *skip_pos == size) {
			skip_pos++;
			continue;
		}
		(void)event_log_parser_next(&context);
		CHECK_TRUE(event_log_parser_get(&context, &data));
		memset(&context, 0, sizeof(context));
	}
}

TEST(TcgEventLogParserTests, event2_invalidDigestCount)
{
	struct evl_context context = { 0 };
	SpecIDVersionEvent id_record;
	StartupLocalityRecord st_locality_record;
	PostCodeRecord bl33_sha("BL_33");

	st_locality_record.record.set_digest_count(0);

	std::vector<uint8_t> event_log = id_record.buffer;
	event_log.insert(event_log.end(), st_locality_record.record.buffer.begin(),
			 st_locality_record.record.buffer.end());
	event_log.insert(event_log.end(), bl33_sha.record.buffer.begin(), bl33_sha.record.buffer.end());

	CHECK_EVENT_LOG_INIT_OK(&context, event_log.data(), event_log.size());
	CHECK_TRUE(event_log_parser_next(&context));
	CHECK_FALSE(event_log_parser_is_done(&context));
	CHECK_FALSE(event_log_parser_next(&context));

	st_locality_record.record.set_digest_count(3);
	event_log = id_record.buffer;
	event_log.insert(event_log.end(), st_locality_record.record.buffer.begin(),
			 st_locality_record.record.buffer.end());
	event_log.insert(event_log.end(), bl33_sha.record.buffer.begin(), bl33_sha.record.buffer.end());

	CHECK_EVENT_LOG_INIT_OK(&context, event_log.data(), event_log.size());
	CHECK_TRUE(event_log_parser_next(&context));
	CHECK_FALSE(event_log_parser_is_done(&context));
	CHECK_FALSE(event_log_parser_next(&context));
}

TEST(TcgEventLogParserTests, event2_invalidAlgorithm)
{
	struct evl_context context = {};
	SpecIDVersionEvent id_record;
	StartupLocalityRecord st_locality_record;
	struct evl_record_data data = {};

	uint32_t value = 0;
	st_locality_record.record.add_digest(66, 4, (uint8_t *)&value);

	std::vector<uint8_t> event_log = id_record.buffer;
	event_log.insert(event_log.end(), st_locality_record.record.buffer.begin(),
			 st_locality_record.record.buffer.end());

	CHECK_EVENT_LOG_INIT_OK(&context, event_log.data(), event_log.size());
	CHECK_TRUE(event_log_parser_next(&context));
	CHECK_FALSE(event_log_parser_next(&context));
	CHECK_TRUE(event_log_parser_get(&context, &data));
}

TEST(TcgEventLogParserTests, event2_invalidEventSize)
{
	struct evl_context context = { 0 };
	SpecIDVersionEvent id_record;
	StartupLocalityRecord st_locality_record;
	struct evl_record_data data = {};

	st_locality_record.record.set_event_size(77);

	std::vector<uint8_t> event_log = id_record.buffer;
	event_log.insert(event_log.end(), st_locality_record.record.buffer.begin(),
			 st_locality_record.record.buffer.end());

	CHECK_EVENT_LOG_INIT_OK(&context, event_log.data(), event_log.size());
	CHECK_TRUE(event_log_parser_next(&context));
	CHECK_FALSE(event_log_parser_next(&context));
	CHECK_TRUE(event_log_parser_get(&context, &data));
}

TEST(TcgEventLogParserTests, readTest)
{
	struct evl_context context = { 0 };
	struct evl_record_data data = {};

	SpecIDVersionEvent id_record;
	StartupLocalityRecord st_locality_record;
	PostCodeRecord bl2_measurement("BL_2");

	std::vector<uint8_t> event_log = id_record.buffer;
	event_log.insert(event_log.end(), st_locality_record.record.buffer.begin(),
			 st_locality_record.record.buffer.end());
	event_log.insert(event_log.end(), bl2_measurement.record.buffer.begin(),
			 bl2_measurement.record.buffer.end());

	// Parser init
	CHECK_EVENT_LOG_INIT_OK(&context, event_log.data(), event_log.size());

	// Read & check the ID record
	CHECK_FALSE(event_log_parser_get(&context, &data));
	LONGS_EQUAL(rct_id_record, data.data_type);
	id_record.check_equal(data.data.id_record);

	// Check locality record
	CHECK_TRUE(event_log_parser_next(&context));
	CHECK_FALSE(event_log_parser_get(&context, &data));
	LONGS_EQUAL(rct_pcr_record, data.data_type);
	st_locality_record.record.check_equal(data.data.pcr_record);

	LONGS_EQUAL(st_locality_record.get_locality(), data.data.pcr_record.event_data[16]);
	MEMCMP_EQUAL(st_locality_record.get_signature(), data.data.pcr_record.event_data,
		     st_locality_record.get_signature_length());

	// Check boot measurement record
	CHECK_TRUE(event_log_parser_next(&context));
	CHECK_FALSE(event_log_parser_get(&context, &data));
	LONGS_EQUAL(rct_pcr_record, data.data_type);
	bl2_measurement.record.check_equal(data.data.pcr_record);

	const char *id_string;
	CHECK_FALSE(event_log_parser_get_measurement_id(&data, &id_string));
	MEMCMP_EQUAL(bl2_measurement.get_signature(), id_string,
		     bl2_measurement.get_signature_length());

	size_t digest_size = 0;
	const uint8_t *digest_data = NULL;
	for (size_t x = 0; x < data.data.pcr_record.header.n_digests; x++) {
		uint8_t alg_id = bl2_measurement.record.get_digest_algorithm(x);
		digest_size = 0;
		digest_data = NULL;
		CHECK_FALSE(event_log_parser_get_measurement_digest(&data, alg_id, &digest_data, &digest_size));
		LONGS_EQUAL(bl2_measurement.record.get_digest_size(x), digest_size);
		MEMCMP_EQUAL(bl2_measurement.record.get_digest_value(x), digest_data, digest_size);
	}

	CHECK_TRUE(event_log_parser_get_measurement_digest(&data, 66, &digest_data, &digest_size));

	const_cast<uint8_t *>(data.data.pcr_record.event_data)[data.data.pcr_record.data_size-1] = 'a';
	CHECK_TRUE(event_log_parser_get_measurement_id(&data, &id_string));

	data.data_type = rct_id_record;
	CHECK_TRUE(event_log_parser_get_measurement_digest(&data, TPM_ALG_SHA256, &digest_data, &digest_size));
	CHECK_TRUE(event_log_parser_get_measurement_id(&data, &id_string));

	// Iterator shall not find further records.
	CHECK_FALSE(event_log_parser_next(&context));
}
