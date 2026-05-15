/*
 * Copyright (c) 2022-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "common/uuid/uuid.h"
#include <service/test_runner/provider/backend/simple_c/simple_c_test_runner.h>
#include <service/block_storage/block_store/device/semihosting/semihosting_block_store.h>

#define BLOCK_SIZE		(128)
#define NUM_BLOCKS		(20)
#define CLIENT_ID		(0)

/**
 * Tests for the semihosting block store.
 */
static const char *sh_filename = "test-store.img";
static struct semihosting_block_store sh_block_store;
static storage_partition_handle_t partition_handle;

static bool setup(struct test_failure *failure)
{
	semihosting_block_store_wipe(sh_filename);

	struct uuid_octets disk_guid;
	uuid_guid_octets_from_canonical(&disk_guid,
		"418edc38-fa66-4871-979b-e67bee88b8f2");

	if (!semihosting_block_store_init(&sh_block_store, &disk_guid, sh_filename, BLOCK_SIZE)) {

		/* Failed to initialize block store */
		failure->line_num = __LINE__;
		return false;
	}


	semihosting_block_store_configure(&sh_block_store,
		&disk_guid,
		NUM_BLOCKS, BLOCK_SIZE);

	psa_status_t status = block_store_open(
		&sh_block_store.base_block_device.base_block_store,
		CLIENT_ID,
		&disk_guid,
		&partition_handle);

	if (status != PSA_SUCCESS) {

		failure->line_num = __LINE__;
		failure->info = status;

		semihosting_block_store_deinit(&sh_block_store);

		return false;
	}

	return true;
}

static void teardown(void)
{
	block_store_close(
		&sh_block_store.base_block_device.base_block_store,
		CLIENT_ID,
		partition_handle);

	semihosting_block_store_deinit(&sh_block_store);
}

static bool set_block(struct test_failure *failure,
	size_t lba, size_t offset,
	size_t len, uint8_t val,
	size_t *num_written)
{
	bool success = true;
	struct block_store *bs = &sh_block_store.base_block_device.base_block_store;
	uint8_t *write_buf = malloc(len);

	if (!write_buf) {

		failure->line_num = __LINE__;
		return false;
	}

	memset(write_buf, val, len);
	*num_written = 0;

	psa_status_t status = block_store_write(
		bs, CLIENT_ID, partition_handle,
		lba, offset,
		write_buf, len, num_written);

	if (status != PSA_SUCCESS) {

		failure->line_num = __LINE__;
		failure->info = status;
		success	= false;
	}

	free(write_buf);

	return success;
}

static bool check_block(struct test_failure *failure,
	size_t lba, size_t offset,
	size_t len, uint8_t expected_val)
{
	bool success = true;
	struct block_store *bs = &sh_block_store.base_block_device.base_block_store;
	uint8_t *read_buf = malloc(len);

	if (!read_buf) {

		failure->line_num = __LINE__;
		return false;
	}

	size_t num_read = 0;

	psa_status_t status = block_store_read(
		bs, CLIENT_ID, partition_handle,
		lba, offset,
		len, read_buf,
		&num_read);

	if (status == PSA_SUCCESS) {

		if (num_read == len) {

			for (size_t i = 0; i < len; i++) {

				if (read_buf[i] != expected_val) {

					failure->line_num = __LINE__;
					failure->info = read_buf[i];
					success	= false;
					break;
				}
			}
		}
		else {

			failure->line_num = __LINE__;
			failure->info = num_read;
			success	= false;
		}
	}
	else {

		failure->line_num = __LINE__;
		failure->info = status;
		success	= false;
	}

	free(read_buf);

	return success;
}

static bool erase_blocks(struct test_failure *failure,
	uint64_t begin_lba,
	size_t num_blocks)
{
	bool success = true;
	struct block_store *bs = &sh_block_store.base_block_device.base_block_store;

	psa_status_t status = block_store_erase(
		bs, CLIENT_ID, partition_handle,
		begin_lba, num_blocks);

	if (status != PSA_SUCCESS) {

		failure->line_num = __LINE__;
		failure->info = status;
		success	= false;
	}

	return success;
}

/*
 * Check a sequence of whole block writes and reads.
 */
static bool check_whole_block_rw(struct test_failure *failure)
{
	bool pass = setup(failure);

	if (pass) {

		size_t num_written = 0;

		if (pass && (pass = set_block(failure, 7, 0, BLOCK_SIZE, 'a', &num_written))) {

			if (!(pass = (num_written == BLOCK_SIZE))) {

				failure->line_num = __LINE__;
				failure->info = num_written;
			}
		}

		if (pass && (pass = set_block(failure, 6, 0, BLOCK_SIZE, 'b', &num_written))) {

			if (!(pass = (num_written == BLOCK_SIZE))) {

				failure->line_num = __LINE__;
				failure->info = num_written;
			}
		}

		if (pass && (pass = set_block(failure, 1, 0, BLOCK_SIZE, 'c', &num_written))) {

			if (!(pass = (num_written == BLOCK_SIZE))) {

				failure->line_num = __LINE__;
				failure->info = num_written;
			}
		}

		if (pass && (pass = set_block(failure, 9, 0, BLOCK_SIZE, 'd', &num_written))) {

			if (!(pass = (num_written == BLOCK_SIZE))) {

				failure->line_num = __LINE__;
				failure->info = num_written;
			}
		}

		/* Check written blocks are as expected */
		if (pass) {

			pass =
				check_block(failure, 7, 0, BLOCK_SIZE, 'a') &&
				check_block(failure, 6, 0, BLOCK_SIZE, 'b') &&
				check_block(failure, 1, 0, BLOCK_SIZE, 'c') &&
				check_block(failure, 9, 0, BLOCK_SIZE, 'd');
		}

		/* Erase all the written blocks */
		if (pass) {

			pass =
				erase_blocks(failure, 9, 1) &&
				erase_blocks(failure, 1, 1) &&
				erase_blocks(failure, 7, 1) &&
				erase_blocks(failure, 6, 1);
		}

		teardown();
	}

	return pass;
}

/**
 * Define and register test group
 */
void sh_block_store_tests_register(void)
{
	static const struct simple_c_test_case sh_block_store_tests[] = {
		{.name = "WholeBlockRw", .test_func = check_whole_block_rw},
	};

	static const struct simple_c_test_group sh_block_store_test_group =
	{
		.group = "ShBlockStoreTests",
		.num_test_cases = sizeof(sh_block_store_tests)/sizeof(struct simple_c_test_case),
		.test_cases = sh_block_store_tests
	};

	simple_c_test_runner_register_group(&sh_block_store_test_group);
}
