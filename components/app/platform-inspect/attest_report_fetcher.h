/*
 * Copyright (c) 2021-2023, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ATTEST_REPORT_FETCHER_H
#define ATTEST_REPORT_FETCHER_H

#include <cstdint>
#include <string>
#include <vector>

/** \brief Fetch and verify an attestaton report
 *
 * \param[out] report       The CBOR encoded report
 * \param[out] error_msg    Error message on failure
 *
 * \return Returns true if fetch successful
 */
bool fetch_and_verify(std::vector<uint8_t> &report, std::string &error_msg);

#endif /* ATTEST_REPORT_FETCHER_H */
