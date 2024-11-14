/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <string.h>

char *strstr(const char *haystack, const char *needle)
{
	const char *h = NULL;
	size_t needle_len = 0;

	if (needle[0] == '\0')
		return (char *)haystack;

	needle_len = strlen(needle);
	for (h = haystack; (h = strchr(h, needle[0])) != 0; h++)
		if (strncmp(h, needle, needle_len) == 0)
			return (char *)h;

	return NULL;
}
