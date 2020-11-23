/*
 * Copyright (c) 2020, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <string.h>
#include <stdlib.h>
#include <protocols/rpc/common/packed-c/status.h>
#include <service/common/serializer/protobuf/pb_helper.h>
#include <service/crypto/protobuf/generate_key.pb.h>
#include <service/crypto/protobuf/open_key.pb.h>
#include <service/crypto/protobuf/close_key.pb.h>
#include <service/crypto/protobuf/destroy_key.pb.h>
#include <service/crypto/protobuf/export_key.pb.h>
#include <service/crypto/protobuf/export_public_key.pb.h>
#include <service/crypto/protobuf/import_key.pb.h>
#include <service/crypto/protobuf/sign_hash.pb.h>
#include <service/crypto/protobuf/verify_hash.pb.h>
#include <service/crypto/protobuf/asymmetric_decrypt.pb.h>
#include <service/crypto/protobuf/asymmetric_encrypt.pb.h>
#include <service/crypto/protobuf/generate_random.pb.h>
#include <pb_decode.h>
#include <pb_encode.h>
#include <psa/crypto.h>
#include "pb_key_attributes_translator.h"
#include "pb_crypto_provider_serializer.h"

/* Returns the maximum possible deserialized parameter size for a protobuf encoded message. */
static size_t max_deserialised_parameter_size(const struct call_param_buf *req_buf)
{
    /*
     * Assume that a deserialized parameter must be the same size or smalled than the
     * entire serialized message.
     */
    return req_buf->data_len;
}

/* Operation: generate_key */
static rpc_status_t deserialize_generate_key_req(const struct call_param_buf *req_buf,
                                            psa_key_attributes_t *attributes)
{
    rpc_status_t rpc_status = TS_RPC_ERROR_INVALID_REQ_BODY;
    ts_crypto_GenerateKeyIn recv_msg = ts_crypto_GenerateKeyIn_init_default;

    pb_istream_t istream = pb_istream_from_buffer((const uint8_t*)req_buf->data, req_buf->data_len);

    if (pb_decode(&istream, ts_crypto_GenerateKeyIn_fields, &recv_msg) && recv_msg.has_attributes) {

        pb_crypto_provider_translate_key_attributes(attributes, &recv_msg.attributes);
        rpc_status = TS_RPC_CALL_ACCEPTED;
    }

    return rpc_status;
}

static rpc_status_t serialize_generate_key_resp(struct call_param_buf *resp_buf,
                                            psa_key_handle_t handle)
{
    size_t packed_resp_size;
    rpc_status_t rpc_status = TS_RPC_ERROR_INTERNAL;
    ts_crypto_GenerateKeyOut resp_msg = ts_crypto_GenerateKeyOut_init_default;
    resp_msg.handle = handle;

    if (pb_get_encoded_size(&packed_resp_size, ts_crypto_GenerateKeyOut_fields, &resp_msg) &&
        (packed_resp_size <= resp_buf->size)) {

        pb_ostream_t ostream = pb_ostream_from_buffer((uint8_t*)resp_buf->data, packed_resp_size);
	    if (pb_encode(&ostream, ts_crypto_GenerateKeyOut_fields, &resp_msg)) {

            resp_buf->data_len = packed_resp_size;
            rpc_status = TS_RPC_CALL_ACCEPTED;
        }
    }

    return rpc_status;
}

/* Operation: destroy_key */
static rpc_status_t deserialize_destroy_key_req(const struct call_param_buf *req_buf,
                                            psa_key_handle_t *handle)
{
    rpc_status_t rpc_status = TS_RPC_ERROR_INVALID_REQ_BODY;
    ts_crypto_DestroyKeyIn recv_msg = ts_crypto_DestroyKeyIn_init_default;

    pb_istream_t istream = pb_istream_from_buffer((const uint8_t*)req_buf->data, req_buf->data_len);

    if (pb_decode(&istream, ts_crypto_DestroyKeyIn_fields, &recv_msg)) {

        *handle = recv_msg.handle;
        rpc_status = TS_RPC_CALL_ACCEPTED;
    }

    return rpc_status;
}

