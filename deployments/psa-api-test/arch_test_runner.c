/*
 * Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <ctype.h>
#include <rpc_caller.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "psa_api_test_common.h"
#include "trace.h"

#define TEST_ID_OFFSET	    (5)
#define TEST_POSTFIX_OFFSET (8)
#define TEST_ENTRY_LENGTH   (9)

int32_t val_entry(void);
size_t val_get_test_list(uint32_t *test_id_list, size_t size);
void pal_set_custom_test_list(char *custom_test_list);

/* Returns whether option_switch is in the argv list and provide its index in the array */
static bool option_selected(const char *option_switch, int argc, char *argv[], int *index)
{
	bool selected = false;
	*index = 0;

	for (int i = 1; (i < argc) && !selected; ++i) {
		selected = (strcmp(argv[i], option_switch) == 0);
		*index = i;
	}

	return selected;
}

/* Print the supported command line arguments */
static void print_help(void)
{
	EMSG("Supported command line arguments:\n");
	EMSG("\t -l: Print list of tests.");
	EMSG("\t -t <test_list>: Run only the listed tests (e.g: test_201;test_202;). test_list = ^(test_[0-9]{3};)+");
	EMSG("\t -c: <service name>: Crypto service name");
	EMSG("\t -a: <service name>: Initial attestation service name");
	EMSG("\t -p: <service name>: Protected Storage service name");
	EMSG("\t -i: <service name>: Internal Trusted Storage service name");
	EMSG("\t -v: Verbose mode.");
	EMSG("\t -h: Print this help message.\n");
}

/* Prints the list of selectable psa-api tests */
static void print_psa_api_tests(void)
{
	/*
	 * Request the number of tests to find out the size
	 * of the area needed to store the test ID-s.
	 */
	size_t n_test = val_get_test_list(NULL, 0);

	uint32_t *test_id_list = (uint32_t *)calloc(n_test, sizeof(uint32_t));

	if (test_id_list) {
		n_test = val_get_test_list(test_id_list, n_test);

		EMSG("Available psa-api tests:");
		for (int i = 0; i < n_test; i++)
			EMSG("\t test_%d;", test_id_list[i]);

		free(test_id_list);
	} else {
		EMSG("Could not allocate enough memory to store the list of tests");
	}
}

/* Check if the received test list string is formatted as expected */
static bool is_test_list_wrong(char *test_list)
{
	size_t len = strlen(test_list);

	for (unsigned int i = 0; i < len; i += TEST_ENTRY_LENGTH) {
		/* Report error when the test entry is not properly finished */
		if (i + TEST_ENTRY_LENGTH > len) {
			EMSG("Expecting \"test_xxx;\" test entry at the %dth character, got \"%s\" instead.",
			     i, &test_list[i]);
			return true;
		}

		/* Report error at incorrect test entry prefix */
		if (memcmp(&test_list[i], "test_", TEST_ID_OFFSET)) {
			EMSG("Expecting \"test_\" at the %dth character, got \"%.5s\" instead.", i,
			     &test_list[i]);
			return true;
		}

		/* Report error if the test ID is incorrect */
		if (!(isdigit(test_list[i + TEST_ID_OFFSET]) &&
		      isdigit(test_list[i + TEST_ID_OFFSET + 1]) &&
		      isdigit(test_list[i + TEST_ID_OFFSET + 2]))) {
			EMSG("Expecting three digits at %dth character, got \"%.3s\" instead.",
			     i + TEST_ID_OFFSET, &test_list[i + TEST_ID_OFFSET]);
			return true;
		}

		/* Report error at incorrect test entry postfix */
		if (test_list[i + TEST_POSTFIX_OFFSET] != ';') {
			EMSG("Expecting ; at the %dth character, got \"%.1s\" instead.",
			     i + TEST_POSTFIX_OFFSET, &test_list[i + TEST_POSTFIX_OFFSET]);
			return true;
		}
	}

	return false;
}

/* Entry point */
int main(int argc, char *argv[])
{
	int rval = -1;
	int option_index = 0;
	char *service_name_crypto = NULL;
	char *service_name_iat = NULL;
	char *service_name_ps = NULL;
	char *service_name_its = NULL;

	/* Print available tests */
	if (option_selected("-l", argc, argv, &option_index)) {
		print_psa_api_tests();
		return 0;
	}

	/* Create custom test list */
	if (option_selected("-t", argc, argv, &option_index)) {
		/*
		 * Avoid overindexing of argv and detect if the
		 * option is followed by another option
		 */
		char *test_list_values = argv[option_index + 1];

		if ((option_index >= argc) || (test_list_values[0] == '-')) {
			EMSG("Testlist string is expected after -t argument!");
			return -1;
		}

		if (is_test_list_wrong(test_list_values)) {
			EMSG("Testlist string is not valid!");
			print_psa_api_tests();
			return -1;
		}

		/* Filter tests */
		pal_set_custom_test_list(test_list_values);
	}

	/* Print help */
	if (option_selected("-h", argc, argv, &option_index)) {
		print_help();
		return 0;
	}

	/* Process optional service names */
	if (option_selected("-c", argc, argv, &option_index))
		service_name_crypto = argv[option_index + 1];

	if (option_selected("-a", argc, argv, &option_index))
		service_name_iat = argv[option_index + 1];

	if (option_selected("-p", argc, argv, &option_index))
		service_name_ps = argv[option_index + 1];

	if (option_selected("-i", argc, argv, &option_index))
		service_name_its = argv[option_index + 1];

	rval = test_setup(service_name_crypto, service_name_iat, service_name_ps, service_name_its);

	if (!rval)
		rval = val_entry();

	test_teardown();

	return rval;
}
