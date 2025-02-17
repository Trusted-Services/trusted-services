RD-1 AE
=======

The **Arm Reference Design-1 AE**, or **RD-1 AE** introduces the concept of a high-performance
Arm\ :sup:`®` Neoverse\ :sup:`TM` V3AE Application Processor (Primary Compute) system
augmented with an Arm\ :sup:`®` Cortex\ :sup:`®`-R82AE based Safety Island for scenarios where
additional system safety monitoring is required. The system additionally includes a Runtime Security
Engine (RSE) used for the secure boot of the system elements and the runtime Secure Services.

The Reference Software Stack implements the following Secure Services on top of
the Trusted Services framework:

* Crypto Service
* Secure Storage Service
* UEFI SMM Services

On the Primary Compute, the implementation of Crypto Service and Secure Storage Service is based
on the SE Proxy secure partition.

The Primary Compute also provides the implementation of UEFI SMM Services via the
SMM Gateway Secure Partition (SMM Gateway SP) to support UEFI Variable Service.

For more information, including how to obtain the Technical Overview document, visit the
`Arm Reference Design-1 AE page on developer.arm.com`_.

--------------

.. _`Arm Reference Design-1 AE page on developer.arm.com`: https://developer.arm.com/Tools%20and%20Software/Arm%20Reference%20Design-1%20AE

*Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.*

SPDX-License-Identifier: BSD-3-Clause
