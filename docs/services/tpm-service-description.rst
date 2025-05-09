TPM 2.0 service
===============
Overview
--------
A TPM is a Root of Trust component that is standardized by the Trusted Computing Group in the
`TPM specification`_. The Trusted Services project implements the following TPM-related
components:

  * TPM Service CRB Interface Over FF-A (`DEN0138`_) RPC layer,
  * TPM CRB provider,
  * TPM backend using `ms-tpm-20-ref`_ library.

These components are integrated as a Secure Partition in the fTPM deployment, which provides
standard TPM 2.0 functionality to clients both in Normal World and Secure World. This service is
suitable for platforms without discrete TPM hardware, while maintaining compatibility with software
stacks expecting a standard TPM CRB interface.

Communication with the fTPM follows the DEN0138 protocol, using shared memory for the CRB registers
and FF-A direct messaging for signaling.

.. image:: image/tpm-components.svg

TPM CRB over FF-A RPC endpoint
------------------------------
Location: ``components/rpc/tpm_crb_ffa/endpoint/sp/tpm_crb_ffa_endpoint.c``

This component handles the incoming FF-A direct requests and parses them according to the DEN0138
specification. The functions defined by this specification fall into two categories:

  * Management type functions, for controlling the TPM service frontend in the SP.
  * Start function, to signal to the TPM backend that a locality request or command available in the
    CRB should be processed.

The management type functions are handled in this component, while the start function is passed to
the TPM CRB provider.

TPM CRB provider
----------------
Location: ``components/service/tpm/provider/tpm_crb_provider.c``

This component provides the core logic for handling TPM CRB memory-mapped register access, it
interprets CRB register state transitions. The two main types of events to handle are:

  * Locality request: a client requests or relinquishes access to a locality.
  * Command request: a client has placed a TPM command into a locality and requires processing of
    the command.

The locality requests are handled in this components, while the command request is passed to the
TPM backend.

TPM backend
-----------
Location: ``components/service/tpm/backend/ms_tpm/ms_tpm_backend.c``

Currently a single type of TPM backend is supported, which is based on the ms-tpm-20-ref library.
This is responsible for:

  * Initial provisioning using ``TPM_Manufacture()``.
  * Startup and runtime initialization of the TPM.
  * Command execution: the memory buffer from the CRB which contains the incoming command is passed
    to the library using ``ExecuteCommand()``, writing the response back to the same memory buffer.

The platform, crypto library and build system related modifications for ms-tpm-20-ref are
implemented in patch files found at ``external/ms_tpm/*.patch``. Summary of the modifications:

  * CMake support: adds a simple CMake build system to the project which can compile the necessary
    files into a static library an install it as a CMake package.
  * Mbed TLS support: adds support for using hash, symmetric crypto and bignum operations from Mbed
    TLS instead of OpenSSL.
  * Platform porting: implements a minimal necessary platform layer to satisfy the requirements of
    ms-tpm-20-ref. This includes:

    * Non-volatile storage using PSA Storage API.
    * Get entropy using Mbed TLS.

Limitations
-----------

  * Handling of Locality 4 commands is not implemented.
  * Hardware clock usage is not implemented, a software counter is used instead that's incremented
    on each query.
  * Handling of early boot measurements (from BL1 and BL2) is not implemented.

--------------

.. _`TPM specification`: https://trustedcomputinggroup.org/resource/tpm-library-specification/
.. _`DEN0138`: https://developer.arm.com/documentation/den0138/latest
.. _`ms-tpm-20-ref`: https://github.com/microsoft/ms-tpm-20-ref

*Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.*

SPDX-License-Identifier: BSD-3-Clause
