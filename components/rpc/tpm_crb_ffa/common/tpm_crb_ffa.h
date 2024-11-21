/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 */

#ifndef TPM_CRB_FFA_H
#define TPM_CRB_FFA_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TPM CRB over FF-A definitions based on DEN0138 v1.0 BET */

#define TPM_CRB_FFA_UUID \
{ 0x17, 0xb8, 0x62, 0xa4, 0x18, 0x06, 0x4f, 0xaf, 0x86, 0xb3, 0x08, 0x9a, 0x58, 0x35, 0x38, 0x61 }

/* The interface in this version of the specification is version (1, 0) */
#define TPM_INTERFACE_VERSION			0x00010000u

/* TPM service function IDs */
#define TPM_FUNC_GET_INTERFACE_VERSION		0x0f000001u
#define TPM_FUNC_GET_FEATURE_INFO		0x0f000101u
#define TPM_FUNC_START				0x0f000201u
#define TPM_FUNC_REGISTER_FOR_NOTIFICATION	0x0f000301u
#define TPM_FUNC_UNREGISTER_FROM_NOTIFICATION	0x0f000401u
#define TPM_FUNC_FINISH_NOTIFIED		0x0f000501u

/* TPM service function status codes */
/* Function succeeded */
#define TPM_STATUS_OK				0x05000001u
/* Function succeeded and results have been returned */
#define TPM_STATUS_OK_RESULTS_RETURNED		0x05000002u
/* No such function */
#define TPM_ERROR_NOFUNC			0x8e000001u
/* Function not supported */
#define TPM_ERROR_NOTSUP			0x8e000002u
/* Invalid argument */
#define TPM_ERROR_INVARG			0x8e000005u
/* Invalid Command-Response Buffer control data */
#define TPM_ERROR_INV_CRB_CTRL_DATA		0x8e000006u
/* This request has already been carried out */
#define TPM_ERROR_ALREADY			0x8e000009u
/* Operation not allowed in the current state */
#define TPM_ERROR_DENIED			0x8e00000au
/* Not enough available memory */
#define TPM_ERROR_NOMEM				0x8e00000bu

/* TPM service features */
/* Client notification of TPM service events through FF-A Notification */
#define TPM_SERVICE_FEATURE_NOTIFICATION	0xfea70000u

/* A command is ready to be processed */
#define TPM_START_QUALIFIER_COMMAND		0
/* A locality request is ready to be processed */
#define TPM_START_QUALIFIER_LOCALITY_REQ	1

uint64_t tpm_crb_ffa_get_function_id(const uint64_t regs[5]);
uint8_t tpm_crb_ffa_get_start_qualifier(const uint64_t regs[5]);
uint8_t tpm_crb_ffa_get_start_loc_qualifier(const uint64_t regs[5]);
void tpm_crb_ffa_set_status(uint64_t regs[5], uint64_t status);
void tpm_crb_ffa_set_interface_version(uint64_t regs[5], uint64_t version);

#ifdef __cplusplus
}
#endif

#endif /* TPM_CRB_FFA_H */
