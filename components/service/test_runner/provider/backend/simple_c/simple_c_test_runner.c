/*
 * Copyright (c) 2021-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <service/test_runner/provider/test_runner_backend.h>
#include <service/test_runner/provider/test_runner_provider.h>
#include "simple_c_test_runner.h"
#include <string.h>


/* Private defines */
#define SIMPLE_C_TEST_GROUP_LIMIT       (50)

/**
 * The simple_c test runner specialises the base test_runner_backend
 * to add a regsitry of registered test groups.
 */
static struct simple_c_test_runner
{
    struct test_runner_backend base_backend;
    size_t num_groups;
    const struct simple_c_test_group *groups[SIMPLE_C_TEST_GROUP_LIMIT];
} the_test_runner;

/* Concrete test_runner_backed functions */
static size_t count_tests(const struct test_spec *spec);
static int run_tests(const struct test_spec *spec,
        struct test_summary *summary, struct test_result *results, size_t result_limit);
static void list_tests(const struct test_spec *spec,
		struct test_summary *summary, struct test_result *results, size_t result_limit);


void simple_c_test_runner_init(struct test_runner_provider *frontend)
{
    /* Initialise base test_runner_backend */
    the_test_runner.base_backend.count_tests = count_tests;
    the_test_runner.base_backend.run_tests = run_tests;
    the_test_runner.base_backend.list_tests = list_tests;
    the_test_runner.base_backend.next = NULL;

    /* Registry initially empty */
    the_test_runner.num_groups = 0;

    test_runner_provider_register_backend(frontend, &the_test_runner.base_backend);
}

void simple_c_test_runner_register_group(const struct simple_c_test_group *test_group)
{
    if (the_test_runner.num_groups < SIMPLE_C_TEST_GROUP_LIMIT) {

        the_test_runner.groups[the_test_runner.num_groups] = test_group;
        ++the_test_runner.num_groups;
    }
}

static bool does_qualify(const char *spec_string, const char *test_string)
{
    return ((strlen(spec_string) == 0) || (strcmp(spec_string, test_string) == 0));
}

static int test_iterate(const struct test_spec *spec, bool list_only,
        struct test_summary *summary, struct test_result *results, size_t result_limit)
{
    summary->num_tests = 0;
	summary->num_results = 0;
	summary->num_passed = 0;
	summary->num_failed = 0;

    for (size_t group_index = 0; group_index < the_test_runner.num_groups; ++group_index) {

        const struct simple_c_test_group *test_group = the_test_runner.groups[group_index];

        if (does_qualify(spec->group, test_group->group)) {

            for (size_t test_index = 0; test_index < test_group->num_test_cases; ++test_index) {

                const struct simple_c_test_case *test_case = &test_group->test_cases[test_index];

                if (does_qualify(spec->name, test_case->name)) {

                    enum test_run_state run_state = TEST_RUN_STATE_NOT_RUN;
                    struct test_failure failure = {0};

                    /* Run the qualifying test case if we're not just listing tests */
                    if (!list_only) {

                        if (test_case->test_func(&failure)) {

                            run_state = TEST_RUN_STATE_PASSED;
                            ++summary->num_passed;
                        }
                        else {

                            run_state = TEST_RUN_STATE_FAILED;
                            ++summary->num_failed;
                        }
                    }

                    /* Update result object if capacity - common for listing and running tests */
                    if (summary->num_tests < result_limit) {

                        struct test_result *new_result = &results[summary->num_results];
			size_t len = 0;

                        new_result->run_state = run_state;
                        new_result->failure = failure;

			len = strlen(test_group->group) + 1;
			if (len > sizeof(new_result->group))
				return -1;

			memcpy(new_result->group, test_group->group, len);

			len = strlen(test_case->name) + 1;
			if (len > sizeof(new_result->name))
				return -1;

			memcpy(new_result->name, test_case->name, len);

                        ++summary->num_results;
                    }

                    ++summary->num_tests;
                }
            }
        }
    }

    return 0;
}

static size_t count_tests(const struct test_spec *spec)
{
    size_t count = 0;

    for (size_t group_index = 0; group_index < the_test_runner.num_groups; ++group_index) {

        count += the_test_runner.groups[group_index]->num_test_cases;
    }

    return count;
}

static int run_tests(const struct test_spec *spec,
        struct test_summary *summary, struct test_result *results, size_t result_limit)
{
    return test_iterate(spec, false, summary, results, result_limit);
}

static void list_tests(const struct test_spec *spec,
        struct test_summary *summary, struct test_result *results, size_t result_limit)
{
    test_iterate(spec, true, summary, results, result_limit);
}
