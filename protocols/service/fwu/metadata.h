/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * FWU metadata information as per the specification section 4.1:
 * https://developer.arm.com/documentation/den0118/a/
 *
 */

#ifndef FWU_PROTO_METADATA_H
#define FWU_PROTO_METADATA_H

/**
 * The number of banks for different versions of firmware.
 * With the default configuration, a dual bank A/B scheme is used.
 */
#ifndef FWU_METADATA_NUM_BANKS
#define FWU_METADATA_NUM_BANKS (2)
#endif

#endif /* FWU_PROTO_METADATA_H */
