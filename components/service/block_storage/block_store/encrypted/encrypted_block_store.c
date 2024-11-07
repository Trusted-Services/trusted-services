/*
 * Copyright (c) 2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "encrypted_block_store.h"

#include <psa/crypto.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

#include "trace.h"

/* Fixed by AES standard */
#define BLK_AES_BLOCK_SIZE (16)

#ifndef BLOCK_ENCRYPTION_ROOT_KEY
#define BLOCK_ENCRYPTION_ROOT_KEY { 0x32, 0x2b, 0x78, 0x27, 0xa3, 0x08, 0xcb, \
				    0x5e, 0xb4, 0x12, 0x0b, 0xab, 0x96, 0xd4, \
				    0x3d, 0x4e, 0x7b, 0xc4, 0x46, 0x46, 0xad, \
				    0x93, 0xe9, 0x03, 0x28, 0x47, 0xe8, 0xb6, \
				    0x2c, 0xec, 0x5f, 0x14 \
				  }
#endif

#define ESSIV_KEY_INFO {'E', 'S', 'S', 'I', 'V'}
#define DECRYPTION_KEY_INFO {'D', 'E', 'C', 'R', 'Y', 'P', 'T'}

#ifndef BLOCK_ENCRYPTION_SALT
#define BLOCK_ENCRYPTION_SALT { 0xcf, 0x9e, 0x66, 0xf1, \
				0x96, 0x91, 0x2d, 0x78, \
				0x50, 0xa7, 0x18, 0x32, \
				0x1f, 0x48, 0x50, 0x94  \
			      }
#endif

#ifndef BLK_AES_KEY_BITS
#define BLK_AES_KEY_BITS (256)
#endif

static const uint8_t block_encryption_root_key[] = BLOCK_ENCRYPTION_ROOT_KEY;
static const uint8_t block_encryption_salt[] = BLOCK_ENCRYPTION_SALT;

_Static_assert(sizeof(block_encryption_root_key) == (BLK_AES_KEY_BITS / 8),
	       "Block Encryption Root Key must be the same width as BLK_AES_KEY_BITS!");

_Static_assert(sizeof(block_encryption_salt) >= 16,
	       "Block Encryption salt must be at least 128 bit long!");

_Static_assert(BLK_AES_KEY_BITS == 128 || BLK_AES_KEY_BITS == 192 || BLK_AES_KEY_BITS == 256,
	       "Invalid BLK_AES_KEY_BITS value!");

_Static_assert(((BLK_AES_BLOCK_SIZE) % (sizeof(uint64_t))) == 0,
       "This code requires that BLK_AES_BLOCK_SIZE is the exact multiple of size of uint64_t");

_Static_assert(offsetof(struct encrypted_block_store, base_block_store) == 0,
	       "base_block_store must be the first element of struct encrypted_block_store!");

static void clear_block_buffers(void *context)
{
	const struct encrypted_block_store *encrypted_block_store =
		(struct encrypted_block_store *)context;

	if (encrypted_block_store->block_buffer_A)
		memset(encrypted_block_store->block_buffer_A, 0,
		       encrypted_block_store->back_store_info.block_size);
	if (encrypted_block_store->block_buffer_B)
		memset(encrypted_block_store->block_buffer_B, 0,
		       encrypted_block_store->back_store_info.block_size);
}

