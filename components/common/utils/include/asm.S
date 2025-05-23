/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 * Copyright (c) 2014, STMicroelectronics International N.V.
 */

#if defined(BTI_ENABLED)
#include <arm64_bti.S>

#define BTI(...) __VA_ARGS__
#else
#define BTI(...)
#endif

	.macro FUNC name colon section=default
	.ifc	\section\(),default
	.section .text.\name
	.else
	.section \section , "ax" , %progbits
	.endif
	.global \name
	.type \name , %function
	.balign 4
	\name \colon
	BTI(bti c)
	.endm

	.macro DATA name colon
	.global \name
	.type \name , %object
	\name \colon
	.endm

	.macro LOCAL_FUNC name colon section=default
	.ifc	\section\(),default
	.section .text.\name
	.else
	.section \section , "ax" , %progbits
	.endif
	.type \name , %function
	.balign 4
	\name \colon
	BTI(bti c)
	.endm

	.macro LOCAL_DATA name colon
	.type \name , %object
	\name \colon
	.endm

	.macro END_DATA name
	.size \name , .-\name
	.endm

	.macro END_FUNC name
	.size \name , .-\name
	.endm
