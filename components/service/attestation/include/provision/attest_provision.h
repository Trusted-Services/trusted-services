/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stddef.h>
#include <stdint.h>
#include <psa/error.h>

#ifndef ATTEST_PROVISION_H
#define ATTEST_PROVISION_H

/**
 * A provisioning client API for perfoming one-off provisioning
 * operations related to the attestation service.  This API will typically
 * be used by a special factory application during device manufacture.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Export IAK public key
 *
 * Used to retrieve the IAK public key that corresponds to the key-pair
 * that was generated or provisioned for the device.  The public key
 * may be used by a remote verifier as an identifier for the device.
 *
 * \param[out] data         Buffer where the key data is to be written.
 * \param data_size         Size of the \p data buffer in bytes.
 * \param[out] data_length  On success, the number of bytes
 *                          that make up the key data.
 *
 * \return Returns error code as specified in \ref psa_status_t
 */
psa_status_t attest_provision_export_iak_public_key(
    uint8_t *data,
    size_t data_size,
    size_t *data_length);

/**
 * \brief Import IAK
 *
 * Used during device manufacture to provision the IAK.  Two IAK
 * provisioning strategies are supported 1) Externally generated
 * key-pair that is provisioned using this interface.  2) Self
 * generated where the IAK is generated by the device autonomously.
 * If a key is to be imported, the operation must be performed before
 * any other operation related to the attestation service.  This
 * operation may only be performed once for a device.  An attempt
 * to repeat the operation will be rejected.
 *
 * \param[in] data    Buffer containing the key data.
 * \param[in] data_length Size of the \p data buffer in bytes.
 *
 * \return Returns error code as specified in \ref psa_status_t
 */
psa_status_t attest_provision_import_iak(
    const uint8_t *data,
    size_t data_length);


#ifdef __cplusplus
}
#endif

#endif /* ATTEST_PROVISION_H */
