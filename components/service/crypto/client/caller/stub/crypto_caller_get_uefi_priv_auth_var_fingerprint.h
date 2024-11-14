/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef STUB_CRYPTO_CALLER_GET_UEFI_PRIV_AUTH_VAR_FINGERPRINT_H
#define STUB_CRYPTO_CALLER_GET_UEFI_PRIV_AUTH_VAR_FINGERPRINT_H

#include <rpc/common/interface/rpc_status.h>
#include <service/common/client/service_client.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int crypto_caller_get_uefi_priv_auth_var_fingerprint(struct service_client *context,
								   const uint8_t *signature_cert,
								   uint64_t signature_cert_len,
								   uint8_t *output)
{
	(void)context;
	(void)signature_cert;
	(void)signature_cert_len;
	(void)output;

	return RPC_ERROR_INTERNAL;
}

#ifdef __cplusplus
}
#endif

#endif /* STUB_CRYPTO_CALLER_GET_UEFI_PRIV_AUTH_VAR_FINGERPRINT_H */
