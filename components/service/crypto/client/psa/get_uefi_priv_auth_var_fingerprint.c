/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "crypto_caller_selector.h"
#include "crypto_client.h"
#include "psa_crypto_client.h"

int get_uefi_priv_auth_var_fingerprint_handler(const uint8_t *signature_cert,
				    uint64_t signature_cert_len,
				    uint8_t *output)
{
	if (psa_crypto_client_instance.init_status != PSA_SUCCESS)
		return psa_crypto_client_instance.init_status;

	return crypto_caller_get_uefi_priv_auth_var_fingerprint(&psa_crypto_client_instance.base,
						    signature_cert, signature_cert_len,
						    output);
}
