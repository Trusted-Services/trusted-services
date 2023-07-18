/*
 * Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "psa_crypto_client.h"

#include <service/crypto/include/psa/crypto.h>
#include <stddef.h>

struct psa_crypto_client psa_crypto_client_instance = {

	.base.session = NULL,

	/* To conform to PSA API, psa_crypto_init needs to be called.
	 * This state variable is used enforces this.
	 */
	.init_status = PSA_ERROR_BAD_STATE
};

psa_status_t psa_crypto_init(void)
{
	/* Must be called after psa_crypto_client_init */
	if (psa_crypto_client_instance.base.session)
		psa_crypto_client_instance.init_status = PSA_SUCCESS;

	return psa_crypto_client_instance.init_status;
}

psa_status_t psa_crypto_client_init(struct rpc_caller_session *session)
{
	return service_client_init(&psa_crypto_client_instance.base, session);
}

void psa_crypto_client_deinit(void)
{
	service_client_deinit(&psa_crypto_client_instance.base);
	psa_crypto_client_instance.init_status = PSA_ERROR_BAD_STATE;
}

int psa_crypto_client_rpc_status(void)
{
	return psa_crypto_client_instance.base.rpc_status;
}
