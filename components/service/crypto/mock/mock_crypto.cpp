/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "mock_crypto.h"

#include <CppUTestExt/MockSupport.h>

void expect_psa_crypto_init(psa_status_t result)
{
	mock().expectOneCall("psa_crypto_init").andReturnValue(result);
}

psa_status_t psa_crypto_init(void)
{
	return mock().actualCall("psa_crypto_init").returnIntValue();
}

void expect_psa_reset_key_attributes(psa_key_attributes_t *attributes)
{
	mock().expectOneCall("psa_reset_key_attributes")
		.withMemoryBufferParameter("attributes", (const uint8_t *)attributes, sizeof(*attributes));
}

void psa_reset_key_attributes(psa_key_attributes_t *attributes)
{
	mock().actualCall("psa_reset_key_attributes").withOutputParameter("attributes", attributes);
}

void expect_psa_destroy_key(psa_key_id_t key, psa_status_t result)
{
	mock().expectOneCall("psa_destroy_key")
		.withUnsignedIntParameter("key", key)
		.andReturnValue(result);
}

psa_status_t psa_destroy_key(psa_key_id_t key)
{
	return mock()
		.actualCall("psa_destroy_key")
		.withUnsignedIntParameter("key", key)
		.returnIntValue();
}

void expect_psa_import_key(const psa_key_attributes_t *attributes, const uint8_t *data,
			   size_t data_length, psa_key_id_t *key, psa_status_t result)
{
	mock().expectOneCall("psa_import_key")
		.withMemoryBufferParameter("attributes", (const uint8_t *)attributes, sizeof(*attributes))
		.withMemoryBufferParameter("data", data, sizeof(*data))
		.withUnsignedIntParameter("data_length", data_length)
		.withOutputParameterReturning("key", key, sizeof(*key))
		.andReturnValue(result);
}

psa_status_t psa_import_key(const psa_key_attributes_t *attributes, const uint8_t *data,
			    size_t data_length, psa_key_id_t *key)
{
	return mock()
		.actualCall("psa_import_key")
		.withMemoryBufferParameter("attributes", (const uint8_t *)attributes, sizeof(*attributes))
		.withMemoryBufferParameter("data", data, data_length)
		.withUnsignedIntParameter("data_length", data_length)
		.withOutputParameter("key", key)
		.returnIntValue();
}

void expect_psa_cipher_encrypt_setup(psa_cipher_operation_t *operation, psa_key_id_t key,
				     psa_algorithm_t alg, psa_status_t result)
{
	mock().expectOneCall("psa_cipher_encrypt_setup")
		.withMemoryBufferParameter("operation", (uint8_t *)operation, sizeof(*operation))
		.withUnsignedIntParameter("key", key)
		.withUnsignedIntParameter("alg", alg)
		.andReturnValue(result);
}

psa_status_t psa_cipher_encrypt_setup(psa_cipher_operation_t *operation, psa_key_id_t key,
				      psa_algorithm_t alg)
{
	return mock()
		.actualCall("psa_cipher_encrypt_setup")
		.withMemoryBufferParameter("operation", (uint8_t *)operation, sizeof(*operation))
		.withUnsignedIntParameter("key", key)
		.withUnsignedIntParameter("alg", alg)
		.returnIntValue();
}

void expect_psa_cipher_decrypt_setup(psa_cipher_operation_t *operation, psa_key_id_t key,
				     psa_algorithm_t alg, psa_status_t result)
{
	mock().expectOneCall("psa_cipher_decrypt_setup")
		.withMemoryBufferParameter("operation", (uint8_t *)operation, sizeof(*operation))
		.withUnsignedIntParameter("key", key)
		.withUnsignedIntParameter("alg", alg)
		.andReturnValue(result);
}

psa_status_t psa_cipher_decrypt_setup(psa_cipher_operation_t *operation, psa_key_id_t key,
				      psa_algorithm_t alg)
{
	return mock()
		.actualCall("psa_cipher_decrypt_setup")
		.withMemoryBufferParameter("operation", (uint8_t *)operation, sizeof(*operation))
		.withUnsignedIntParameter("key", key)
		.withUnsignedIntParameter("alg", alg)
		.returnIntValue();
}

