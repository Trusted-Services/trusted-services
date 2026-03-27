#-------------------------------------------------------------------------------
# Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
# Defines an infrastructure for the attestation service provider that uses a
# a TPM eventlog to collect claims about the booted firmware. Uses PSA crypto
# for EAT token signing.
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
# Infrastructure components
#
#-------------------------------------------------------------------------------
add_components(TARGET "attestation"
	BASE_DIR ${TS_ROOT}
	COMPONENTS
		"components/common/uuid"
		"components/common/event_log"
		"components/rpc/ts_rpc/caller/sp"
		"components/rpc/common/caller"
		"components/service/common/client"
		"components/service/locator"
		"components/service/locator/interface"
		"components/service/locator/sp"
		"components/service/locator/sp/ffa"
		"components/service/attestation/claims/sources/boot_seed_generator"
		"components/service/attestation/claims/sources/null_lifecycle"
		"components/service/attestation/claims/sources/instance_id"
		"components/service/attestation/claims/sources/implementation_id"
		"components/service/attestation/claims/sources/event_log"
		"components/service/attestation/claims/sources/event_log/mock"
		"components/service/crypto/include"
		"components/service/crypto/client/psa"
)
