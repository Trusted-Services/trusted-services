Glossary
========

This glossary provides definitions for terms and abbreviations used in the Trusted Services documentation.

You can find additional definitions in the `Arm Glossary`_.

.. glossary::
    :sorted:

    TF-A
        Trusted Firmware-A

    TS
        Trusted Services

    C identifier like string
        A name which uses only alphanumeric characters and underscores and the first character is not a digit.

    LCS
        `Linux Coding Style`_

    ACLE
        `Arm C language extensions`_

    PSA
        `Platforn Security Arhitecture`_

    FF-A
        `Firmware Framework for A`_

    Normal World
        The rich execution environment running the "business logic" of the device. NWd is separated by hardware
        technology (e.g. see :term:`TrustZone`) from components which are managing sensitive information. See
        :term:`Secure World`.

    NWd
        See :term:`Normal World`.

    Secure World
        Hardware isolated execution environment executing specially crafted software managing sensitive data. On Arm
        devices :term:`TrustZone` technology can be used to implement the needed isolation.

    SWd
        See :term:`Secure World`.

    TEE
        Trusted Execution Environment. An SPE implemented using TrustZone.

    TrustZone
        Arm :sup:`®` TrustZone :sup:`®` ia a hardware assisted isolation technology built into arm CPUs. See `TrustZone
        for Cortex-A`_.

    SPE
        See :term:`Secure Processing Environment`.

    Secure Processing Environment
        An isolated environment to execute software images backed by a specific set of hardware and arm architecture
        features. The aim of isolation is to protect sensitive workloads and their assets.

    SP
        see :term:`Secure Partition`

    Secure Partition
        Secure Partition is a compartment to execute a software image isolated from other images. Isolation can be logical
        or physical based on if physical address range isolation is involved or not. See :term:`Physical SP` and :term:`Logical SP`.

        An SP may host a single or multiple services.

    Physical SP
        A Secure Partition which executes a software image in and isolated physical address space.

    Logical SP
        A Secure Partition which executes a software image isolated without physical address space isolation.

    SPM
        See :term:`Secure Partition Manager`.

    Secure Partition Manager
        A component responsible for creating and managing the physical isolation boundary of an :term:`SP` in the SWd. It
        is built from two sub-components the :term:`Secure Partition Manager Dispatcher` and the
        :term:`Secure Partition Manager Core`.

    SPMD
        See :term:`Secure Partition Manager Dispatcher`.

    Secure Partition Manager Dispatcher
        The SPM component responsible for SPMC initialization boot-time, and forwarding FF-A calls run-time between SPs
        and between SPs and the SPMC.

    SPMC
        See :term:`Secure Partition Manager Core`.

    Secure Partition Manager Core
        A component responsible for SP initialization and isolation at boot-time, inter partition isolation at run-time,
        inter-partition communication at run-time.

    Secure Enclave
        An isolated hardware subsystem focusing on security related operations. The subsystem may include hardware
        peripherals and one or more processing elements. As an example see the `Arm SSE-700`_ subsystem.

--------------

.. _`Arm Glossary`: https://developer.arm.com/documentation/aeg0014
.. _`Linux Coding Style`: https://www.kernel.org/doc/html/v4.10/process/coding-style.html
.. _`Arm C language extensions`: https://developer.arm.com/documentation/101028/0012/?lang=en
.. _`Platforn Security Arhitecture`:  https://developer.arm.com/architectures/security-architectures/platform-security-architecture
.. _`Firmware Framework for A`: https://developer.arm.com/docs/den0077/latest
.. _`TrustZone for Cortex-A`: https://www.arm.com/technologies/trustzone-for-cortex-a
.. _`Arm SSE-700`: https://developer.arm.com/documentation/101418/0100/Overview/About-SSE-700

*Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.*

SPDX-License-Identifier: BSD-3-Clause
