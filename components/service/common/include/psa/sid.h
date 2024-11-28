/*
 * Copyright (c) 2019-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __PSA_MANIFEST_SID_H__
#define __PSA_MANIFEST_SID_H__

#ifdef __cplusplus
extern "C" {
#endif

/******** PSA Crypto SIDs ********/
#include "crypto_sid.h"

/******** TFM_SP_PS ********/
#define TFM_PROTECTED_STORAGE_SERVICE_SID                          (0x00000060U)
#define TFM_PROTECTED_STORAGE_SERVICE_VERSION                      (1U)
#define TFM_PROTECTED_STORAGE_SERVICE_HANDLE                       (0x40000101U)

/* Invalid UID */
#define TFM_PS_INVALID_UID 0

/* PS / ITS message types that distinguish PS services. */
#define TFM_PS_ITS_SET                1001
#define TFM_PS_ITS_GET                1002
#define TFM_PS_ITS_GET_INFO           1003
#define TFM_PS_ITS_REMOVE             1004
#define TFM_PS_ITS_GET_SUPPORT        1005

/******** TFM_SP_ITS ********/
#define TFM_INTERNAL_TRUSTED_STORAGE_SERVICE_SID                   (0x00000070U)
#define TFM_INTERNAL_TRUSTED_STORAGE_SERVICE_VERSION               (1U)
#define TFM_INTERNAL_TRUSTED_STORAGE_SERVICE_HANDLE                (0x40000102U)

/******** TFM_SP_CRYPTO ********/
#define TFM_CRYPTO_SID                                             (0x00000080U)
#define TFM_CRYPTO_VERSION                                         (1U)
#define TFM_CRYPTO_HANDLE                                          (0x40000100U)

/******** TFM_SP_PLATFORM ********/
#define TFM_SP_PLATFORM_SYSTEM_RESET_SID                           (0x00000040U)
#define TFM_SP_PLATFORM_SYSTEM_RESET_VERSION                       (1U)
#define TFM_SP_PLATFORM_IOCTL_SID                                  (0x00000041U)
#define TFM_SP_PLATFORM_IOCTL_VERSION                              (1U)
#define TFM_SP_PLATFORM_NV_COUNTER_SID                             (0x00000042U)
#define TFM_SP_PLATFORM_NV_COUNTER_VERSION                         (1U)

/******** TFM_SP_INITIAL_ATTESTATION ********/
#define TFM_ATTESTATION_SERVICE_SID                                (0x00000020U)
#define TFM_ATTESTATION_SERVICE_VERSION                            (1U)
#define TFM_ATTESTATION_SERVICE_HANDLE                             (0x40000103U)

/* Initial Attestation message types that distinguish Attest services. */
#define TFM_ATTEST_GET_TOKEN       1001
#define TFM_ATTEST_GET_TOKEN_SIZE  1002

/******** TFM_SP_FWU ********/
#define TFM_FIRMWARE_UPDATE_SERVICE_SID                            (0x000000A0U)
#define TFM_FIRMWARE_UPDATE_SERVICE_VERSION                        (1U)
#define TFM_FIRMWARE_UPDATE_SERVICE_HANDLE                         (0x40000104U)

#define TFM_FWU_WRITE_SID                                          (0x000000A0U)
#define TFM_FWU_WRITE_VERSION                                      (1U)
#define TFM_FWU_INSTALL_SID                                        (0x000000A1U)
#define TFM_FWU_INSTALL_VERSION                                    (1U)
#define TFM_FWU_ABORT_SID                                          (0x000000A2U)
#define TFM_FWU_ABORT_VERSION                                      (1U)
#define TFM_FWU_QUERY_SID                                          (0x000000A3U)
#define TFM_FWU_QUERY_VERSION                                      (1U)
#define TFM_FWU_REQUEST_REBOOT_SID                                 (0x000000A4U)
#define TFM_FWU_REQUEST_REBOOT_VERSION                             (1U)
#define TFM_FWU_ACCEPT_SID                                         (0x000000A5U)
#define TFM_FWU_ACCEPT_VERSION                                     (1U)

#ifdef __cplusplus
}
#endif

#endif /* __PSA_MANIFEST_SID_H__ */
