/*
 * Copyright (c) 2020-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ts-demo.h"

#include <psa/crypto.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const psa_key_id_t SIGNING_KEY_ID = 0x100;
static const psa_key_id_t ENCRYPTION_KEY_ID = 0x101;

bool m_verbose = true;

void print_intro(void)
{
	if (m_verbose) {
		printf("\nDemonstrates use of trusted services from an application");
		printf("\n---------------------------------------------------------");
		printf("\nA client requests a set of crypto operations performed by");
		printf("\nthe Crypto service.  Key storage for persistent keys is");
		printf("\nprovided by the Secure Storage service via the ITS client.\n");
		printf("\n");
	}
}

void wait(int seconds)
{
	if (m_verbose)
		sleep(seconds);
}

void print_status(psa_status_t status)
{
	if (m_verbose) {
		if (status == PSA_SUCCESS)
			printf("\n\tOperation successful\n");
		else
			printf("\n\tOperation failed. op error: %d\n", status);
	}
}

void print_byte_array(const uint8_t *array, size_t len)
{
	size_t count = 0;
	size_t column = 0;

	while (count < len) {
		if (column == 0)
			printf("\n\t\t");
		else
			printf(" ");

		printf("%02X", array[count]);

		++count;
		column = (column + 1) % 8;
	}

	printf("\n");
}

bool generate_signing_key(psa_key_id_t *signing_key_id)
{
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_id(&attributes, SIGNING_KEY_ID);
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH);
	psa_set_key_algorithm(&attributes, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&attributes, 256);

	if (m_verbose)
		printf("Generating ECC signing key");

	status = psa_generate_key(&attributes, signing_key_id);
	psa_reset_key_attributes(&attributes);

	print_status(status);

	return (status != PSA_SUCCESS);
}

bool sign_and_verify_message(const char *message, psa_key_id_t signing_key_id)
{
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;
	bool error = false;
	uint8_t message_buffer[100];
	size_t message_len = strlen(message) + 1;

	if (message_len > sizeof(message_buffer))
		message_len = sizeof(message_buffer) - 1;

	memset(message_buffer, 0, sizeof(message_buffer));
	memcpy(message_buffer, message, message_len);

	/* Sign message */
	uint8_t signature[PSA_SIGNATURE_MAX_SIZE];
	size_t signature_length;

	if (m_verbose)
		printf("Signing message: \"%s\" using key: %d", message_buffer, signing_key_id);

	status = psa_sign_hash(signing_key_id, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256),
			       message_buffer, message_len, signature, sizeof(signature),
			       &signature_length);

	print_status(status);

	if (m_verbose && (status == PSA_SUCCESS)) {
		printf("\tSignature bytes: ");
		print_byte_array(signature, signature_length);
	}

	error |= (status != PSA_SUCCESS);

	/* Verify signature against original message */
	if (m_verbose)
		printf("Verify signature using original message: \"%s\"", message_buffer);

	status = psa_verify_hash(signing_key_id, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256),
				 message_buffer, message_len, signature, signature_length);

	print_status(status);

	error |= (status != PSA_SUCCESS);

	/* Verify signature against modified message */
	message_buffer[0] = '!';
	if (m_verbose)
		printf("Verify signature using modified message: \"%s\"", message_buffer);

	status = psa_verify_hash(signing_key_id, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256),
				 message_buffer, message_len, signature, signature_length);

	if (status == PSA_ERROR_INVALID_SIGNATURE) {
		if (m_verbose)
			printf("\n\tSuccessfully detected modified message\n");
	} else {
		print_status(status);
	}

	error |= (status == PSA_SUCCESS);

	return error;
}

bool generate_asymmetric_encryption_key(psa_key_id_t *encryption_key_id)
{
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_id(&attributes, ENCRYPTION_KEY_ID);
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attributes, PSA_ALG_RSA_PKCS1V15_CRYPT);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_RSA_KEY_PAIR);
	psa_set_key_bits(&attributes, 1024);

	if (m_verbose)
		printf("Generating RSA encryption key");

	status = psa_generate_key(&attributes, encryption_key_id);
	psa_reset_key_attributes(&attributes);

	print_status(status);

	return (status != PSA_SUCCESS);
}

