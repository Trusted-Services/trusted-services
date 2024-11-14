/*
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FWU_PROTO_STATUS_H
#define FWU_PROTO_STATUS_H

/**
 * Service-level status codes
 */
#define FWU_STATUS_SUCCESS	 ((int32_t)0)
#define FWU_STATUS_UNKNOWN	 ((int32_t)-1)
#define FWU_STATUS_BUSY		 ((int32_t)-2)
#define FWU_STATUS_OUT_OF_BOUNDS ((int32_t)-3)
#define FWU_STATUS_AUTH_FAIL	 ((int32_t)-4)
#define FWU_STATUS_NO_PERMISSION ((int32_t)-5)
#define FWU_STATUS_DENIED	 ((int32_t)-6)
#define FWU_STATUS_RESUME	 ((int32_t)-7)
#define FWU_STATUS_NOT_AVAILABLE ((int32_t)-8)

#endif /* FWU_PROTO_STATUS_H */
