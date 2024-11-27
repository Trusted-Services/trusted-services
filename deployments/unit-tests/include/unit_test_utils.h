/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef UNIT_TEST_UTILS_H
#define UNIT_TEST_UTILS_H

/*
 * Add an expectOneCall() expectation with the defined "chained arguments" and return
 * from the function if variable matches "stage". The macro allows defining all
 * expectations for a test subject function in a single mocking function. The stage
 * enables conditionally disabling expectations to match the point where the test subject
 * will return with an error. This avoids setting unmeet expectations and removes test
 * code duplication.
 *
 * Example usage (so not use apostrophes in the last argument!):
 * 	MOCK_TILL_STAGE(variable, 1, "psa_cipher_encrypt_setup", .ignoreOtherParameters());
 */
#define MOCK_TILL_STAGE(variable, stage, function, chained_args)    \
	do {                                                        \
		if ((variable) >= (stage))                          \
			mock().expectOneCall(function)chained_args; \
	} while (0)

/*
 * Ignores all parameters of the mocked function
 * Example usage:
 * 	MOCK_IGNORE(psa_destroy_key)
 */
#define MOCK_IGNORE(func) mock().expectOneCall(#func).ignoreOtherParameters()

/*
 * Ignores all parameters of the mocked function and expects N calls
 * Example usage:
 * 	MOCK_IGNORE(psa_destroy_key, 5)
 */
#define MOCK_IGNORE_NCALL(func, N) mock().expectNCalls(N, #func).ignoreOtherParameters()

/*
 * Ignores all parameters of the mocked function, but returns the specified value
 * Example usage:
 * 	MOCK_RETVAL_ONLY(psa_destroy_key, PSA_ERROR_GENERIC_ERROR)
 */
#define MOCK_RETVAL_ONLY(func, retval) mock().expectOneCall(#func).andReturnValue(retval).ignoreOtherParameters()

/*
 * Ignores all parameters of the mocked function, except an output parameter
 * Example usage:
 * 	MOCK_OUTPUT_PARAMETER_ONLY(block_store_read, data_len, data_len);
 */
#define MOCK_OUTPUT_PARAMETER_ONLY(func, parameter, variable) mock().expectOneCall(#func).withOutputParameterReturning(#parameter, &variable, sizeof(variable)).ignoreOtherParameters()

/*
 * Ignores all parameters of the mocked function, except an output parameter and returns value
 * Example usage:
 * 	MOCK_OUTPUT_PARAMETER_RETVAL(block_store_write, num_written, num_written, PSA_ERROR_GENERIC_ERROR);
 */
#define MOCK_OUTPUT_PARAMETER_RETVAL(func, parameter, variable, retval) mock().expectOneCall(#func).withOutputParameterReturning(#parameter, &variable, sizeof(variable)).andReturnValue(retval).ignoreOtherParameters()

/*
 * Ignores all parameters of the mocked function, except an unsigned int paramter and returns value
 * Example usage:
 * 	MOCK_UINT_PARAMETER_RETVAL(psa_cipher_encrypt_setup, alg, PSA_ALG_ECB_NO_PADDING, PSA_ERROR_GENERIC_ERROR);
 */
#define MOCK_UINT_PARAMETER_RETVAL(func, parameter, value, retval) mock().expectOneCall(#func).withUnsignedIntParameter(#parameter, value).andReturnValue(retval).ignoreOtherParameters()

#endif /* UNIT_TEST_UTILS_H */
