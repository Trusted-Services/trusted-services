/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 */

#ifndef TPM_CRB_PROVIDER_H
#define TPM_CRB_PROVIDER_H

#include "components/rpc/common/endpoint/rpc_service_interface.h"
#include "service/common/provider/service_provider.h"
#include "service/tpm/backend/ms_tpm/ms_tpm_backend.h"
#include "compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

struct loc_and_crb_ctrl;

struct tpm_crb_provider {
	struct service_provider base_provider;
	struct loc_and_crb_ctrl *loc_ptr[5];
};

struct rpc_service_interface *tpm_provider_init(struct tpm_crb_provider *context,
						uint8_t *ns_crb, size_t ns_crb_size,
						uint8_t* s_crb, size_t s_crb_size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TPM_CRB_PROVIDER_H */
