/*
 * Copyright (c) 2020-2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CPPUTEST_TEST_RUNNER_H
#define CPPUTEST_TEST_RUNNER_H

#include <protocols/service/test_runner/packed-c/test_result.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Constants
 */
#define TEST_NAME_MAX_LEN		(30)
#define TEST_GROUP_MAX_LEN		(30)

/**
 * Specifies a set of tests for running or listing.
 */
struct test_spec
{
	char name[TEST_NAME_MAX_LEN];
	char group[TEST_GROUP_MAX_LEN];
};

/**
 * A summary of a set of tests qualified by a test_spec.
 */
struct test_summary
{
	int num_tests;		/* Number of qualifying tests */
	int num_results;	/* Number of available test result objects */
	int num_passed;		/* Number that ran and passed */
	int num_failed;		/* Number that ran and failed */
};

/**
 * The run state of a test case
 */
enum test_run_state
{
	TEST_RUN_STATE_NOT_RUN = TS_TEST_RUNNER_TEST_RESULT_RUN_STATE_NOT_RUN,
	TEST_RUN_STATE_PASSED = TS_TEST_RUNNER_TEST_RESULT_RUN_STATE_PASSED,
	TEST_RUN_STATE_FAILED = TS_TEST_RUNNER_TEST_RESULT_RUN_STATE_FAILED
};

/**
 * The result for a particular test case.
 */
struct test_result
{
	char name[TEST_NAME_MAX_LEN];
	char group[TEST_GROUP_MAX_LEN];
	enum test_run_state run_state;
	unsigned int fail_line;
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CPPUTEST_TEST_RUNNER_H */
