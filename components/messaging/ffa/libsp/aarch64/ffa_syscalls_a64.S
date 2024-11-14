/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2020, Arm Limited and Contributors. All rights reserved.
 */

#include <asm.S>

	.section .text

	/*
	 * The C function prototype is defined in ffa_internal_api.h
	 *
	 * void ffa_svc(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3,
	 *		uint64_t a4, uint64_t a5, uint64_t a6, uint64_t a7,
	 *		struct ffa_params *result);
	 */

	FUNC ffa_svc , :
	svc #0
	/* Store ERET args to struct ffa_params ptr */
	ldr x8, [sp]
	stp	x0, x1, [x8, #0]
	stp	x2, x3, [x8, #16]
	stp	x4, x5, [x8, #32]
	stp	x6, x7, [x8, #48]
	ret
	END_FUNC ffa_svc

BTI(emit_aarch64_feature_1_and     GNU_PROPERTY_AARCH64_FEATURE_1_BTI)
