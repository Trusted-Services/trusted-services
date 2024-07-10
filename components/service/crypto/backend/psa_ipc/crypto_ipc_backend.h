/*
 * Copyright (c) 2021-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CRYPTO_IPC_BACKEND_H
#define CRYPTO_IPC_BACKEND_H

#include <service/crypto/client/psa/psa_crypto_client.h>
#include <psa/error.h>
#include <rpc_caller.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief This type is used to overcome a limitation in the number of maximum
 *        IOVECs that can be used especially in psa_aead_encrypt and
 *        psa_aead_decrypt. To be removed in case the AEAD APIs number of
 *        parameters passed gets restructured
 */
#define TFM_CRYPTO_MAX_NONCE_LENGTH (16u)
struct psa_ipc_crypto_aead_pack_input {
	uint8_t nonce[TFM_CRYPTO_MAX_NONCE_LENGTH];
	uint32_t nonce_length;
};

struct psa_ipc_crypto_pack_iovec {
    psa_key_id_t key_id;     /*!< Key id */
    psa_algorithm_t alg;     /*!< Algorithm */
    uint32_t op_handle;      /*!< Client context handle associated to a
                              *   multipart operation
                              */
    uint32_t ad_length;        /*!< Additional Data length for multipart AEAD */
    uint32_t plaintext_length; /*!< Plaintext length for multipart AEAD */

    struct psa_ipc_crypto_aead_pack_input aead_in; /*!< Packs AEAD-related inputs */

    uint16_t function_id;    /*!< Used to identify the function in the
                              *   API dispatcher to the service backend
                              *   See tfm_crypto_func_sid for detail
                              */
    uint16_t step;           /*!< Key derivation step */
    union {
        size_t capacity;     /*!< Key derivation capacity */
        uint64_t value;      /*!< Key derivation integer for update*/
    };
};

#define iov_size sizeof(struct psa_ipc_crypto_pack_iovec)

/**
 * \brief Initialize the psa ipc crypto backend
 *
 * Initializes a crypto backend that uses the psa API client with a
 * psa_ipc_backend caller to realize the PSA crypto API used by the crypto
 * service proviser.
 *
 * \return PSA_SUCCESS if backend initialized successfully
 */
psa_status_t crypto_ipc_backend_init(struct rpc_caller_session *session);

/**
 * \brief Clean-up to free any resource used by the crypto backend
 */
void crypto_ipc_backend_deinit(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CRYPTO_IPC_BACKEND_H */
