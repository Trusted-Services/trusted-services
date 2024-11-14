// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021-2024, Arm Limited and Contributors. All rights reserved.
 */

#include <stddef.h>
#include <stdint.h>
#include "compiler.h"
#include "sp_api.h"

/*
 * According to the FF-A specification an optional initialization descriptor can
 * be passed to the SP in w0/x0-w3/x3 registers (a0-a3 parameters). As the exact
 * register is implementation defined the first four registers are forwarded to
 * the user code.
 */
void __noreturn _sp_entry(uintptr_t a0, uintptr_t a1,
			   uintptr_t a2, uintptr_t a3);
void __noreturn _sp_entry(uintptr_t a0, uintptr_t a1,
			   uintptr_t a2, uintptr_t a3)
{
	(void)a1;
	(void)a2;
	(void)a3;

	sp_main((union ffa_boot_info *)a0);
}
