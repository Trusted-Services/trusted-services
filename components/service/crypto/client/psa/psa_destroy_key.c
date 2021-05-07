/*
 * Copyright (c) 2021, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <psa/crypto.h>
#include "psa_crypto_client.h"
#include <protocols/rpc/common/packed-c/status.h>
#include <protocols/service/crypto/packed-c/opcodes.h>
#include <protocols/service/crypto/packed-c/destroy_key.h>


psa_status_t psa_destroy_key(psa_key_id_t id)
{
    psa_status_t psa_status = PSA_ERROR_GENERIC_ERROR;
    struct ts_crypto_destroy_key_in req_msg;
    size_t req_len = sizeof(struct ts_crypto_destroy_key_in);

    req_msg.id = id;

    rpc_call_handle call_handle;
    uint8_t *req_buf;

    call_handle = rpc_caller_begin(psa_crypto_client_instance.caller, &req_buf, req_len);

    if (call_handle) {

        uint8_t *resp_buf;
        size_t resp_len;
        int opstatus;

        memcpy(req_buf, &req_msg, req_len);

        psa_crypto_client_instance.rpc_status =
            rpc_caller_invoke(psa_crypto_client_instance.caller, call_handle,
                        TS_CRYPTO_OPCODE_DESTROY_KEY, &opstatus, &resp_buf, &resp_len);

        if (psa_crypto_client_instance.rpc_status == TS_RPC_CALL_ACCEPTED) psa_status = opstatus;

        rpc_caller_end(psa_crypto_client_instance.caller, call_handle);
    }

    return psa_status;
}