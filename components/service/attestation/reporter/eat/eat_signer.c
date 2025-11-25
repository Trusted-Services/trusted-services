/*
 * Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <stdlib.h>
#include <t_cose/t_cose_sign1_sign.h>
#include <psa/error.h>
#include "eat_signer.h"

static bool alloc_output_buf(struct q_useful_buf *buf, size_t input_len);
static int t_cose_to_psa_status(enum t_cose_err_t t_cose_status);

int eat_sign(psa_key_id_t key_id,
    const uint8_t *unsigned_token, size_t unsigned_token_len,
    const uint8_t **signed_token, size_t *signed_token_len)
{
    struct t_cose_key signing_key;
    struct t_cose_sign1_sign_ctx sign_ctx;
    enum t_cose_err_t sign_status;
    struct q_useful_buf_c input_buf;
    struct q_useful_buf output_buf;
    struct q_useful_buf_c result_buf;

    /* Prepare input and output buffers for signing operation */
    input_buf.ptr = unsigned_token;
    input_buf.len = unsigned_token_len;
    result_buf.ptr = NULL;
    result_buf.len = 0;
    if (!alloc_output_buf(&output_buf, unsigned_token_len))
        return PSA_ERROR_INSUFFICIENT_MEMORY;

    /* Initialize signing context */
    signing_key.k.key_handle = key_id;
    signing_key.crypto_lib  = T_COSE_CRYPTO_LIB_PSA;
    t_cose_sign1_sign_init(&sign_ctx, 0, T_COSE_ALGORITHM_ES256);
    t_cose_sign1_set_signing_key(&sign_ctx, signing_key,  NULL_Q_USEFUL_BUF_C);

    /* Sign */
    sign_status = t_cose_sign1_sign(&sign_ctx, input_buf, output_buf, &result_buf);

    if (sign_status == T_COSE_SUCCESS) {

        /* Note that result_buf points into output_buf with len
         * corresponding to the signed token length.
         */
        *signed_token = result_buf.ptr;
        *signed_token_len = result_buf.len;
    }
    else {

        free((void*)output_buf.ptr);
        *signed_token = NULL;
        *signed_token_len = 0;
    }

    return t_cose_to_psa_status(sign_status);
}

static bool alloc_output_buf(struct q_useful_buf *buf, size_t input_len)
{
    size_t required_space = input_len + 300;  /* todo figure out correct overhead */
    uint8_t *space = malloc(required_space);

    if (space) {
        buf->ptr = space;
        buf->len = required_space;
    }

    return (space != NULL);
}

static int t_cose_to_psa_status(enum t_cose_err_t t_cose_status)
{
    if (t_cose_status == T_COSE_SUCCESS)          return PSA_SUCCESS;
    if (t_cose_status == T_COSE_ERR_TOO_SMALL)    return PSA_ERROR_BUFFER_TOO_SMALL;

    return PSA_ERROR_PROGRAMMER_ERROR;
}
