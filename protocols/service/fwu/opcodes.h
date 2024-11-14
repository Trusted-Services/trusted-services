/*
 * Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FWU_PROTO_OPCODES_H
#define FWU_PROTO_OPCODES_H

/**
 * Service-level opcodes
 */
#define FWU_FUNC_ID_DISCOVER		(0)
#define FWU_FUNC_ID_BEGIN_STAGING	(16)
#define FWU_FUNC_ID_END_STAGING	(17)
#define FWU_FUNC_ID_CANCEL_STAGING	(18)
#define FWU_FUNC_ID_OPEN		(19)
#define FWU_FUNC_ID_WRITE_STREAM	(20)
#define FWU_FUNC_ID_READ_STREAM	(21)
#define FWU_FUNC_ID_COMMIT		(22)
#define FWU_FUNC_ID_ACCEPT_IMAGE	(23)
#define FWU_FUNC_ID_SELECT_PREVIOUS	(24)

#define FWU_FUNC_ID_COUNT		(10)

#endif /* FWU_PROTO_OPCODES_H */
