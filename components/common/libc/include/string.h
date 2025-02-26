/*
 * Copyright (c) 2012-2017 Roberto E. Vargas Caballero
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 * Portions copyright (c) 2018-2024, Arm Limited and Contributors.
 * Portions copyright (c) 2023, Intel Corporation. All rights reserved.
 * All rights reserved.
 */

#ifndef STRING_H
#define STRING_H

#include <stddef.h>

void *memcpy(void *dst, const void *src, size_t len);
int memcpy_s(void *dst, size_t dsize, void *src, size_t ssize);
void *memmove(void *dst, const void *src, size_t len);
int memcmp(const void *s1, const void *s2, size_t len);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
void *memchr(const void *src, int c, size_t len);
void *memrchr(const void *src, int c, size_t len);
char *strchr(const char *s, int c);
void *memset(void *dst, int val, size_t count);
size_t strlen(const char *s);
size_t strnlen(const char *s, size_t maxlen);
char *strrchr(const char *p, int ch);
size_t strlcpy(char * dst, const char * src, size_t dsize);
size_t strlcat(char * dst, const char * src, size_t dsize);
char *strtok_r(char *s, const char *delim, char **last);
char *strstr(const char *haystack, const char *needle);

#endif /* STRING_H */
