/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __PSA_CRYPTO_SID_H__
#define __PSA_CRYPTO_SID_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

/**
 * \brief Type associated to the group of a function encoding. There can be
 *        nine groups (Random, Key management, Hash, MAC, Cipher, AEAD,
 *        Asym sign, Asym encrypt, Key derivation).
 */
enum tfm_crypto_group_id {
    TFM_CRYPTO_GROUP_ID_RANDOM = 0x0,
    TFM_CRYPTO_GROUP_ID_KEY_MANAGEMENT,
    TFM_CRYPTO_GROUP_ID_HASH,
    TFM_CRYPTO_GROUP_ID_MAC,
    TFM_CRYPTO_GROUP_ID_CIPHER,
    TFM_CRYPTO_GROUP_ID_AEAD,
    TFM_CRYPTO_GROUP_ID_ASYM_SIGN,
    TFM_CRYPTO_GROUP_ID_ASYM_ENCRYPT,
    TFM_CRYPTO_GROUP_ID_KEY_DERIVATION,
};

/* X macro describing each of the available PSA Crypto APIs */
#define KEY_MANAGEMENT_FUNCS                       \
    X(TFM_CRYPTO_GET_KEY_ATTRIBUTES)               \
    X(TFM_CRYPTO_RESET_KEY_ATTRIBUTES)             \
    X(TFM_CRYPTO_OPEN_KEY)                         \
    X(TFM_CRYPTO_CLOSE_KEY)                        \
    X(TFM_CRYPTO_IMPORT_KEY)                       \
    X(TFM_CRYPTO_DESTROY_KEY)                      \
    X(TFM_CRYPTO_EXPORT_KEY)                       \
    X(TFM_CRYPTO_EXPORT_PUBLIC_KEY)                \
    X(TFM_CRYPTO_PURGE_KEY)                        \
    X(TFM_CRYPTO_COPY_KEY)                         \
    X(TFM_CRYPTO_GENERATE_KEY)

#define HASH_FUNCS                                 \
    X(TFM_CRYPTO_HASH_COMPUTE)                     \
    X(TFM_CRYPTO_HASH_COMPARE)                     \
    X(TFM_CRYPTO_HASH_SETUP)                       \
    X(TFM_CRYPTO_HASH_UPDATE)                      \
    X(TFM_CRYPTO_HASH_CLONE)                       \
    X(TFM_CRYPTO_HASH_FINISH)                      \
    X(TFM_CRYPTO_HASH_VERIFY)                      \
    X(TFM_CRYPTO_HASH_ABORT)

#define MAC_FUNCS                                  \
    X(TFM_CRYPTO_MAC_COMPUTE)                      \
    X(TFM_CRYPTO_MAC_VERIFY)                       \
    X(TFM_CRYPTO_MAC_SIGN_SETUP)                   \
    X(TFM_CRYPTO_MAC_VERIFY_SETUP)                 \
    X(TFM_CRYPTO_MAC_UPDATE)                       \
    X(TFM_CRYPTO_MAC_SIGN_FINISH)                  \
    X(TFM_CRYPTO_MAC_VERIFY_FINISH)                \
    X(TFM_CRYPTO_MAC_ABORT)

#define CIPHER_FUNCS                               \
    X(TFM_CRYPTO_CIPHER_ENCRYPT)                   \
    X(TFM_CRYPTO_CIPHER_DECRYPT)                   \
    X(TFM_CRYPTO_CIPHER_ENCRYPT_SETUP)             \
    X(TFM_CRYPTO_CIPHER_DECRYPT_SETUP)             \
    X(TFM_CRYPTO_CIPHER_GENERATE_IV)               \
    X(TFM_CRYPTO_CIPHER_SET_IV)                    \
    X(TFM_CRYPTO_CIPHER_UPDATE)                    \
    X(TFM_CRYPTO_CIPHER_FINISH)                    \
    X(TFM_CRYPTO_CIPHER_ABORT)

#define AEAD_FUNCS                                 \
    X(TFM_CRYPTO_AEAD_ENCRYPT)                     \
    X(TFM_CRYPTO_AEAD_DECRYPT)                     \
    X(TFM_CRYPTO_AEAD_ENCRYPT_SETUP)               \
    X(TFM_CRYPTO_AEAD_DECRYPT_SETUP)               \
    X(TFM_CRYPTO_AEAD_GENERATE_NONCE)              \
    X(TFM_CRYPTO_AEAD_SET_NONCE)                   \
    X(TFM_CRYPTO_AEAD_SET_LENGTHS)                 \
    X(TFM_CRYPTO_AEAD_UPDATE_AD)                   \
    X(TFM_CRYPTO_AEAD_UPDATE)                      \
    X(TFM_CRYPTO_AEAD_FINISH)                      \
    X(TFM_CRYPTO_AEAD_VERIFY)                      \
    X(TFM_CRYPTO_AEAD_ABORT)

#define ASYMMETRIC_SIGN_FUNCS                      \
    X(TFM_CRYPTO_ASYMMETRIC_SIGN_MESSAGE)          \
    X(TFM_CRYPTO_ASYMMETRIC_VERIFY_MESSAGE)        \
    X(TFM_CRYPTO_ASYMMETRIC_SIGN_HASH)             \
    X(TFM_CRYPTO_ASYMMETRIC_VERIFY_HASH)

#define AYSMMETRIC_ENCRYPT_FUNCS                   \
    X(TFM_CRYPTO_ASYMMETRIC_ENCRYPT)               \
    X(TFM_CRYPTO_ASYMMETRIC_DECRYPT)

