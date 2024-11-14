Build and run tests on OP-TEE reference integration for FVP
===========================================================

The Linux based build maintained by the OP-TEE project is used as the reference integration for testing
trusted service deployments on a simulated hardware platform. Service providers deployed within secure partitions
are tested using test executables that run as user-space programs under Linux. Test cases interact with trusted
service providers using standard service access protocols, carried by FF-A based messages.

The test executables most often used for service level testing on hardware platforms are:

  - *ts-service-test* - contains a set of service-level end-to-end tests. Discovers and communicates
    with service providers using libts.
  - *psa-api-test* - PSA functional API conformance tests (from external project). Also uses libts.

This method uses the makefiles from the ``op-tee/build`` repository.


Before you start
----------------

Before attempting to run tests on the FVP simulation, the OP-TEE reference integration needs to be
built and run. Read the following guides to understand how to do this:

  - OP-TEE build and run instructions, see:
    :ref:`Deploying trusted services in S-EL0 Secure Partitions under OP-TEE`

  - Instructions for loading and running user-space programs on FVP:
    :ref:`Running user-space programs on FVP`


Build the Linux application binaries
------------------------------------

From the root directory of the workspace, enter the following to build the test applications::

  make -C build ffa-test-all


Run *ts-service-test*
---------------------

From the root directory of the workspace, enter::

  FVP_PATH=../Base_RevC_AEMvA_pkg/models/Linux64_GCC-9.3 make -C build run-only

Once it boots to the login prompt, log in as root and from the FVP terminal, enter::

  cd /mnt/host
  cp -vat /usr out/ts-install/arm-linux/lib out/ts-install/arm-linux/bin
  out/linux-arm-ffa-user/load_module.sh
  ts-service-test -v

Use the same flow for other user-space programs. Check the output of the ``cp`` command executed to see
executables copied under ``/usr/bin``.

.. _build-spmc-tests:

Build SPMC tests
----------------

Trusted Services includes S-EL0 tests SPs for validating the SPMC's behavior by invoking various FF-A interfaces. These tests
require OP-TEE's `xtest`_ for starting them from the normal world. ``xtest`` uses ``linux-arm-ffa-user`` for accessing the
FF-A layer. Building the SPMC tests uses the same build system but with the ``SPMC_TEST=y`` configuration::

  make -C build SPMC_TEST=y all

.. note::
  This build configuration only contains the SPMC test SPs and it does not include any of the :ref:`Services`.

See :ref:`OP-TEE SPMC tests` for further information.

Run SPMC tests
--------------

From the root directory of the workspace, enter::

  FVP_PATH=../Base_RevC_AEMvA_pkg/models/Linux64_GCC-9.3 make -C build run-only

Once it boots to the login prompt, log in as root and from the FVP terminal, enter::

  cd /mnt/host
  out/linux-arm-ffa-user/load_module.sh
  xtest -t ffa_spmc

--------------

.. _`xtest`: https://optee.readthedocs.io/en/latest/building/gits/optee_test.html

*Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.*

SPDX-License-Identifier: BSD-3-Clause
