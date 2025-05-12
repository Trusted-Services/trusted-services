Secure Partition Images
=======================
Secure partition (SP) deployments are concerned with building SP images that can
be loaded and run under a secure partition manager such as Hafnium or OP-TEE.
SP images will usually include service provider components that expose a
service interface that may be reached using FF-A messages. A set of SP images
will be loaded and verified by device firmware to provide the required services.

The following deployments that create SP images are currently supported:

crypto
------
An instance of the crypto service provider is built into an SP image to
perform cryptographic operations on behalf of clients running in different
partitions. Backend crypto operations are implemented by the crypto library
component of MbedTLS. This deployment provides the cryptographic facilities
needed for PSA certification. For more information, see:
:ref:`Crypto Service`.

.. list-table::
  :widths: 1 2
  :header-rows: 0

  * - Supported Environments
    - * *opteesp* (runs as an S-EL0 SP under OP-TEE)
      * *sp* (SPMC agnostic S-EL0 SP format)
  * - External Dependencies
    - * | TRNG (platform specific)
      * | Secure storage SP

attestation
-----------
An instance of the attestation service provider is built into an SP image
to support remote attestation use-cases. The service provider obtains a
trusted view of the boot state of device firmware from the TPM event log
collected by the boot loader. This deployment provides the initial attestation
facility needed for PSA certification. For more information, see:
:ref:`Attestation Service`.

.. list-table::
  :widths: 1 2
  :header-rows: 0

  * - Supported Environments
    - * *opteesp* (runs as an S-EL0 SP under OP-TEE)
      * *sp* (SPMC agnostic S-EL0 SP format)
  * - External Dependencies
    - * | TPM Event Log (via SP boot parameter)
      * | Crypto SP

internal-trusted-storage & protected-storage
--------------------------------------------
Two secure storage SP deployments are provided to allow different classes
of storage to coexist on a device. Both deployments build an instance of
the secure storage service provider with a storage backend. To allow
different security trade-offs to be made and to support different hardware,
a system integrator may configure which storage backend to use. Secure storage
is a requirement for PSA certification. For more information, see:
:ref:`Secure Storage Service`.

.. list-table::
  :widths: 1 2
  :header-rows: 0

  * - Supported Environments
    - * *opteesp* (runs as an S-EL0 SP under OP-TEE)
      * *sp* (SPMC agnostic S-EL0 SP format)
  * - External Dependencies
    - * Depends on configured storage backend

se-proxy
--------
The se-proxy SP provides access to services hosted by a secure enclave (hence
'se'). A secure enclave consists of a separate MCU, connected to the host via
a secure communications channel. To protect access to the communication channel,
the se-proxy SP is assigned exclusive access to the communication peripheral via
device or memory regions defined in the SP manifest. The deployment integrates
multiple service providers into the SP image. After performing access control,
service requests are forwarded to the secure enclave.

The se-proxy deployment includes proxies for the following services:

  - Crypto
  - Attestation
  - Internal Trusted Storage
  - Protected Storage

.. list-table::
  :widths: 1 2
  :header-rows: 0

  * - Supported Environments
    - * *opteesp* (runs as an S-EL0 SP under OP-TEE)
      * *sp* (SPMC agnostic S-EL0 SP format)
  * - External Dependencies
    - * SE communication peripheral (platform specific)

smm-gateway
-----------
An instance of the smm-variable service provider is built into the smm-gateway SP
image to provide secure world backing for UEFI SMM services. The smm-gateway SP
provides a lightweight alternative to StMM. For more information, see:
:ref:`UEFI SMM Services`.

.. list-table::
  :widths: 1 2
  :header-rows: 0

  * - Supported Environments
    - * *opteesp* (runs as an S-EL0 SP under OP-TEE)
      * *sp* (SPMC agnostic S-EL0 SP format)
  * - External Dependencies
    - * | Secure storage service instance (e.g. hosted by protected-storage SP)
      * | Crypto service instance (e.g. hosted crypto SP)

ftpm
----
The fTPM deployment provides a software TPM 2.0 implementation in an SP, based
on the `ms-tpm-20-ref` reference implementation. It exposes a TPM CRB interface
over FF-A, conforming to the `Arm DEN0138` specification. For more information,
see: :ref:`TPM 2.0 service`.

.. list-table::
  :widths: 1 2
  :header-rows: 0

  * - Supported Environments
    - * *opteesp* (runs as an S-EL0 SP under OP-TEE)
      * *sp* (SPMC agnostic S-EL0 SP format)
  * - External Dependencies
    - * | Secure storage service instance (e.g. hosted by protected-storage SP)
      * | TRNG (platform specific)
      * | Carveout for CRB memory regions

env-test
--------
An instance of the test runner service provider is built into an SP image to
allow test cases to be run from within the SP isolated environment. The SP
image also includes environment and platform specific test cases to allow
access to FF-A services and platform hardware to be tested. The test runner
service provider is intended to be used in conjunction with a client that
coordinates which tests to run and collects test results.

.. list-table::
  :widths: 1 2
  :header-rows: 0

  * - Supported Environments
    - * *opteesp* (runs as an S-EL0 SP under OP-TEE)
      * *sp* (SPMC agnostic S-EL0 SP format)
  * - External Dependencies
    - * Any hardware accessed by test cases (platform specific)

--------------

*Copyright (c) 2022-2023, Arm Limited and Contributors. All rights reserved.*

SPDX-License-Identifier: BSD-3-Clause
