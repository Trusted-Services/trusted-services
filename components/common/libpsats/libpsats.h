/*
 * Copyright (c) 2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "psa/crypto_types.h"
#include "psa/initial_attestation.h"
#include "psa/internal_trusted_storage.h"
#include "psa/protected_storage.h"

#ifndef LIBPSATS_H
#define LIBPSATS_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The functions may be exported as a public interface to
 * a shared library.
 */
#ifdef EXPORT_PUBLIC_INTERFACE_LIBPSATS
#define LIBPSATS_EXPORTED __attribute__((__visibility__("default")))
#else
#define LIBPSATS_EXPORTED
#endif

LIBPSATS_EXPORTED psa_status_t libpsats_init_crypto_context(const char *service_name);
LIBPSATS_EXPORTED void libpsats_deinit_crypto_context(void);

LIBPSATS_EXPORTED psa_status_t libpsats_init_attestation_context(const char *service_name);
LIBPSATS_EXPORTED void libpsats_deinit_attestation_context(void);

LIBPSATS_EXPORTED psa_status_t libpsats_init_its_context(const char *service_name);
LIBPSATS_EXPORTED void libpsats_deinit_its_context(void);

LIBPSATS_EXPORTED psa_status_t libpsats_init_ps_context(const char *service_name);
LIBPSATS_EXPORTED void libpsats_deinit_ps_context(void);

#ifdef __cplusplus
}
#endif

#endif /* LIBPSATS_H */