void expect_psa_cipher_set_iv(psa_cipher_operation_t *operation, const uint8_t *iv,
			      size_t iv_length, psa_status_t result)
{
	mock().expectOneCall("psa_cipher_set_iv")
		.withMemoryBufferParameter("operation", (uint8_t *)operation, sizeof(*operation))
		.withMemoryBufferParameter("iv", iv, iv_length)
		.withUnsignedIntParameter("iv_length", iv_length)
		.andReturnValue(result);
}

psa_status_t psa_cipher_set_iv(psa_cipher_operation_t *operation, const uint8_t *iv,
			       size_t iv_length)
{
	return mock()
		.actualCall("psa_cipher_set_iv")
		.withMemoryBufferParameter("operation", (uint8_t *)operation, sizeof(*operation))
		.withMemoryBufferParameter("iv", iv, iv_length)
		.withUnsignedIntParameter("iv_length", iv_length)
		.returnIntValue();
}

void expect_psa_cipher_update(psa_cipher_operation_t *operation, const uint8_t *input,
			      size_t input_length, uint8_t *output, size_t output_size,
			      size_t *output_length, psa_status_t result)
{
	mock().expectOneCall("psa_cipher_update")
		.withMemoryBufferParameter("operation", (uint8_t *)operation, sizeof(*operation))
		.withMemoryBufferParameter("input", input, input_length)
		.withUnsignedIntParameter("input_length", input_length)
		.withMemoryBufferParameter("output", output, *output_length)
		.withUnsignedIntParameter("output_size", output_size)
		.withOutputParameterReturning("output_length", output_length,
					      sizeof(*output_length))
		.andReturnValue(result);
}

psa_status_t psa_cipher_update(psa_cipher_operation_t *operation, const uint8_t *input,
			       size_t input_length, uint8_t *output, size_t output_size,
			       size_t *output_length)
{
	return mock()
		.actualCall("psa_cipher_update")
		.withMemoryBufferParameter("operation", (uint8_t *)operation, sizeof(*operation))
		.withMemoryBufferParameter("input", input, input_length)
		.withUnsignedIntParameter("input_length", input_length)
		.withMemoryBufferParameter("output", output, output_size)
		.withUnsignedIntParameter("output_size", output_size)
		.withOutputParameter("output_length", output_length)
		.returnIntValue();
}

void expect_psa_cipher_finish(psa_cipher_operation_t *operation, uint8_t *output,
			      size_t output_size, size_t *output_length, psa_status_t result)
{
	mock().expectOneCall("psa_cipher_finish")
		.withMemoryBufferParameter("operation", (uint8_t *)operation, sizeof(*operation))
		.withOutputParameterReturning("output", output, sizeof(*output))
		.withUnsignedIntParameter("output_size", output_size)
		.withOutputParameterReturning("output_length", output_length,
					      sizeof(*output_length))
		.andReturnValue(result);
}

psa_status_t psa_cipher_finish(psa_cipher_operation_t *operation, uint8_t *output,
			       size_t output_size, size_t *output_length)
{
	return mock()
		.actualCall("psa_cipher_finish")
		.withMemoryBufferParameter("operation", (uint8_t *)operation, sizeof(*operation))
		.withOutputParameter("output", output)
		.withUnsignedIntParameter("output_size", output_size)
		.withOutputParameter("output_length", output_length)
		.returnIntValue();
}

void expect_psa_cipher_abort(psa_cipher_operation_t *operation, psa_status_t result)
{
	mock().expectOneCall("psa_cipher_abort")
		.withMemoryBufferParameter("operation", (uint8_t *)operation, sizeof(*operation))
		.andReturnValue(result);
}

psa_status_t psa_cipher_abort(psa_cipher_operation_t *operation)
{
	return mock()
		.actualCall("psa_cipher_abort")
		.withMemoryBufferParameter("operation", (uint8_t *)operation, sizeof(*operation))
		.returnIntValue();
}

void expect_psa_key_derivation_setup(psa_key_derivation_operation_t *operation, psa_algorithm_t alg,
				     psa_status_t result)
{
	mock().expectOneCall("psa_key_derivation_setup")
		.withMemoryBufferParameter("operation", (uint8_t *)operation, sizeof(*operation))
		.withUnsignedIntParameter("alg", alg)
		.andReturnValue(result);
}

