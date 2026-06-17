/*
 * Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <service/crypto/provider/crypto_context_pool.h>
#include <CppUTest/TestHarness.h>

static unsigned int cleanup_count;

static void count_cleanup(struct crypto_context *context)
{
	(void)context;
	++cleanup_count;
}

/*
 * Component tests for the crypto_context_pool.
 */
TEST_GROUP(CryptoContextPoolTests)
{
	void setup()
	{
		cleanup_count = 0;
		crypto_context_pool_init(&pool_under_test);
	}

	void teardown()
	{
		crypto_context_pool_deinit(&pool_under_test);
	}

	struct crypto_context_pool pool_under_test;
};

TEST(CryptoContextPoolTests, checkEmptyPool)
{
	struct crypto_context *context =
		crypto_context_pool_find(&pool_under_test, CRYPTO_CONTEXT_OP_ID_HASH, 0, 0);

	/* Expect a freshly initialized pool to fail to find a context */
	CHECK_FALSE(context);
}

TEST(CryptoContextPoolTests, singleContext)
{
	uint32_t op_handle;
	uint32_t client_id = 22;

	struct crypto_context *initial_context =
		crypto_context_pool_alloc(&pool_under_test,
			CRYPTO_CONTEXT_OP_ID_HASH, client_id,
			NULL, &op_handle);

	CHECK_TRUE(initial_context);

	struct crypto_context *follow_on_context =
		crypto_context_pool_find(&pool_under_test,
		CRYPTO_CONTEXT_OP_ID_HASH, client_id,
		op_handle);

	UNSIGNED_LONGS_EQUAL(initial_context, follow_on_context);

	crypto_context_pool_free(&pool_under_test, initial_context);
}

TEST(CryptoContextPoolTests, multipleContexts)
{
	/* Test multiple concurrent contexts but never exceeding the pool size */
	struct crypto_context *context;
	uint32_t zombie_handle_1, zombie_handle_2;

	/* First start a couple of zombie contexts.  This will occur if a client
	 * starts a transaction but never finishes it.  This could happen due to a
	 * misbehaving client or if a client process crashes.  This checks that
	 * recycling of least recently used contexts is working.
	 */
	context = crypto_context_pool_alloc(&pool_under_test,
				CRYPTO_CONTEXT_OP_ID_MAC, 77,
				count_cleanup, &zombie_handle_1);
	CHECK_TRUE(context);

	context = crypto_context_pool_alloc(&pool_under_test,
				CRYPTO_CONTEXT_OP_ID_CIPHER, 88,
				count_cleanup, &zombie_handle_2);
	CHECK_TRUE(context);

	/* Now run through the normal life-cycle for a load of concurrent contexts */
	for (unsigned int i = 0; i < 1000; ++i) {

		uint32_t op_handles[CRYPTO_CONTEXT_POOL_SIZE];
		unsigned int num_concurrent = (rand() % CRYPTO_CONTEXT_POOL_SIZE) + 1;

		/* Start some concurrent contexts, each belonging to different clients */
		for (unsigned int context_index = 0; context_index < num_concurrent; ++context_index) {

			context = crypto_context_pool_alloc(&pool_under_test,
				CRYPTO_CONTEXT_OP_ID_HASH, context_index,
				NULL, &op_handles[context_index]);

			CHECK_TRUE(context);
		}

		/* Expect the find to work for all active contexts */
		for (unsigned int context_index = 0; context_index < num_concurrent; ++context_index) {

			context = crypto_context_pool_find(&pool_under_test,
				CRYPTO_CONTEXT_OP_ID_HASH, context_index,
				op_handles[context_index]);

			CHECK_TRUE(context);
		}

		/* Then find and free all contexts */
		for (unsigned int context_index = 0; context_index < num_concurrent; ++context_index) {

			context = crypto_context_pool_find(&pool_under_test,
				CRYPTO_CONTEXT_OP_ID_HASH, context_index,
				op_handles[context_index]);

			CHECK_TRUE(context);

			crypto_context_pool_free(&pool_under_test, context);
		}
	}

	/* Expect the zombie contexts to have been recycled */
	UNSIGNED_LONGS_EQUAL(2, cleanup_count);

	context = crypto_context_pool_find(&pool_under_test,
				CRYPTO_CONTEXT_OP_ID_MAC, 77,
				zombie_handle_1);
	CHECK_FALSE(context);

	context = crypto_context_pool_find(&pool_under_test,
				CRYPTO_CONTEXT_OP_ID_CIPHER, 88,
				zombie_handle_2);
	CHECK_FALSE(context);
}
