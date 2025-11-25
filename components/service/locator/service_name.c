/*
 * Copyright (c) 2020-2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <ctype.h>
#include <string.h>
#include "service_name.h"

/*
 * Structure to hold result of parsing the service name.  Sets
 * indices and length of the following fields.  Lengths are set to zero
 * if field is not present.
 */
struct sn_parsed_fields
{
	size_t authority_pos;
	size_t authority_len;
	size_t service_pos;
	size_t service_len;
	size_t version_pos;
	size_t version_len;
	size_t instance_pos;
	size_t instance_len;
};

/*
 * Find the lengthof the current field, up to the specified separator or end
 * of the service name.
 */
static size_t field_len(const char *sn, size_t from, size_t len, char separator)
{
	size_t field_len = 0;
	size_t pos = from;

	while (pos < len) {

		if (sn[pos] == separator) break;

		++pos;
		++field_len;
	}

	return field_len;
}

/*
 * Find the next field in the service name after the specified separator from the
 * specified position.
 */
static bool find_next(const char *sn, size_t from, size_t len, char separator, size_t *next)
{
	bool found = false;
	size_t pos = from;

	while (pos < len) {

		if (sn[pos] == separator) {
			*next = pos + 1;
			found = (*next < len);
			break;
		}

		++pos;
	}

	return found;
}

/*
 * Parse the service name.  Returns true if is a legal service
 * name and mandatory fields are present;
 */
static bool sn_parse(const char *sn, struct sn_parsed_fields *fields)
{
	size_t sn_len = strlen(sn);
	const char *str_urn = "urn";
	const size_t str_urn_len = strlen(str_urn);
	const char *str_sn = "sn";
	const size_t str_sn_len = strlen(str_sn);
	size_t field_pos = 0;

	fields->authority_pos = 0;
	fields->authority_len = 0;
	fields->service_pos = 0;
	fields->service_len = 0;
	fields->version_pos = 0;
	fields->version_len = 0;
	fields->instance_pos = 0;
	fields->instance_len = 0;

	if (sn_len >= str_urn_len) {
		/* Absorb urn: if present */
		if (memcmp(str_urn, &sn[field_pos], str_urn_len) == 0) {
			if (!find_next(sn, field_pos, sn_len, ':', &field_pos)) return false;
		}
	}

	/* Check it is a service name */
	if (sn_len - field_pos < str_sn_len) {
		return false;
	}
	if (memcmp(str_sn, &sn[field_pos], str_sn_len) != 0) {
		/* Not a service name */
		 return false;
	}

	/* Expect the authority field */
	if (find_next(sn, field_pos, sn_len, ':', &field_pos)) {
		fields->authority_pos = field_pos;
		fields->authority_len = field_len(sn, field_pos, sn_len, ':');
	}
	else {
		/* Missing mandatory authority field */
		return false;
	}

	/* Expect the service field */
	if (find_next(sn, field_pos, sn_len, ':', &field_pos)) {
		fields->service_pos = field_pos;
		fields->service_len = field_len(sn, field_pos, sn_len, ':');

		/* Check for the optional version */
		if (find_next(sn, field_pos, field_pos + fields->service_len, '.', &field_pos)) {
			fields->version_pos = field_pos;
			fields->version_len = field_len(sn, field_pos, sn_len, ':');
		}
	}
	else {
		/* Missing mandatory service field */
		return false;
	}

	/* Check for optional instance */
	if (find_next(sn, field_pos, sn_len, ':', &field_pos)) {
		fields->instance_pos = field_pos;
		fields->instance_len = sn_len - field_pos;
	}

	return true;
}

bool sn_is_valid(const char *sn)
{
	struct sn_parsed_fields fields;
	return sn_parse(sn, &fields);
}

bool sn_check_authority(const char *sn, const char *auth)
{
	bool matched = false;
	struct sn_parsed_fields fields;

	if (sn_parse(sn, &fields) && fields.authority_len) {

		matched = (memcmp(auth, &sn[fields.authority_pos], strlen(auth)) == 0);
	}

	return matched;
}

bool sn_check_service(const char *sn, const char *service)
{
	bool matched = false;
	struct sn_parsed_fields fields;

	if (sn_parse(sn, &fields) && fields.service_len) {

		matched = (memcmp(service, &sn[fields.service_pos], strlen(service)) == 0);
	}

	return matched;
}

unsigned int sn_get_service_instance(const char *sn)
{
	unsigned int instance = 0;

	struct sn_parsed_fields fields;

	if (sn_parse(sn, &fields) && fields.instance_len) {

		/* Instance must be expressed as a decimal */
		unsigned int multiplier = 1;

		for (size_t i = 0; i < fields.instance_len; i++) {

			size_t digit_index = fields.instance_pos + fields.instance_len - 1 - i;
			char digit = sn[digit_index];

			if (isdigit((int)digit)) {

			   instance += ((digit - '0') * multiplier);
			   multiplier *= 10;
			}
			else {

				/* Invalid instance string */
				instance = 0;
				break;
			}
		}
	}

	return instance;
}

size_t sn_read_service(const char *sn, char *buf, size_t buf_len)
{
	size_t field_len = 0;

	memset(buf, 0, buf_len);
	struct sn_parsed_fields fields;

	if (sn_parse(sn, &fields) && fields.service_len && (fields.service_len < buf_len)) {

		field_len = fields.service_len;
		memcpy(buf, &sn[fields.service_pos], field_len);
		buf[field_len] = '\0';
	}

	return field_len;
}
