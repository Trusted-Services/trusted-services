/*
 * Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CRYPTO_CONTEXT_POOL_H
#define CRYPTO_CONTEXT_POOL_H

#include <stdint.h>
#include <service/crypto/backend/crypto_backend.h>

/**
 * Some crypto transactions require state to be held between separate
 * service operations.  A typical multi-call transaction such as a
 * hash calculation comprises a setup, one or more updates and a finish
 * operation.  This pool is used for allocating state context for multi-call
 * transactions.  For a well behaved client, a fresh context is allocated
 * on a setup and freed on the finish.  To cope with badly behaved clients
 * that may never finish a transaction, if no free contexts are available
 * for a new transaction, the least recently used active context is
 * recycled.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  Identifier for the operation type that a context is used for.
 */
enum crypto_context_op_id
{
	CRYPTO_CONTEXT_OP_ID_NONE,
	CRYPTO_CONTEXT_OP_ID_HASH,
	CRYPTO_CONTEXT_OP_ID_MAC,
	CRYPTO_CONTEXT_OP_ID_CIPHER,
	CRYPTO_CONTEXT_OP_ID_AEAD,
	CRYPTO_CONTEXT_OP_ID_KEY_DERIVATION
};

struct crypto_context;

typedef void (*crypto_context_cleanup_t)(struct crypto_context *context);

/**
 * A crypto context, used to hold state for a multi-step transaction.
 */
struct crypto_context
{
	enum crypto_context_op_id usage;
	uint32_t client_id;
	uint32_t op_handle;
	crypto_context_cleanup_t cleanup;
	struct crypto_context *next;
	struct crypto_context *prev;

	union context_variant
	{
		psa_hash_operation_t hash;
		psa_mac_operation_t mac;
		psa_cipher_operation_t cipher;
		psa_aead_operation_t aead;
		psa_key_derivation_operation_t key_derivation;
	} op;
};

/**
 * The default pool size.  This may be overridden to meet the needs
 * of a particular deployment.
 */
#ifndef CRYPTO_CONTEXT_POOL_SIZE
#define CRYPTO_CONTEXT_POOL_SIZE            (10)
#endif

/**
 * The crypto context pool structure.
 */
struct crypto_context_pool
{
	struct crypto_context contexts[CRYPTO_CONTEXT_POOL_SIZE];
	struct crypto_context *free;
	struct crypto_context *active_head;
	struct crypto_context *active_tail;
	uint32_t most_recent_op_handle;
};

/*
 * Initializes a crypto_context_pool, called once during setup.
 */
void crypto_context_pool_init(struct crypto_context_pool *pool);

/*
 * De-initializes a crypto_context_pool, called once during tear-down.
 */
void crypto_context_pool_deinit(struct crypto_context_pool *pool);

/*
 * Allocate a fresh context.  On success, a pointer to a crypto_context object
 * is returned and an op handle is provided for reacqiring the context during
 * sunsequent operations.
 */
struct crypto_context *crypto_context_pool_alloc(struct crypto_context_pool *pool,
	enum crypto_context_op_id usage,
	uint32_t client_id,
	crypto_context_cleanup_t cleanup,
	uint32_t *op_handle);

/*
 * Frees a context after use.
 */
void crypto_context_pool_free(struct crypto_context_pool *pool,
	struct crypto_context *context);

/*
 * Find an allocated context.  Returns NULL is no qualifying context is held.
 */
struct crypto_context *crypto_context_pool_find(struct crypto_context_pool *pool,
	enum crypto_context_op_id usage,
	uint32_t client_id,
	uint32_t op_handle);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CRYPTO_CONTEXT_POOL_H */