/* Operation: open_key */
static rpc_status_t deserialize_open_key_req(const struct call_param_buf *req_buf, psa_key_id_t *id)
{
    rpc_status_t rpc_status = TS_RPC_ERROR_INVALID_REQ_BODY;
    ts_crypto_OpenKeyIn recv_msg = ts_crypto_OpenKeyIn_init_default;

     pb_istream_t istream = pb_istream_from_buffer((const uint8_t*)req_buf->data, req_buf->data_len);

    if (pb_decode(&istream, ts_crypto_OpenKeyIn_fields, &recv_msg)) {

        *id = recv_msg.id;

        rpc_status = TS_RPC_CALL_ACCEPTED;
    }

    return rpc_status;
}

static rpc_status_t serialize_open_key_resp(struct call_param_buf *resp_buf,
                                psa_key_handle_t handle)
{
    size_t packed_resp_size;
    rpc_status_t rpc_status = TS_RPC_ERROR_INTERNAL;
    ts_crypto_OpenKeyOut resp_msg = ts_crypto_OpenKeyOut_init_default;
    resp_msg.handle = handle;

    if (pb_get_encoded_size(&packed_resp_size, ts_crypto_OpenKeyOut_fields, &resp_msg) &&
        (packed_resp_size <= resp_buf->size)) {

        pb_ostream_t ostream = pb_ostream_from_buffer((uint8_t*)resp_buf->data, packed_resp_size);
	    if (pb_encode(&ostream, ts_crypto_OpenKeyOut_fields, &resp_msg)) {

            resp_buf->data_len = packed_resp_size;
            rpc_status = TS_RPC_CALL_ACCEPTED;
        }
    }

    return rpc_status;
}

/* Operation: close_key */
static rpc_status_t deserialize_close_key_req(const struct call_param_buf *req_buf,
                                psa_key_handle_t *handle)
{
    rpc_status_t rpc_status = TS_RPC_ERROR_INVALID_REQ_BODY;
    ts_crypto_CloseKeyIn recv_msg = ts_crypto_CloseKeyIn_init_default;

    pb_istream_t istream = pb_istream_from_buffer((const uint8_t*)req_buf->data, req_buf->data_len);

    if (pb_decode(&istream, ts_crypto_CloseKeyIn_fields, &recv_msg)) {

        *handle = recv_msg.handle;
        rpc_status = TS_RPC_CALL_ACCEPTED;
    }

    return rpc_status;
}

/* Operation: export_key */
static rpc_status_t deserialize_export_key_req(const struct call_param_buf *req_buf,
                                            psa_key_handle_t *handle)
{
    rpc_status_t rpc_status = TS_RPC_ERROR_INVALID_REQ_BODY;
    ts_crypto_ExportKeyIn recv_msg = ts_crypto_ExportKeyIn_init_default;

    pb_istream_t istream = pb_istream_from_buffer((const uint8_t*)req_buf->data, req_buf->data_len);

    if (pb_decode(&istream, ts_crypto_ExportKeyIn_fields, &recv_msg)) {

        *handle = recv_msg.handle;
        rpc_status = TS_RPC_CALL_ACCEPTED;
    }

    return rpc_status;
}

static rpc_status_t serialize_export_key_resp(struct call_param_buf *resp_buf,
                                    const uint8_t *data, size_t data_len)
{
    size_t packed_resp_size;
    rpc_status_t rpc_status = TS_RPC_ERROR_INTERNAL;
    ts_crypto_ExportKeyOut resp_msg = ts_crypto_ExportKeyOut_init_default;
    pb_bytes_array_t *key_buffer = pb_malloc_byte_array(data_len);

    memcpy(&key_buffer->bytes, data, data_len);
    resp_msg.data = pb_out_byte_array(key_buffer);

    if (pb_get_encoded_size(&packed_resp_size, ts_crypto_ExportKeyOut_fields, &resp_msg) &&
        (packed_resp_size <= resp_buf->size)) {

        pb_ostream_t ostream = pb_ostream_from_buffer((uint8_t*)resp_buf->data, packed_resp_size);
	    if (pb_encode(&ostream, ts_crypto_ExportKeyOut_fields, &resp_msg)) {

            resp_buf->data_len = packed_resp_size;
            rpc_status = TS_RPC_CALL_ACCEPTED;
        }
    }

    free(key_buffer);

    return rpc_status;
}

