/*
 * Copyright (c) 2021-2026, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "crypto_context_pool.h"

static void add_to_free_list(struct crypto_context_pool *pool,
	struct crypto_context *context);

static uint32_t alloc_op_handle(struct crypto_context_pool *pool);
static bool op_handle_in_use(struct crypto_context_pool *pool, uint32_t candidate);


void crypto_context_pool_init(struct crypto_context_pool *pool)
{
	pool->free = NULL;
	pool->active_head = NULL;
	pool->active_tail = NULL;
	pool->most_recent_op_handle = 0;

	for (size_t i = 0; i < CRYPTO_CONTEXT_POOL_SIZE; i++) {

		add_to_free_list(pool, &pool->contexts[i]);
	}
}

void crypto_context_pool_deinit(struct crypto_context_pool *pool)
{
	(void)pool;
}

struct crypto_context *crypto_context_pool_alloc(struct crypto_context_pool *pool,
	enum crypto_context_op_id usage,
	uint32_t client_id,
	crypto_context_cleanup_t cleanup,
	uint32_t *op_handle)
{
	struct crypto_context *context = NULL;

	/* Re-cycle least-recently used context if there are no free contexts */
	if (!pool->free && pool->active_tail) {
		if (pool->active_tail->cleanup)
			pool->active_tail->cleanup(pool->active_tail);

		crypto_context_pool_free(pool, pool->active_tail);
	}

	/* Active context are held in a linked list in most recently allocated order */
	if (pool->free) {

		context = pool->free;
		pool->free = context->next;

		context->next = pool->active_head;
		context->prev = NULL;
		pool->active_head = context;

		if (!pool->active_tail) pool->active_tail = context;
		if (context->next) context->next->prev = context;

		context->usage = usage;
		context->client_id = client_id;
		context->cleanup = cleanup;

		context->op_handle = alloc_op_handle(pool);
		*op_handle = context->op_handle;
	}

	return context;
}

void crypto_context_pool_free(struct crypto_context_pool *pool,
	struct crypto_context *context)
{
	/* Remove from active list */
	if (context->prev) {
		context->prev->next = context->next;
	}
	else {
		pool->active_head = context->next;
	}

	if (context->next) {
		context->next->prev = context->prev;
	}
	else {
		pool->active_tail = context->prev;
	}

	/* Add to free list */
	add_to_free_list(pool, context);
}

struct crypto_context *crypto_context_pool_find(struct crypto_context_pool *pool,
	enum crypto_context_op_id usage,
	uint32_t client_id,
	uint32_t op_handle)
{
	/* Finds an active context that looks as though it legitimately belongs to the
	 * requesting client.  Defends against bad behaviour from the client such
	 * as misusing a context for a different operation from the one that was
	 * setup.
	 */
	struct crypto_context *found = NULL;
	struct crypto_context *context = pool->active_head;

	while (context) {

		if ((context->op_handle == op_handle) &&
			(context->usage == usage) &&
			(context->client_id == client_id)) {

			found = context;
			break;
		}

		context = context->next;
	}

	return found;
}

static void add_to_free_list(struct crypto_context_pool *pool,
	struct crypto_context *context)
{
	context->usage = CRYPTO_CONTEXT_OP_ID_NONE;
	context->op_handle = 0;
	context->cleanup = NULL;
	context->next = pool->free;
	context->prev = NULL;
	pool->free = context;
}

static uint32_t alloc_op_handle(struct crypto_context_pool *pool)
{
	/* op handles need to be unique and to minimize the probability
	 * of a client using a stale handle that collides with a legitmately
	 * active one, use a rolling 32-bit integer.
	 */
	uint32_t candidate = pool->most_recent_op_handle + 1;

	while (op_handle_in_use(pool, candidate)) ++candidate;

	pool->most_recent_op_handle = candidate;

	return candidate;
}

static bool op_handle_in_use(struct crypto_context_pool *pool, uint32_t candidate)
{
	bool in_use = false;
	struct crypto_context *context = pool->active_head;

	while (context && !in_use) {

		in_use = (candidate == context->op_handle);
		context = context->next;
	}

	return in_use;
}
