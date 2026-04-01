/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 */

#ifndef MS_TPM_BACKEND_H
#define MS_TPM_BACKEND_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>   // for size_t
#include <stdint.h>   // for uint8_t
#include <stdbool.h>  // for bool

/**
 * \brief Execute the command serialized to req, and return a response.
 *
 * \param[in]      req			Pointer to command buffer.
 * \param[in]      req_len		Length of command buffer.
 * \param[out]     resp			Pointer to response.
 * \param[in/out]  resp_len		Length of response.
 * \param[in/out]  resp_max_size	Size of response buffer.
 */
void ms_tpm_backend_execute_command(const uint8_t *req, size_t req_len, uint8_t **resp,
				    size_t *resp_len, size_t resp_max_size);

/**
 * \brief Initialize the backend.
 *
 * \return false on success and true on error
 */
bool ms_tpm_backend_init(void);

/**
 * \brief Replay the PCR update events of the supplied Event Log.
 *
 * Process an Event Log formatted as described by the TCG PC Client Platform
 * Firmware Profile Specification, and replay PCR update events in to the TPM.
 *
 * Assumptions:
 * - PCR registers can be written with empty password authentication.
 * - The service uses a locality which can write all PCR registers.
 * - All PCR banks mentioned in the log are enabled.
 *
 * If the integration violates these assumptions, some PCR updates may have no
 *      effect, or the replay may fail.
 *
 * The function will search the Config Store for the event log. A missing log
 * is not an error. If the log is present but corrupt the SP will enter a
 * fatal error and crash.
 *
 * \param[in]   log_buffer              Pointer to event log data. \param[in]
 * log_buffer_length Number of bytes in the event log
 *
 * \return false on success and true on error
 */
bool ms_tpm_backend_replay_eventlog(const uint8_t *log_buffer, const size_t log_buffer_length);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MS_TPM_BACKEND_H */
