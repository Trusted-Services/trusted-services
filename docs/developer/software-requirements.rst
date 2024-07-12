Software Requirements
=====================

As of today the only available normal-world interface for Trusted Services is available trough linux. Building and end-to-end
firmware stack requires compiling the Linux Kernel and linux user space applications. This restricts the possible host
environments to Linux distributions.
While some TS components can be built under Windows this scenario is not mandated by this documentation.

The preferred host environment is Ubuntu 18.04.

The following tools are required:

   * CMake, version 3.18.4. (See the `CMake download page`_.)
   * GNU Make v4.1 or higher.
   * Git v2.17 or newer.
   * Python3.6 and the modules listed in ``<project>/requirements.txt``.
   * GCC supporting the deployment.

       * `opteesp` and `sp` environments: a host to aarch64 cross-compiler is needed. Please use the compilers specified by the
         `OP-TEE documentation`_.
       * `arm-linux` environment: a host to aarch64 linux cross-compiler is needed. Please use the version `9.2-2019.12` of the
         "aarch64-none-linux-gnu" compiler available from `arm Developer`_.
         (Note: the compiler part of the OP-TEE build environment works too.)
       * For `linux-pc` environment use the native version of GCC available in your Linux distribution.

   * The `AEM FVP`_ binaries if targeting the FVP platform.

To build the documentation, please refer to :ref:`Documentation Build Instructions`.

--------------

.. _OP-TEE documentation: https://optee.readthedocs.io/en/latest/building/gits/build.html#step-4-get-the-toolchains
.. _arm Developer: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-a/downloads
.. _CMake download page: https://cmake.org/files/v3.18/
.. _`AEM FVP`: https://developer.arm.com/-/media/Files/downloads/ecosystem-models/FVP_Base_RevC-2xAEMvA_11.22_14_Linux64.tgz

*Copyright (c) 2020-2023, Arm Limited and Contributors. All rights reserved.*

SPDX-License-Identifier: BSD-3-Clause