psa_status_t psa_key_derivation_setup(psa_key_derivation_operation_t *operation,
				      psa_algorithm_t alg)
{
	return mock()
		.actualCall("psa_key_derivation_setup")
		.withMemoryBufferParameter("operation", (uint8_t *)operation, sizeof(*operation))
		.withUnsignedIntParameter("alg", alg)
		.returnIntValue();
}

void expect_psa_key_derivation_input_bytes(psa_key_derivation_operation_t *operation,
					   psa_key_derivation_step_t step, const uint8_t *data,
					   size_t data_length, psa_status_t result)
{
	mock().expectOneCall("psa_key_derivation_input_bytes")
		.withMemoryBufferParameter("operation", (uint8_t *)operation, sizeof(*operation))
		.withUnsignedIntParameter("step", step)
		.withMemoryBufferParameter("data", data, data_length)
		.withUnsignedIntParameter("data_length", data_length)
		.andReturnValue(result);
}

psa_status_t psa_key_derivation_input_bytes(psa_key_derivation_operation_t *operation,
					    psa_key_derivation_step_t step, const uint8_t *data,
					    size_t data_length)
{
	return mock()
		.actualCall("psa_key_derivation_input_bytes")
		.withMemoryBufferParameter("operation", (uint8_t *)operation, sizeof(*operation))
		.withUnsignedIntParameter("step", step)
		.withMemoryBufferParameter("data", data, data_length)
		.withUnsignedIntParameter("data_length", data_length)
		.returnIntValue();
}

void expect_psa_key_derivation_input_key(psa_key_derivation_operation_t *operation,
					 psa_key_derivation_step_t step, psa_key_id_t key,
					 psa_status_t result)
{
	mock().expectOneCall("psa_key_derivation_input_key")
		.withMemoryBufferParameter("operation", (uint8_t *)operation, sizeof(*operation))
		.withUnsignedIntParameter("step", step)
		.withUnsignedIntParameter("key", key)
		.andReturnValue(result);
}

psa_status_t psa_key_derivation_input_key(psa_key_derivation_operation_t *operation,
					  psa_key_derivation_step_t step, psa_key_id_t key)
{
	return mock()
		.actualCall("psa_key_derivation_input_key")
		.withMemoryBufferParameter("operation", (uint8_t *)operation, sizeof(*operation))
		.withUnsignedIntParameter("step", step)
		.withUnsignedIntParameter("key", key)
		.returnIntValue();
}

void expect_psa_key_derivation_output_key(const psa_key_attributes_t *attributes,
					  psa_key_derivation_operation_t *operation,
					  psa_key_id_t *key, psa_status_t result)
{
	mock().expectOneCall("psa_key_derivation_output_key")
		.withMemoryBufferParameter("attributes", (const uint8_t *)attributes, sizeof(*attributes))
		.withMemoryBufferParameter("operation", (uint8_t *)operation, sizeof(*operation))
		.withOutputParameterReturning("key", key, sizeof(*key))
		.andReturnValue(result);
}

psa_status_t psa_key_derivation_output_key(const psa_key_attributes_t *attributes,
					   psa_key_derivation_operation_t *operation,
					   psa_key_id_t *key)
{
	return mock()
		.actualCall("psa_key_derivation_output_key")
		.withMemoryBufferParameter("attributes", (const uint8_t *)attributes, sizeof(*attributes))
		.withMemoryBufferParameter("operation", (uint8_t *)operation, sizeof(*operation))
		.withOutputParameter("key", key)
		.returnIntValue();
}

void expect_psa_key_derivation_abort(psa_key_derivation_operation_t *operation, psa_status_t result)
{
	mock().expectOneCall("psa_key_derivation_abort")
		.withMemoryBufferParameter("operation", (uint8_t *)operation, sizeof(*operation))
		.andReturnValue(result);
}

psa_status_t psa_key_derivation_abort(psa_key_derivation_operation_t *operation)
{
	return mock()
		.actualCall("psa_key_derivation_abort")
		.withMemoryBufferParameter("operation", (uint8_t *)operation, sizeof(*operation))
		.returnIntValue();
}
