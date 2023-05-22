Build and run PC based tests
============================

Many components within the Trusted Services project may be built and tested within a native PC environment.
PC based testing is an important part of the development flow and can provide a straight-forward way to check
for regressions and debug problems. PC based tests range from small unit tests up to end-to-end service tests.
All test cases in the Trusted Services project are written for the CppUTest test framework.

The test executables most often used for PC based testing of Trusted Services components are:

  - *component-test* - a PC executable that runs many component level tests.
  - *ts-service-test* - contains a set of service-level end-to-end tests. For PC build, service providers
    are included in the libts library.
  - *psa-api-test* - PSA functional API conformance tests (from external project).


Before you start
----------------
Before attempting to run any builds, ensure that all necessary tools are installed.  See: :ref:`Software Requirements`


Build and run *component-test*
------------------------------

From the root directory of the checked-out TS project, enter the following::

  cmake -B build-ct -S deployments/component-test/linux-pc
  make -C build-ct install
  build-ct/install/linux-pc/bin/component-test -v


Build and run *ts-service-test*
-------------------------------

From the root directory of the checked-out TS project, enter the following::

  cmake -B build-ts -S deployments/ts-service-test/linux-pc
  make -C build-ts install
  LD_LIBRARY_PATH=build-ts/install/linux-pc/lib build-ts/install/linux-pc/bin/ts-service-test -v

Build and run *psa-api-test*
----------------------------
Tests for each API are built as separate executables. Test are available for the following APIs::

  crypto
  initial_attestation
  internal_trusted_storage
  protected_storage

To build and run tests for the Crypto API, enter the following (use the same flow for other APIs)::

  cmake -B build-pa deployments/psa-api-test/crypto/linux-pc
  make -C build-pa install
  LD_LIBRARY_PATH=build-ts/install/linux-pc/lib build-pa/install/linux-pc/bin/psa-crypto-api-test

psa-api test binaries accept the listed command-line arguments:

===========  ====================================================================
CL argument  Description
===========  ====================================================================
-v           verbose mode
-h           print supported arguments
-l           print a list of all the available tests
-t           run only the listed tests with 'test\_' prefix and semicolon postfix
             e.g: "-t test_201;test_205;test_260;"
===========  ====================================================================

More information
----------------
For more information about deployments and building, see: :ref:`Build Instructions`

PSA functional API conformance tests git location: https://github.com/ARM-software/psa-arch-tests.git

--------------

*Copyright (c) 2022-2023, Arm Limited and Contributors. All rights reserved.*

SPDX-License-Identifier: BSD-3-Clause