#define KEY_DERIVATION_FUNCS                       \
    X(TFM_CRYPTO_RAW_KEY_AGREEMENT)                \
    X(TFM_CRYPTO_KEY_DERIVATION_SETUP)             \
    X(TFM_CRYPTO_KEY_DERIVATION_GET_CAPACITY)      \
    X(TFM_CRYPTO_KEY_DERIVATION_SET_CAPACITY)      \
    X(TFM_CRYPTO_KEY_DERIVATION_INPUT_BYTES)       \
    X(TFM_CRYPTO_KEY_DERIVATION_INPUT_KEY)         \
    X(TFM_CRYPTO_KEY_DERIVATION_KEY_AGREEMENT)     \
    X(TFM_CRYPTO_KEY_DERIVATION_OUTPUT_BYTES)      \
    X(TFM_CRYPTO_KEY_DERIVATION_OUTPUT_KEY)        \
    X(TFM_CRYPTO_KEY_DERIVATION_ABORT)

#define RANDOM_FUNCS                               \
    X(TFM_CRYPTO_GENERATE_RANDOM)

/*
 * Define function IDs in each group. The function ID will be encoded into
 * tfm_crypto_func_sid below.
 * Each group is defined as a dedicated enum in case the total number of
 * PSA Crypto APIs exceeds 256.
 */
#define X(func_id)    func_id,
enum tfm_crypto_key_management_func_id {
    KEY_MANAGEMENT_FUNCS
};
enum tfm_crypto_hash_func_id {
    HASH_FUNCS
};
enum tfm_crypto_mac_func_id {
    MAC_FUNCS
};
enum tfm_crypto_cipher_func_id {
    CIPHER_FUNCS
};
enum tfm_crypto_aead_func_id {
    AEAD_FUNCS
};
enum tfm_crypto_asym_sign_func_id {
    ASYMMETRIC_SIGN_FUNCS
};
enum tfm_crypto_asym_encrypt_func_id {
    AYSMMETRIC_ENCRYPT_FUNCS
};
enum tfm_crypto_key_derivation_func_id {
    KEY_DERIVATION_FUNCS
};
enum tfm_crypto_random_func_id {
    RANDOM_FUNCS
};
#undef X

#define FUNC_ID(func_id)    (((func_id) & 0xFF) << 8)

/*
 * Numerical progressive value identifying a function API exposed through
 * the interfaces (S or NS). It's used to dispatch the requests from S/NS
 * to the corresponding API implementation in the Crypto service backend.
 *
 * Each function SID is encoded as uint16_t.
 *     |  Func ID  |  Group ID |
 *     15         8 7          0
 * Func ID is defined in each group func_id enum above
 * Group ID is defined in tfm_crypto_group_id.
 */
enum tfm_crypto_func_sid {

#define X(func_id)      func_id ## _SID = (uint16_t)((FUNC_ID(func_id)) | \
                                   (TFM_CRYPTO_GROUP_ID_KEY_MANAGEMENT & 0xFF)),

    KEY_MANAGEMENT_FUNCS

#undef X
#define X(func_id)      func_id ## _SID = (uint16_t)((FUNC_ID(func_id)) | \
                                            (TFM_CRYPTO_GROUP_ID_HASH & 0xFF)),
    HASH_FUNCS

#undef X
#define X(func_id)      func_id ## _SID = (uint16_t)((FUNC_ID(func_id)) | \
                                            (TFM_CRYPTO_GROUP_ID_MAC & 0xFF)),
    MAC_FUNCS

#undef X
#define X(func_id)      func_id ## _SID = (uint16_t)((FUNC_ID(func_id)) | \
                                           (TFM_CRYPTO_GROUP_ID_CIPHER & 0xFF)),
    CIPHER_FUNCS

#undef X
#define X(func_id)      func_id ## _SID = (uint16_t)((FUNC_ID(func_id)) | \
                                             (TFM_CRYPTO_GROUP_ID_AEAD & 0xFF)),
    AEAD_FUNCS

#undef X
#define X(func_id)      func_id ## _SID = (uint16_t)((FUNC_ID(func_id)) | \
                                        (TFM_CRYPTO_GROUP_ID_ASYM_SIGN & 0xFF)),
    ASYMMETRIC_SIGN_FUNCS

#undef X
#define X(func_id)      func_id ## _SID = (uint16_t)((FUNC_ID(func_id)) | \
                                     (TFM_CRYPTO_GROUP_ID_ASYM_ENCRYPT & 0xFF)),
    AYSMMETRIC_ENCRYPT_FUNCS

#undef X
#define X(func_id)      func_id ## _SID = (uint16_t)((FUNC_ID(func_id)) | \
                                   (TFM_CRYPTO_GROUP_ID_KEY_DERIVATION & 0xFF)),
    KEY_DERIVATION_FUNCS

#undef X
#define X(func_id)      func_id ## _SID = (uint16_t)((FUNC_ID(func_id)) | \
                                           (TFM_CRYPTO_GROUP_ID_RANDOM & 0xFF)),
    RANDOM_FUNCS

};
#undef X

/**
 * \brief Define an invalid value for an SID
 *
 */
#define TFM_CRYPTO_SID_INVALID (~0x0u)

/**
 * \brief This value is used to mark an handle as invalid.
 *
 */
#define TFM_CRYPTO_INVALID_HANDLE (0x0u)

/**
 * \brief Define miscellaneous literal constants that are used in the service
 *
 */
enum {
    TFM_CRYPTO_NOT_IN_USE = 0,
    TFM_CRYPTO_IN_USE = 1
};

#ifdef __cplusplus
}
#endif

#endif /* __PSA_CRYPTO_SID_H__ */
