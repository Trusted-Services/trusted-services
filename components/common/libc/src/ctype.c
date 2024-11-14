/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <ctype.h>

int isalnum(int c)
{
	return isalpha(c) || isdigit(c);
}

int isalpha(int c)
{
	return isupper(c) || islower(c);
}

int iscntrl(int c)
{
	return c < 0x20 || c == 0x7f;
}

int isdigit(int c)
{
	return c >= '0' && c <= '9';
}

int isgraph(int c)
{
	return c >= 0x21 && c < 0x7f;
}

int islower(int c)
{
	return c >= 'a' && c <= 'z';
}

int isprint(int c)
{
	return c >= 0x20 && c < 0x7f;
}

int ispunct(int c)
{
	return isgraph(c) && !isalnum(c);
}

int isspace(int c)
{
	return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
}

int isupper(int c)
{
	return c >= 'A' && c <= 'Z';
}

int isxdigit(int c)
{
	if (isdigit(c))
		return 1;

	if (c >= 'A' && c <= 'F')
		return 1;

	if (c >= 'a' && c <= 'f')
		return 1;

	return 0;
}

int tolower(int c)
{
	if (c >= 'A' && c <= 'Z')
		return c - 'A' + 'a';

	return c;
}

int toupper(int c)
{
	if (c >= 'a' && c <= 'z')
		return c - 'a' + 'A';

	return c;
}
