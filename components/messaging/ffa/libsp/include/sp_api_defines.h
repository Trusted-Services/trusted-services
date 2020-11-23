/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2020, Arm Limited and Contributors. All rights reserved.
 */

#ifndef LIBSP_INCLUDE_SP_API_DEFINES_H_
#define LIBSP_INCLUDE_SP_API_DEFINES_H_

/**
 * SP result values are integers where the zero value indicates success and
 * negative values are error codes. The first part of the error code range is
 * mapped directly to the FF-A error code range so the SP result type is able
 * to propagate FF-A error codes. The second part of the range is for SP layer
 * specific error codes. This ranges starts immediately after the value of the
 * last (lowest value) FF-A error code.
 */

#define SP_RESULT_OFFSET	(FFA_ABORTED)
#define SP_RESULT_CREATE(x)	(SP_RESULT_OFFSET + (x))

/** SP API call result codes */
#define SP_RESULT_OK			(0)
#define SP_RESULT_FFA(res)		(res)

#endif /* LIBSP_INCLUDE_SP_API_DEFINES_H_ */