/* Operation: export_public_key */
static rpc_status_t deserialize_export_public_key_req(const struct call_param_buf *req_buf,
                                                psa_key_handle_t *handle)
{
    rpc_status_t rpc_status = TS_RPC_ERROR_INVALID_REQ_BODY;
    ts_crypto_ExportPublicKeyIn recv_msg = ts_crypto_ExportPublicKeyIn_init_default;

    pb_istream_t istream = pb_istream_from_buffer((const uint8_t*)req_buf->data, req_buf->data_len);

    if (pb_decode(&istream, ts_crypto_ExportPublicKeyIn_fields, &recv_msg)) {

        *handle = recv_msg.handle;
        rpc_status = TS_RPC_CALL_ACCEPTED;
    }

    return rpc_status;
}

static rpc_status_t serialize_export_public_key_resp(struct call_param_buf *resp_buf,
                                            const uint8_t *data, size_t data_len)
{
    size_t packed_resp_size;
    rpc_status_t rpc_status = TS_RPC_ERROR_INTERNAL;
    ts_crypto_ExportPublicKeyOut resp_msg = ts_crypto_ExportPublicKeyOut_init_default;

    pb_bytes_array_t *key_buffer = pb_malloc_byte_array(data_len);
    resp_msg.data = pb_out_byte_array(key_buffer);
    memcpy(&key_buffer->bytes, data, data_len);

    if (pb_get_encoded_size(&packed_resp_size, ts_crypto_ExportPublicKeyOut_fields, &resp_msg) &&
        (packed_resp_size <= resp_buf->size)) {

        pb_ostream_t ostream = pb_ostream_from_buffer((uint8_t*)resp_buf->data, packed_resp_size);
	    if (pb_encode(&ostream, ts_crypto_ExportPublicKeyOut_fields, &resp_msg)) {

            resp_buf->data_len = packed_resp_size;
            rpc_status = TS_RPC_CALL_ACCEPTED;
        }
    }

    free(key_buffer);

    return rpc_status;
}

/* Operation: import_key */
static rpc_status_t deserialize_import_key_req(const struct call_param_buf *req_buf,
                    psa_key_attributes_t *attributes, uint8_t *data, size_t *data_len)
{
    rpc_status_t rpc_status = TS_RPC_ERROR_INVALID_REQ_BODY;
    ts_crypto_ImportKeyIn recv_msg = ts_crypto_ImportKeyIn_init_default;

    pb_bytes_array_t *key_buffer = pb_malloc_byte_array(*data_len);
    recv_msg.data = pb_in_byte_array(key_buffer);

    pb_istream_t istream = pb_istream_from_buffer((const uint8_t*)req_buf->data, req_buf->data_len);

    if (pb_decode(&istream, ts_crypto_ImportKeyIn_fields, &recv_msg) &&
        recv_msg.has_attributes &&
        (key_buffer->size <= *data_len)) {

        pb_crypto_provider_translate_key_attributes(attributes, &recv_msg.attributes);

        memcpy(data, &key_buffer->bytes, key_buffer->size);
        *data_len = key_buffer->size;

        rpc_status = TS_RPC_CALL_ACCEPTED;
    }

    free(key_buffer);

    return rpc_status;
}

static rpc_status_t serialize_import_key_resp(struct call_param_buf *resp_buf,
                                        psa_key_handle_t handle)
{
    size_t packed_resp_size;
    rpc_status_t rpc_status = TS_RPC_ERROR_INTERNAL;
    ts_crypto_ImportKeyOut resp_msg = ts_crypto_ImportKeyOut_init_default;
    resp_msg.handle = handle;

    if (pb_get_encoded_size(&packed_resp_size, ts_crypto_ImportKeyOut_fields, &resp_msg) &&
        (packed_resp_size <= resp_buf->size)) {

        pb_ostream_t ostream = pb_ostream_from_buffer((uint8_t*)resp_buf->data, packed_resp_size);
	    if (pb_encode(&ostream, ts_crypto_ImportKeyOut_fields, &resp_msg)) {

            resp_buf->data_len = packed_resp_size;
            rpc_status = TS_RPC_CALL_ACCEPTED;
        }
    }

    return rpc_status;
}

