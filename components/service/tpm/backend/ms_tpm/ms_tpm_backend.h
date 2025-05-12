/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 */

#ifndef MS_TPM_BACKEND_H
#define MS_TPM_BACKEND_H

#ifdef __cplusplus
extern "C" {
#endif

void ms_tpm_backend_execute_command(const uint8_t *req, size_t req_len, uint8_t **resp,
				    size_t *resp_len, size_t resp_max_size);
bool ms_tpm_backend_init(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MS_TPM_BACKEND_H */
