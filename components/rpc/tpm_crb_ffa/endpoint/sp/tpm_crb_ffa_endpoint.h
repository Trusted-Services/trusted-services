/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 */

#ifndef TPM_CRB_FFA_ENDPOINT_H
#define TPM_CRB_FFA_ENDPOINT_H

#include "components/rpc/common/endpoint/rpc_service_interface.h"
#include "sp_messaging.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tpm_crb_ffa_ep {
	struct rpc_service_interface *service;
};

bool tpm_crb_ffa_endpoint_init(struct tpm_crb_ffa_ep *endpoint);
bool tpm_crb_ffa_endpoint_add_service(struct tpm_crb_ffa_ep *endpoint,
				      struct rpc_service_interface *service);
void tpm_crb_ffa_endpoint_receive(struct tpm_crb_ffa_ep *endpoint, const struct sp_msg *request,
				  struct sp_msg *response);

#ifdef __cplusplus
}
#endif

#endif /* TPM_CRB_FFA_ENDPOINT_H */