/* Operation: sign_hash */
static rpc_status_t deserialize_sign_hash_req(const struct call_param_buf *req_buf,
                            psa_key_handle_t *handle, psa_algorithm_t *alg,
                            uint8_t *hash, size_t *hash_len)
{
    rpc_status_t rpc_status = TS_RPC_ERROR_INVALID_REQ_BODY;
    ts_crypto_SignHashIn recv_msg = ts_crypto_SignHashIn_init_default;

    pb_bytes_array_t *hash_buffer = pb_malloc_byte_array(*hash_len);
    recv_msg.hash = pb_in_byte_array(hash_buffer);

    pb_istream_t istream = pb_istream_from_buffer((const uint8_t*)req_buf->data, req_buf->data_len);

    if (pb_decode(&istream, ts_crypto_SignHashIn_fields, &recv_msg)) {

        *handle = recv_msg.handle;
        *alg = recv_msg.alg;

        memcpy(hash, &hash_buffer->bytes, hash_buffer->size);
        *hash_len = hash_buffer->size;

        rpc_status = TS_RPC_CALL_ACCEPTED;
    }

    free(hash_buffer);

    return rpc_status;
}

static rpc_status_t serialize_sign_hash_resp(struct call_param_buf *resp_buf,
                            const uint8_t *sig, size_t sig_len)
{
    size_t packed_resp_size;
    rpc_status_t rpc_status = TS_RPC_ERROR_INTERNAL;
    ts_crypto_SignHashOut resp_msg = ts_crypto_SignHashOut_init_default;

    pb_bytes_array_t *sig_buffer = pb_malloc_byte_array(sig_len);
    resp_msg.signature = pb_out_byte_array(sig_buffer);
    memcpy(&sig_buffer->bytes, sig, sig_len);

    if (pb_get_encoded_size(&packed_resp_size, ts_crypto_SignHashOut_fields, &resp_msg) &&
        (packed_resp_size <= resp_buf->size)) {

        pb_ostream_t ostream = pb_ostream_from_buffer((uint8_t*)resp_buf->data, packed_resp_size);
	    if (pb_encode(&ostream, ts_crypto_SignHashOut_fields, &resp_msg)) {

            resp_buf->data_len = packed_resp_size;
            rpc_status = TS_RPC_CALL_ACCEPTED;
        }
    }

    free(sig_buffer);

    return rpc_status;
}

/* Operation: verify_hash */
static rpc_status_t deserialize_verify_hash_req(const struct call_param_buf *req_buf,
                                psa_key_handle_t *handle, psa_algorithm_t *alg,
                                uint8_t *hash, size_t *hash_len,
                                uint8_t *sig, size_t *sig_len)
{
    rpc_status_t rpc_status = TS_RPC_ERROR_INVALID_REQ_BODY;
    ts_crypto_VerifyHashIn recv_msg = ts_crypto_VerifyHashIn_init_default;

    pb_bytes_array_t *hash_buffer = pb_malloc_byte_array(*hash_len);
    recv_msg.hash = pb_in_byte_array(hash_buffer);

    pb_bytes_array_t *sig_buffer = pb_malloc_byte_array(*sig_len);
    recv_msg.signature = pb_in_byte_array(sig_buffer);

    pb_istream_t istream = pb_istream_from_buffer((const uint8_t*)req_buf->data, req_buf->data_len);

    if (pb_decode(&istream, ts_crypto_VerifyHashIn_fields, &recv_msg)) {

        *handle = recv_msg.handle;
        *alg = recv_msg.alg;

        memcpy(hash, &hash_buffer->bytes, hash_buffer->size);
        *hash_len = hash_buffer->size;

        memcpy(sig, &sig_buffer->bytes, sig_buffer->size);
        *sig_len = sig_buffer->size;

        rpc_status = TS_RPC_CALL_ACCEPTED;
    }

    free(hash_buffer);
    free(sig_buffer);

    return rpc_status;
}

