/*
 * Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PSA_IPC_CRYPTO_CALLER_H
#define PSA_IPC_CRYPTO_CALLER_H

/**
 * Includes all header files that form the psa ipc crypto caller
 * interface.  May be used by a client that needs to call operations
 * provided by a crypto service instance using the psa ipc interface.
 */
#include "crypto_caller_aead.h"
#include "crypto_caller_asymmetric_decrypt.h"
#include "crypto_caller_asymmetric_encrypt.h"
#include "crypto_caller_cipher.h"
#include "crypto_caller_copy_key.h"
#include "crypto_caller_destroy_key.h"
#include "crypto_caller_export_key.h"
#include "crypto_caller_export_public_key.h"
#include "crypto_caller_generate_key.h"
#include "crypto_caller_generate_random.h"
#include "crypto_caller_get_key_attributes.h"
#include "crypto_caller_get_uefi_priv_auth_var_fingerprint.h"
#include "crypto_caller_hash.h"
#include "crypto_caller_import_key.h"
#include "crypto_caller_key_derivation.h"
#include "crypto_caller_mac.h"
#include "crypto_caller_purge_key.h"
#include "crypto_caller_sign_hash.h"
#include "crypto_caller_verify_hash.h"
#include "crypto_caller_verify_pkcs7_signature.h"

#endif /* PSA_IPC_CRYPTO_CALLER_H */
