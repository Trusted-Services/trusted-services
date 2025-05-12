// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 */

#include "tpm_crb_ffa.h"

uint64_t tpm_crb_ffa_get_function_id(const uint64_t regs[5])
{
	return regs[1];
}

uint8_t tpm_crb_ffa_get_start_qualifier(const uint64_t regs[5])
{
	return regs[2];
}

uint8_t tpm_crb_ffa_get_start_loc_qualifier(const uint64_t regs[5])
{
	return regs[3];
}

void tpm_crb_ffa_set_status(uint64_t regs[5], uint64_t status)
{
	regs[1] = status;
}

void tpm_crb_ffa_set_interface_version(uint64_t regs[5], uint64_t version)
{
	regs[2] = version;
}