bool encrypt_add_decrypt_message(const char *message, psa_key_id_t encryption_key_id)
{
	bool error = false;
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;
	size_t message_len = strlen(message) + 1;

	/* Encrypt a message */
	if (m_verbose)
		printf("Encrypt message: \"%s\" using RSA key: %d", message, encryption_key_id);

	uint8_t ciphertext[256];
	size_t ciphertext_len = 0;

	status = psa_asymmetric_encrypt(encryption_key_id, PSA_ALG_RSA_PKCS1V15_CRYPT,
					(const uint8_t *)message, message_len, NULL, 0, ciphertext,
					sizeof(ciphertext), &ciphertext_len);
	print_status(status);

	if (m_verbose && (status == PSA_SUCCESS)) {
		printf("\tEncrypted message: ");
		print_byte_array(ciphertext, ciphertext_len);
	}

	error |= (status != PSA_SUCCESS);

	/* Decrypt it */
	if (m_verbose)
		printf("Decrypting message using RSA key: %d", encryption_key_id);

	uint8_t plaintext[256];
	size_t plaintext_len = 0;

	status = psa_asymmetric_decrypt(encryption_key_id, PSA_ALG_RSA_PKCS1V15_CRYPT, ciphertext,
					ciphertext_len, NULL, 0, plaintext, sizeof(plaintext),
					&plaintext_len);
	print_status(status);

	if (m_verbose && (status == PSA_SUCCESS)) {
		if ((plaintext_len == message_len) &&
		    (memcmp(message, plaintext, plaintext_len) == 0)) {
			printf("\tDecrypted message: \"%s\"\n", plaintext);
		} else {
			printf("\tDecrypted message is different from original!: ");
			print_byte_array(plaintext, plaintext_len);
		}
	}

	error |= (status != PSA_SUCCESS);

	return error;
}

bool export_public_key(psa_key_id_t signing_key_id)
{
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;
	uint8_t key_buf[PSA_EXPORT_PUBLIC_KEY_MAX_SIZE];
	size_t key_len = 0;

	if (m_verbose)
		printf("Exporting public key: %d", signing_key_id);

	status = psa_export_public_key(signing_key_id, key_buf, sizeof(key_buf), &key_len);

	print_status(status);

	if (m_verbose && (status == PSA_SUCCESS)) {
		printf("\tPublic key bytes: ");
		print_byte_array(key_buf, key_len);
	}

	return (status != PSA_SUCCESS);
}

bool generate_random_number(size_t length)
{
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;
	uint8_t *buffer = (uint8_t *)malloc(length);

	if (m_verbose)
		printf("Generating random bytes length: %lu", length);

	status = psa_generate_random(buffer, length);

	print_status(status);

	if (m_verbose && (status == PSA_SUCCESS)) {
		printf("\tRandom bytes: ");
		print_byte_array(buffer, length);
	}

	return (status != PSA_SUCCESS);
}

bool destroy_keys(psa_key_id_t signing_key_id, psa_key_id_t encryption_key_id)
{
	bool error = false;
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;

	if (m_verbose)
		printf("Destroying signing key: %d", signing_key_id);

	status = psa_destroy_key(signing_key_id);
	print_status(status);
	error |= (status != PSA_SUCCESS);

	if (m_verbose)
		printf("Destroying encryption key: %d", encryption_key_id);

	status = psa_destroy_key(encryption_key_id);
	print_status(status);
	error |= (status != PSA_SUCCESS);

	return error;
}

bool run_ts_demo(bool is_verbose)
{
	psa_key_id_t signing_key_id = 0;
	psa_key_id_t encryption_key_id = 0;
	bool error = false;

	m_verbose = is_verbose;
	print_intro();
	wait(1);
	psa_crypto_init();
	wait(1);
	error |= generate_random_number(1);
	wait(1);
	error |= generate_random_number(7);
	wait(1);
	error |= generate_random_number(128);
	wait(1);
	error |= generate_signing_key(&signing_key_id);
	wait(2);
	error |= sign_and_verify_message("The quick brown fox", signing_key_id);
	wait(3);
	error |= sign_and_verify_message("jumps over the lazy dog", signing_key_id);
	wait(3);
	error |= generate_asymmetric_encryption_key(&encryption_key_id);
	wait(2);
	error |= encrypt_add_decrypt_message("Top secret", encryption_key_id);
	wait(3);
	error |= export_public_key(signing_key_id);
	wait(2);
	error |= destroy_keys(signing_key_id, encryption_key_id);
	wait(2);

	return error;
}