/* Operation: asymmetric_decrypt */
static rpc_status_t deserialize_asymmetric_decrypt_req(const struct call_param_buf *req_buf,
                                psa_key_handle_t *handle, psa_algorithm_t *alg,
                                uint8_t *ciphertext, size_t *ciphertext_len,
                                uint8_t *salt, size_t *salt_len)
{
    rpc_status_t rpc_status = TS_RPC_ERROR_INVALID_REQ_BODY;
    ts_crypto_AsymmetricDecryptIn recv_msg = ts_crypto_AsymmetricDecryptIn_init_default;

    pb_bytes_array_t *ciphertext_buffer = pb_malloc_byte_array(*ciphertext_len);
    recv_msg.ciphertext = pb_in_byte_array(ciphertext_buffer);

    pb_bytes_array_t *salt_buffer = pb_malloc_byte_array(*salt_len);
    recv_msg.salt = pb_in_byte_array(salt_buffer);

    pb_istream_t istream = pb_istream_from_buffer((const uint8_t*)req_buf->data, req_buf->data_len);

    if (pb_decode(&istream, ts_crypto_AsymmetricDecryptIn_fields, &recv_msg)) {

        *handle = recv_msg.handle;
        *alg = recv_msg.alg;

        memcpy(ciphertext, &ciphertext_buffer->bytes, ciphertext_buffer->size);
        *ciphertext_len = ciphertext_buffer->size;

        memcpy(salt, &salt_buffer->bytes, salt_buffer->size);
        *salt_len = salt_buffer->size;

        rpc_status = TS_RPC_CALL_ACCEPTED;
    }

    free(ciphertext_buffer);
    free(salt_buffer);

    return rpc_status;
}

static rpc_status_t serialize_asymmetric_decrypt_resp(struct call_param_buf *resp_buf,
                                const uint8_t *plaintext, size_t plaintext_len)
{
    size_t packed_resp_size;
    rpc_status_t rpc_status = TS_RPC_ERROR_INTERNAL;
    ts_crypto_AsymmetricDecryptOut resp_msg = ts_crypto_AsymmetricDecryptOut_init_default;

    pb_bytes_array_t *plaintext_buffer = pb_malloc_byte_array(plaintext_len);
    resp_msg.plaintext = pb_out_byte_array(plaintext_buffer);
    memcpy(&plaintext_buffer->bytes, plaintext, plaintext_len);

    if (pb_get_encoded_size(&packed_resp_size, ts_crypto_AsymmetricDecryptOut_fields, &resp_msg) &&
        (packed_resp_size <= resp_buf->size)) {

        pb_ostream_t ostream = pb_ostream_from_buffer((uint8_t*)resp_buf->data, packed_resp_size);
	    if (pb_encode(&ostream, ts_crypto_AsymmetricDecryptOut_fields, &resp_msg)) {

            resp_buf->data_len = packed_resp_size;
            rpc_status = TS_RPC_CALL_ACCEPTED;
        }
    }

    free(plaintext_buffer);

    return rpc_status;
}

/* Operation: asymmetric_encrypt */
static rpc_status_t deserialize_asymmetric_encrypt_req(const struct call_param_buf *req_buf,
                                    psa_key_handle_t *handle, psa_algorithm_t *alg,
                                    uint8_t *plaintext, size_t *plaintext_len,
                                    uint8_t *salt, size_t *salt_len)
{
    rpc_status_t rpc_status = TS_RPC_ERROR_INVALID_REQ_BODY;
    ts_crypto_AsymmetricEncryptIn recv_msg = ts_crypto_AsymmetricEncryptIn_init_default;

    pb_bytes_array_t *plaintext_buffer = pb_malloc_byte_array(*plaintext_len);
    recv_msg.plaintext = pb_in_byte_array(plaintext_buffer);

    pb_bytes_array_t *salt_buffer = pb_malloc_byte_array(*salt_len);
    recv_msg.salt = pb_in_byte_array(salt_buffer);

    pb_istream_t istream = pb_istream_from_buffer((const uint8_t*)req_buf->data, req_buf->data_len);

    if (pb_decode(&istream, ts_crypto_AsymmetricEncryptIn_fields, &recv_msg)) {

        *handle = recv_msg.handle;
        *alg = recv_msg.alg;

        memcpy(plaintext, &plaintext_buffer->bytes, plaintext_buffer->size);
        *plaintext_len = plaintext_buffer->size;

        memcpy(salt, &salt_buffer->bytes, salt_buffer->size);
        *salt_len = salt_buffer->size;

        rpc_status = TS_RPC_CALL_ACCEPTED;
    }

    free(plaintext_buffer);
    free(salt_buffer);

    return rpc_status;
}

