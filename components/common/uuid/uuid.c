/*
 * Copyright (c) 2020-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "uuid.h"
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <compiler.h>

static uint8_t hex_to_nibble(char hex)
{
	uint8_t nibble = 0;

	if (hex >= '0' && hex <= '9') {
		nibble = hex - '0';
	}
	else {
		nibble = ((hex | 0x20) - 'a') + 10;
	}

	return nibble;
}

static char nibble_to_hex(uint8_t nibble)
{
	char hex;

	nibble &= 0x0f;

	if (nibble <= 9)
		hex = '0' + nibble;
	else
		hex = 'a' + nibble - 10;

	return hex;
}

static uint8_t hex_to_byte(const char *hex)
{
	/* Takes a validated input and returns the byte value */
	uint8_t byte = hex_to_nibble(hex[0]) << 4;
	byte |= (hex_to_nibble(hex[1]) & 0x0f);
	return byte;
}

size_t uuid_is_valid(const char *canonical_form)
{
	size_t valid_chars = 0;

	/* Note that a valid canonical uuid may be part of a longer string
	 * such as a urn.
	 */
	if (!memchr(canonical_form, '\0', UUID_CANONICAL_FORM_LEN)) {
		size_t i;
		valid_chars = UUID_CANONICAL_FORM_LEN;

		for (i = 0; i < UUID_CANONICAL_FORM_LEN; ++i) {

			if (i == 8 || i == 13 || i == 18 || i == 23) {
				if (canonical_form[i] != '-') return 0;
			}
			else {
				if (!isxdigit((int)canonical_form[i])) return 0;
			}
		}
	}

	return valid_chars;
}

bool uuid_is_equal(const uint8_t *octets_a, const uint8_t *octets_b)
{
	return memcmp(octets_a, octets_b, UUID_OCTETS_LEN) == 0;
}

bool uuid_is_nil(const uint8_t *octets)
{
	return memcmp(uuid_get_nil()->octets, octets, UUID_OCTETS_LEN) == 0;
}

const struct uuid_octets *uuid_get_nil(void)
{
	static const struct uuid_octets nil_uuid = {0};

	return &nil_uuid;
}

size_t uuid_parse_to_octets(const char *canonical_form, uint8_t *buf, size_t buf_size)
{
	size_t octet_index = 0;
	const char *pos;
	size_t valid_chars = uuid_is_valid(canonical_form);

	if ((buf_size < UUID_OCTETS_LEN) ||
		(valid_chars != UUID_CANONICAL_FORM_LEN)) {
		/* Invalid input */
		return 0;
	}

	/*
	 * UUID string has been validates as having the following form:
	 * xxxxxxxx-xxxx-Mxxx-Nxxx-xxxxxxxxxxxx
	 *     4      2    2   2       6
	 */
	pos = &canonical_form[0];
	while (octet_index < 4) {
		buf[octet_index++] = hex_to_byte(pos);
		pos += 2;
	}

	pos = &canonical_form[9];
	while (octet_index < 6) {
		buf[octet_index++] = hex_to_byte(pos);
		pos += 2;
	}

	pos = &canonical_form[14];
	while (octet_index < 8) {
		buf[octet_index++] = hex_to_byte(pos);
		pos += 2;
	}

	pos = &canonical_form[19];
	while (octet_index < 10) {
		buf[octet_index++] = hex_to_byte(pos);
		pos += 2;
	}

	pos = &canonical_form[24];
	while (octet_index < 16) {
		buf[octet_index++] = hex_to_byte(pos);
		pos += 2;
	}

	return valid_chars;
}

/*
 * The byte order is reversed for the integer sections of the UUID. Converts
 * from standard to GUID octet representations an visa versa.
 */
void uuid_reverse_octets(const struct uuid_octets *input_octets,
	uint8_t *buf, size_t buf_size)
{
	if (buf_size >= UUID_OCTETS_LEN) {
		/* Reverse bytes in each section */
		buf[0] = input_octets->octets[3];
		buf[1] = input_octets->octets[2];
		buf[2] = input_octets->octets[1];
		buf[3] = input_octets->octets[0];

		buf[4] = input_octets->octets[5];
		buf[5] = input_octets->octets[4];

		buf[6] = input_octets->octets[7];
		buf[7] = input_octets->octets[6];

		buf[8] = input_octets->octets[8];
		buf[9] = input_octets->octets[9];

		buf[10] = input_octets->octets[10];
		buf[11] = input_octets->octets[11];
		buf[12] = input_octets->octets[12];
		buf[13] = input_octets->octets[13];
		buf[14] = input_octets->octets[14];
		buf[15] = input_octets->octets[15];
	}
}

size_t uuid_parse_to_guid_octets(const char *canonical_form,
	uint8_t *buf, size_t buf_size)
{
	size_t valid_chars;
	struct uuid_octets standard_encoding;

	valid_chars = uuid_parse_to_octets(canonical_form,
		standard_encoding.octets, sizeof(standard_encoding.octets));

	if (valid_chars == UUID_CANONICAL_FORM_LEN) {

		uuid_reverse_octets(&standard_encoding, buf, buf_size);
	}

	return valid_chars;
}

void uuid_octets_from_canonical(struct uuid_octets *uuid_octets,
	const char *canonical_form)
{
	__maybe_unused size_t valid_chars = uuid_parse_to_octets(canonical_form,
		uuid_octets->octets, sizeof(uuid_octets->octets));

	/* Input string is assumed to be valid. Should not be used if canonical
	 * string originates from an untrusted source.
	 */
	assert(valid_chars == UUID_CANONICAL_FORM_LEN);
}

void uuid_guid_octets_from_canonical(struct uuid_octets *uuid_octets,
	const char *canonical_form)
{
	__maybe_unused size_t valid_chars = uuid_parse_to_guid_octets(canonical_form,
		uuid_octets->octets, sizeof(uuid_octets->octets));

	assert(valid_chars == UUID_CANONICAL_FORM_LEN);
}

void uuid_canonical_from_octets(struct uuid_canonical *canonical_form,
	const struct uuid_octets *uuid_octets)
{
	unsigned int octet_index = 0;
	unsigned int char_index = 0;

	while (octet_index < UUID_OCTETS_LEN) {

		canonical_form->characters[char_index++] =
			nibble_to_hex(uuid_octets->octets[octet_index] >> 4);

		canonical_form->characters[char_index++] =
			nibble_to_hex(uuid_octets->octets[octet_index] & 0x0f);

		++octet_index;

		if ((octet_index == 4) ||
			(octet_index == 6) ||
			(octet_index == 8) ||
			(octet_index == 10))
			canonical_form->characters[char_index++] = '-';
	}

	canonical_form->characters[char_index] = '\0';
}

void uuid_canonical_from_guid_octets(struct uuid_canonical *canonical_form,
	const struct uuid_octets *uuid_octets)
{
	struct uuid_octets reversed_octets;

	uuid_reverse_octets(uuid_octets, reversed_octets.octets, sizeof(reversed_octets.octets));
	uuid_canonical_from_octets(canonical_form, &reversed_octets);
}