static psa_status_t import_root_key_to_keystore(psa_key_handle_t *root_key_handle)
{
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;
	psa_key_attributes_t root_key_attributes = PSA_KEY_ATTRIBUTES_INIT;

	psa_set_key_lifetime(&root_key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_usage_flags(&root_key_attributes, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&root_key_attributes, PSA_ALG_HKDF(PSA_ALG_SHA_256));
	psa_set_key_type(&root_key_attributes, PSA_KEY_TYPE_DERIVE);
	psa_set_key_bits(&root_key_attributes, sizeof(block_encryption_root_key) * 8);

	status = psa_import_key(&root_key_attributes, block_encryption_root_key,
				sizeof(block_encryption_root_key), root_key_handle);

	psa_reset_key_attributes(&root_key_attributes);

	return status;
}

static psa_status_t remove_root_key_from_keystore(psa_key_handle_t root_key_handle)
{
	return psa_destroy_key(root_key_handle);
}

static psa_status_t derive_and_store_aes_key(psa_key_handle_t parent_key, psa_key_id_t *key_id,
					     psa_algorithm_t key_algorithm, const uint8_t *info,
					     size_t info_length, const uint8_t *salt,
					     size_t salt_length)
{
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;
	psa_key_attributes_t derived_key_attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_derivation_operation_t derivation = psa_key_derivation_operation_init();

	/* Set up the key derivation operation */
	status = psa_key_derivation_setup(&derivation, PSA_ALG_HKDF(PSA_ALG_SHA_256));
	if (status != PSA_SUCCESS) {
		EMSG("Key derivation setup failed with status %d\n", status);
		goto cleanup;
	}

	/* Set salt value for key derivation. Must be called before inputting the secret. */
	status = psa_key_derivation_input_bytes(&derivation, PSA_KEY_DERIVATION_INPUT_SALT, salt,
						salt_length);
	if (status != PSA_SUCCESS) {
		EMSG("Setting input of derivation failed with status %d\n", status);
		goto cleanup;
	}

	/* Provide the parent key as the input key material for HKDF */
	status = psa_key_derivation_input_key(&derivation, PSA_KEY_DERIVATION_INPUT_SECRET,
					      parent_key);
	if (status != PSA_SUCCESS) {
		EMSG("Setting parent key for derivation failed with status %d\n", status);
		goto cleanup;
	}

	/* Provide the info parameter for key derivation. */
	status = psa_key_derivation_input_bytes(&derivation, PSA_KEY_DERIVATION_INPUT_INFO, info,
						info_length);
	if (status != PSA_SUCCESS) {
		EMSG("Setting input of derivation failed with status %d\n", status);
		goto cleanup;
	}

	/* Define attributes for the derived key that will be stored */
	psa_set_key_lifetime(&derived_key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_usage_flags(&derived_key_attributes,
				PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&derived_key_attributes, key_algorithm);
	psa_set_key_type(&derived_key_attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&derived_key_attributes, BLK_AES_KEY_BITS);

	/* Output the derived key directly to the keystore */
	status = psa_key_derivation_output_key(&derived_key_attributes, &derivation, key_id);
	if (status != PSA_SUCCESS) {
		EMSG("Storing the derived key failed with status %d\n", status);
		goto cleanup;
	}

cleanup:
	(void)psa_key_derivation_abort(&derivation);
	psa_reset_key_attributes(&derived_key_attributes);

	return status;
}

/*
 * Encrypt the sector number with the hash of the encryption key to calculate ESSIV
 * (Encrypted Salt-Sector Initialization Vector) using AES-ECB algorithm.
 * To mitigate attacks that are based on IV prediction (like watermarking attack)
 * this algorithm generates a unique, unpredictable vector for each sector by using
 * the unique sector number and the hash of a key.
 */
static psa_status_t calculate_essiv(uint64_t lba, psa_key_id_t essiv_key, uint8_t *iv,
				    size_t iv_len)
{
	/*
	 * To calculate Encrypted Salt-Sector Initialization Vector (ESSIV) first a
	 * sector-dependent, unique, deterministic initialization vector is created.
	 * Then this vector is encrypted (AES-ECB in our case) with the essiv key.
	 */
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;

	/* 1 AES_BLOCK_SIZE is enough to store LBA */
	uint64_t sector_specific_data[BLK_AES_BLOCK_SIZE / sizeof(uint64_t)] = { 0 };
	psa_cipher_operation_t cipher_operation = PSA_CIPHER_OPERATION_INIT;
	size_t output_len = 0;
	size_t finish_output_len = 0;

	/*
	 * LCOV_EXCL_START
	 *
	 * This function is called only from this file with proper iv_len.
	 * Although this check is not reachable now, it is left here for
	 * safety reasons.
	 */
	if (iv_len < sizeof(sector_specific_data))
		return PSA_ERROR_BUFFER_TOO_SMALL;
	/* LCOV_EXCL_STOP */

	/* Calculate a deterministic, sector-specific, unique vector */
	sector_specific_data[0] = lba;

	/* Encrypt the sector specific data with the essiv key */
	status = psa_cipher_encrypt_setup(&cipher_operation, essiv_key, PSA_ALG_ECB_NO_PADDING);
	if (status != PSA_SUCCESS) {
		return status;
	}

	status = psa_cipher_update(&cipher_operation, (uint8_t *)sector_specific_data,
				   sizeof(sector_specific_data), iv, iv_len, &output_len);

	if (output_len != iv_len)
		status = PSA_ERROR_GENERIC_ERROR;

	if (status != PSA_SUCCESS) {
		(void)psa_cipher_abort(&cipher_operation);
		return status;
	}

	status = psa_cipher_finish(&cipher_operation, iv + output_len, iv_len - output_len,
				   &finish_output_len);

	if (finish_output_len)
		status = PSA_ERROR_GENERIC_ERROR;

	if (status != PSA_SUCCESS) {
		(void)psa_cipher_abort(&cipher_operation);
		return status;
	}

	return status;
}

static psa_status_t init_encryption_keys(struct encrypted_block_store *context)
{
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;
	psa_key_handle_t root_key_handle = PSA_KEY_ID_NULL;
	uint8_t essiv_key_info[] = ESSIV_KEY_INFO;
	uint8_t decryption_key_info[] = DECRYPTION_KEY_INFO;

	status = import_root_key_to_keystore(&root_key_handle);
	if (status != PSA_SUCCESS) {
		EMSG("Importing block encryption root key failed with status %d\n", status);
		return status;
	}

	status = derive_and_store_aes_key(root_key_handle, &context->data_encryption_key_id,
					  PSA_ALG_CBC_NO_PADDING, essiv_key_info,
					  sizeof(essiv_key_info), block_encryption_salt,
					  sizeof(block_encryption_salt));
	if (status != PSA_SUCCESS) {
		EMSG("Key derivation and storing of data encryption key failed with status %d\n",
		     status);
		(void)remove_root_key_from_keystore(root_key_handle);
		return status;
	}

	/*
	 * By default essiv key is required to be the hash of the encryption key. Deriving it with
	 * HDKF(SHA256) algorithm makes it way safer. The keys are only derived once in each boot
	 * cycle.
	 */
	status = derive_and_store_aes_key(root_key_handle, &context->essiv_key_id,
					  PSA_ALG_ECB_NO_PADDING, decryption_key_info,
					  sizeof(decryption_key_info), block_encryption_salt,
					  sizeof(block_encryption_salt));
	if (status != PSA_SUCCESS) {
		EMSG("Key derivation and storing of essiv key failed with status %d\n", status);
		(void)remove_root_key_from_keystore(root_key_handle);
		return status;
	}

	status = remove_root_key_from_keystore(root_key_handle);
	if (status != PSA_SUCCESS) {
		EMSG("Removal of block encryption root key failed with status %d\n", status);
	}

	return status;
}

static psa_status_t encryption_keys_deinit(struct encrypted_block_store *context)
{
	/* The keys are volatile so even if the destroy request fails, a reset will remove them */
	(void)psa_destroy_key(context->data_encryption_key_id);
	(void)psa_destroy_key(context->essiv_key_id);

	return PSA_SUCCESS;
}

static psa_status_t encrypt_block(psa_key_id_t encryption_key, psa_key_id_t essiv_key, uint64_t lba,
				  const uint8_t *plaintext, uint8_t *ciphertext, size_t text_len)
{
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;
	psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;
	size_t output_len = 0;
	size_t finish_output_len = 0;
	uint8_t iv[BLK_AES_BLOCK_SIZE] = { 0 };

	status = calculate_essiv(lba, essiv_key, iv, sizeof(iv));
	if (status != PSA_SUCCESS) {
		EMSG("Calculating essiv failed with %d\n", status);
		return status;
	}

	status = psa_cipher_encrypt_setup(&operation, encryption_key, PSA_ALG_CBC_NO_PADDING);
	if (status != PSA_SUCCESS) {
		EMSG("Block encryption cipher setup failed with %d\n", status);
		return status;
	}

	status = psa_cipher_set_iv(&operation, iv, sizeof(iv));
	if (status != PSA_SUCCESS) {
		EMSG("Block encryption iv setting failed with %d\n", status);
		(void)psa_cipher_abort(&operation);
		return status;
	}

	status = psa_cipher_update(&operation, plaintext, text_len, ciphertext, text_len,
				   &output_len);

	if (output_len != text_len)
		status = PSA_ERROR_GENERIC_ERROR;

	if (status != PSA_SUCCESS) {
		EMSG("Block encryption cipher update failed with %d\n", status);
		(void)psa_cipher_abort(&operation);
		return status;
	}

	status = psa_cipher_finish(&operation, ciphertext + output_len, text_len - output_len,
				   &finish_output_len);

	if (finish_output_len)
		status = PSA_ERROR_GENERIC_ERROR;

	if (status != PSA_SUCCESS) {
		EMSG("Block encryption cipher finish failed with %d\n", status);
		(void)psa_cipher_abort(&operation);
		return status;
	}

	return status;
}

static psa_status_t decrypt_block(psa_key_id_t decryption_key, psa_key_id_t essiv_key, uint64_t lba,
				  const uint8_t *ciphertext, uint8_t *plaintext, size_t text_len)
{
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;
	psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;
	size_t output_len = 0;
	size_t finish_output_len = 0;
	uint8_t iv[BLK_AES_BLOCK_SIZE] = { 0 };

	status = calculate_essiv(lba, essiv_key, iv, sizeof(iv));
	if (status != PSA_SUCCESS) {
		EMSG("Calculating essiv failed with %d\n", status);
		return status;
	}

	status = psa_cipher_decrypt_setup(&operation, decryption_key, PSA_ALG_CBC_NO_PADDING);
	if (status != PSA_SUCCESS) {
		EMSG("Block decryption cipher setup failed with %d\n", status);
		return status;
	}

	status = psa_cipher_set_iv(&operation, iv, sizeof(iv));
	if (status != PSA_SUCCESS) {
		EMSG("Block decryption iv setting failed with %d\n", status);
		(void)psa_cipher_abort(&operation);
		return status;
	}

	status = psa_cipher_update(&operation, ciphertext, text_len, plaintext, text_len,
				   &output_len);

	if (output_len != text_len)
		status = PSA_ERROR_GENERIC_ERROR;

	if (status != PSA_SUCCESS) {
		EMSG("Block decryption cipher update failed with %d\n", status);
		(void)psa_cipher_abort(&operation);
		return status;
	}

	status = psa_cipher_finish(&operation, plaintext + output_len, text_len - output_len,
				   &finish_output_len);

	if (finish_output_len)
		status = PSA_ERROR_GENERIC_ERROR;

	if (status != PSA_SUCCESS) {
		EMSG("Block decryption cipher finish failed with %d\n", status);
		(void)psa_cipher_abort(&operation);
		return status;
	}

	return status;
}

static psa_status_t
encrypted_block_store_get_partition_info(void *context, const struct uuid_octets *partition_guid,
					 struct storage_partition_info *info)
{
	const struct encrypted_block_store *encrypted_block_store =
		(struct encrypted_block_store *)context;

	if (memcmp(&encrypted_block_store->back_store_info.partition_guid, partition_guid,
		   sizeof(struct uuid_octets)))
		return PSA_ERROR_INVALID_ARGUMENT;

	memcpy(info, &encrypted_block_store->back_store_info,
	       sizeof(encrypted_block_store->back_store_info));

	return PSA_SUCCESS;
}

static psa_status_t encrypted_block_store_open(void *context, uint32_t client_id,
					       const struct uuid_octets *partition_guid,
					       storage_partition_handle_t *handle)
{
	const struct encrypted_block_store *encrypted_block_store =
		(struct encrypted_block_store *)context;

	return block_store_open(encrypted_block_store->back_store, client_id, partition_guid,
				handle);
}

static psa_status_t encrypted_block_store_close(void *context, uint32_t client_id,
						storage_partition_handle_t handle)
{
	const struct encrypted_block_store *encrypted_block_store =
		(struct encrypted_block_store *)context;

	return block_store_close(encrypted_block_store->back_store, client_id, handle);
}

/*
 * Provides the data same way as encrypted_block_store_read, but without clearing the internal buffers.
 * The data will reside in the block_buffer_B
 */
static psa_status_t internal_encrypted_block_store_read(const struct encrypted_block_store *encrypted_block_store, uint32_t client_id,
					       storage_partition_handle_t handle, uint64_t lba,
					       size_t offset, size_t buffer_size, size_t *data_len)
{
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;
	size_t bytes_to_read = 0;
	size_t bytes_read = 0;

	if (offset >= encrypted_block_store->back_store_info.block_size)
		return PSA_ERROR_INVALID_ARGUMENT;

	bytes_to_read =
		MIN(buffer_size, encrypted_block_store->back_store_info.block_size - offset);

	/* Read the whole block */
	status = block_store_read(encrypted_block_store->back_store, client_id, handle, lba, 0,
				  encrypted_block_store->back_store_info.block_size,
				  encrypted_block_store->block_buffer_A, &bytes_read);
	if (status != PSA_SUCCESS)
		return status;

	/* The block is encrypted as a whole, so partial read does not work */
	if (bytes_read != encrypted_block_store->back_store_info.block_size) {
		return PSA_ERROR_INSUFFICIENT_DATA;
	}

	/* Decrypt the whole block */
	status = decrypt_block(encrypted_block_store->data_encryption_key_id,
			       encrypted_block_store->essiv_key_id, lba,
			       encrypted_block_store->block_buffer_A,
			       encrypted_block_store->block_buffer_B,
			       encrypted_block_store->back_store_info.block_size);
	if (status != PSA_SUCCESS) {
		return status;
	}

	/*
	 * As a result of encryption the whole block is read, but only the requested
	 * amount of the data is provided.
	 */
	*data_len = bytes_to_read;

	return status;
}

static psa_status_t encrypted_block_store_read(void *context, uint32_t client_id,
					       storage_partition_handle_t handle, uint64_t lba,
					       size_t offset, size_t buffer_size, uint8_t *buffer,
					       size_t *data_len)
{
	const struct encrypted_block_store *encrypted_block_store =
		(struct encrypted_block_store *)context;

	psa_status_t status = internal_encrypted_block_store_read(encrypted_block_store, client_id, handle, lba,
								  offset, buffer_size, data_len);

	if(status == PSA_SUCCESS) {
		/* Provide the requested part of the decrypted data */
		memcpy(buffer, encrypted_block_store->block_buffer_B + offset, *data_len);
	}

	clear_block_buffers(context);
	return status;
}

/*
 * AES-CBC encrypts the whole block, where each AES segment depends on the ciphertext of the
 * previous one. Even in case of a partial access the data of a whole block needs to be decrypted,
 * updated and re-encrypted before being written back to an erased block.
 */
static psa_status_t encrypted_block_store_write(void *context, uint32_t client_id,
						storage_partition_handle_t handle, uint64_t lba,
						size_t offset, const uint8_t *data, size_t data_len,
						size_t *num_written)
{
	const struct encrypted_block_store *encrypted_block_store =
		(struct encrypted_block_store *)context;

	psa_status_t status = PSA_ERROR_GENERIC_ERROR;
	size_t actual_read_len = 0;

	if (offset >= encrypted_block_store->back_store_info.block_size)
		return PSA_ERROR_INVALID_ARGUMENT;

	/* Request must not overwrite the block */
	data_len = MIN(data_len, encrypted_block_store->back_store_info.block_size - offset);

	/* Read a full block */
	status = internal_encrypted_block_store_read(context, client_id, handle, lba, 0,
					    encrypted_block_store->back_store_info.block_size,
					    &actual_read_len);
	if (status != PSA_SUCCESS ||
	    actual_read_len != encrypted_block_store->back_store_info.block_size) {
		clear_block_buffers(context);
		return status ? status : PSA_ERROR_INSUFFICIENT_DATA;
	}

	/* Read result is in the out buffer, so update it with the data to be written */
	memcpy(encrypted_block_store->block_buffer_B + offset, data, data_len);

	/* Encrypt the extended block in the out buffer */
	status = encrypt_block(encrypted_block_store->data_encryption_key_id,
			       encrypted_block_store->essiv_key_id, lba,
			       encrypted_block_store->block_buffer_B,
			       encrypted_block_store->block_buffer_A,
			       encrypted_block_store->back_store_info.block_size);
	if (status != PSA_SUCCESS) {
		clear_block_buffers(context);
		return status;
	}

	/* Write the block. The encrypted data resides in the in buffer */
	status = block_store_write(encrypted_block_store->back_store, client_id, handle, lba, 0,
				   encrypted_block_store->block_buffer_A,
				   encrypted_block_store->back_store_info.block_size, num_written);

	/* The block is encrypted as a whole, so partial write results in uncomprehensible data */
	if (*num_written != encrypted_block_store->back_store_info.block_size) {
		if (status == PSA_SUCCESS)
			status = PSA_ERROR_INSUFFICIENT_DATA;
	}

	/*
	 * Because of decryption the whole block is written back, but only the requested
	 * data is a relevant change.
	 */
	if (status == PSA_SUCCESS)
		*num_written = data_len;
	else
		*num_written = 0;

	clear_block_buffers(context);
	return status;
}

static psa_status_t encrypted_block_store_erase(void *context, uint32_t client_id,
						storage_partition_handle_t handle,
						uint64_t begin_lba, size_t num_blocks)
{
	const struct encrypted_block_store *encrypted_block_store =
		(struct encrypted_block_store *)context;

	return block_store_erase(encrypted_block_store->back_store, client_id, handle, begin_lba,
				 num_blocks);
}

struct block_store *encrypted_block_store_init(struct encrypted_block_store *encrypted_block_store,
					       uint32_t local_client_id,
					       const struct uuid_octets *back_store_guid,
					       struct block_store *back_store)
{
	psa_status_t status = PSA_ERROR_GENERIC_ERROR;

	/* Define concrete block store interface */
	static const struct block_store_interface interface = {
		encrypted_block_store_get_partition_info,
		encrypted_block_store_open,
		encrypted_block_store_close,
		encrypted_block_store_read,
		encrypted_block_store_write,
		encrypted_block_store_erase
	};

	if (!encrypted_block_store || !back_store_guid || !back_store) {
		EMSG("Invalid arguments, while initing encrypted block store");
		return NULL;
	}

	/* Initialize the fields of the encrypted_block_store */
	encrypted_block_store->base_block_store.context = encrypted_block_store;
	encrypted_block_store->base_block_store.interface = &interface;

	encrypted_block_store->essiv_key_id = PSA_KEY_ID_NULL;
	encrypted_block_store->data_encryption_key_id = PSA_KEY_ID_NULL;
	encrypted_block_store->back_store_handle = 0;
	encrypted_block_store->block_buffer_A = NULL;
	encrypted_block_store->block_buffer_B = NULL;
	memset(&encrypted_block_store->back_store_info, 0,
	       sizeof(encrypted_block_store->back_store_info));

	/* Note the local client ID - this corresponds to the local environment */
	encrypted_block_store->local_client_id = local_client_id;

	/* Stack over provided back store */
	encrypted_block_store->back_store = back_store;

	/* Multiple calls (e.g by separate services) will not cause problems */
	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		EMSG("Failed to init PSA crypto: %d", status);
		return NULL;
	}

	status = init_encryption_keys(encrypted_block_store);
	if (status != PSA_SUCCESS) {
		EMSG("Platform specific initialization failed\n");
		return NULL;
	}

	/* Get information about the underlying back store */
	status = block_store_get_partition_info(encrypted_block_store->back_store, back_store_guid,
						&encrypted_block_store->back_store_info);

	if (status != PSA_SUCCESS)
		return NULL;

	if (encrypted_block_store->back_store_info.block_size % BLK_AES_BLOCK_SIZE) {
		EMSG("Block size must be multiple of AES BLOCKS for AES-CBC encryption");
		(void)encryption_keys_deinit(encrypted_block_store);
		return NULL;
	}

	/* Allocate the block buffers */
	encrypted_block_store->block_buffer_A =
		(uint8_t *)calloc(1, encrypted_block_store->back_store_info.block_size);
	if (!encrypted_block_store->block_buffer_A)
		return NULL;

	encrypted_block_store->block_buffer_B =
		(uint8_t *)calloc(1, encrypted_block_store->back_store_info.block_size);
	if (!encrypted_block_store->block_buffer_B) {
		free(encrypted_block_store->block_buffer_A);
		return NULL;
	}

	return &encrypted_block_store->base_block_store;
}

void encrypted_block_store_deinit(struct encrypted_block_store *encrypted_block_store)
{
	/* Don't leave anything on the heap */
	clear_block_buffers(encrypted_block_store);
	free(encrypted_block_store->block_buffer_A);
	free(encrypted_block_store->block_buffer_B);

	(void)encrypted_block_store_close(encrypted_block_store,
					  encrypted_block_store->local_client_id,
					  encrypted_block_store->back_store_handle);

	(void)encryption_keys_deinit(encrypted_block_store);
}