static rpc_status_t serialize_asymmetric_encrypt_resp(struct call_param_buf *resp_buf,
                                    const uint8_t *ciphertext, size_t ciphertext_len)
{
    size_t packed_resp_size;
    rpc_status_t rpc_status = TS_RPC_ERROR_INTERNAL;
    ts_crypto_AsymmetricEncryptOut resp_msg = ts_crypto_AsymmetricEncryptOut_init_default;

    pb_bytes_array_t *ciphertext_buffer = pb_malloc_byte_array(ciphertext_len);
    resp_msg.ciphertext = pb_out_byte_array(ciphertext_buffer);
    memcpy(&ciphertext_buffer->bytes, ciphertext, ciphertext_len);

    if (pb_get_encoded_size(&packed_resp_size, ts_crypto_AsymmetricEncryptOut_fields, &resp_msg) &&
        (packed_resp_size <= resp_buf->size)) {

        pb_ostream_t ostream = pb_ostream_from_buffer((uint8_t*)resp_buf->data, packed_resp_size);
	    if (pb_encode(&ostream, ts_crypto_AsymmetricEncryptOut_fields, &resp_msg)) {

            resp_buf->data_len = packed_resp_size;
            rpc_status = TS_RPC_CALL_ACCEPTED;
        }
    }

    free(ciphertext_buffer);

    return rpc_status;
}

/* Operation: generate_random */
static rpc_status_t deserialize_generate_random_req(const struct call_param_buf *req_buf,
                                        size_t *size)
{
    rpc_status_t rpc_status = TS_RPC_ERROR_INVALID_REQ_BODY;
    ts_crypto_GenerateRandomIn recv_msg = ts_crypto_GenerateRandomIn_init_default;

    pb_istream_t istream = pb_istream_from_buffer((const uint8_t*)req_buf->data, req_buf->data_len);

    if (pb_decode(&istream, ts_crypto_GenerateRandomIn_fields, &recv_msg)) {

        *size = recv_msg.size;

        rpc_status = TS_RPC_CALL_ACCEPTED;
    }

    return rpc_status;
}

static rpc_status_t serialize_generate_random_resp(struct call_param_buf *resp_buf,
                                        const uint8_t *output, size_t output_len)
{
    size_t packed_resp_size;
    rpc_status_t rpc_status = TS_RPC_ERROR_INTERNAL;
    ts_crypto_GenerateRandomOut resp_msg = ts_crypto_GenerateRandomOut_init_default;

    pb_bytes_array_t *output_buffer = pb_malloc_byte_array(output_len);
    resp_msg.random_bytes = pb_out_byte_array(output_buffer);
    memcpy(&output_buffer->bytes, output, output_len);

    if (pb_get_encoded_size(&packed_resp_size, ts_crypto_GenerateRandomOut_fields, &resp_msg) &&
        (packed_resp_size <= resp_buf->size)) {

        pb_ostream_t ostream = pb_ostream_from_buffer((uint8_t*)resp_buf->data, packed_resp_size);
	    if (pb_encode(&ostream, ts_crypto_GenerateRandomOut_fields, &resp_msg)) {

            resp_buf->data_len = packed_resp_size;
            rpc_status = TS_RPC_CALL_ACCEPTED;
        }
    }

    free(output_buffer);

    return rpc_status;
}

/* Singleton method to provide access to the serializer instance */
const struct crypto_provider_serializer *pb_crypto_provider_serializer_instance(void)
{
    static const struct crypto_provider_serializer instance = {
        max_deserialised_parameter_size,
        deserialize_generate_key_req,
        serialize_generate_key_resp,
        deserialize_destroy_key_req,
        deserialize_open_key_req,
        serialize_open_key_resp,
        deserialize_close_key_req,
        deserialize_export_key_req,
        serialize_export_key_resp,
        deserialize_export_public_key_req,
        serialize_export_public_key_resp,
        deserialize_import_key_req,
        serialize_import_key_resp,
        deserialize_sign_hash_req,
        serialize_sign_hash_resp,
        deserialize_verify_hash_req,
        deserialize_asymmetric_decrypt_req,
        serialize_asymmetric_decrypt_resp,
        deserialize_asymmetric_encrypt_req,
        serialize_asymmetric_encrypt_resp,
        deserialize_generate_random_req,
        serialize_generate_random_resp
    };

    return &instance;
}
