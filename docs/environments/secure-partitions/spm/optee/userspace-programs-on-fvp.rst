Running user-space programs on FVP
==================================

This page explains how to load and run user space programs on a Linux image running in FVP simulation.
The loaded programs may use any trusted services that are available as part of the image firmware.

To prepare and run an image that includes trusted services running in S-EL0 secure partitions under
OP-TEE see: :ref:`Deploying trusted services in S-EL0 Secure Partitions under OP-TEE`

The example assumes that the FVP model has been installed in the following
directory relative to the OP-TEE build directory::

    ../Base_RevC_AEMvA_pkg/models/Linux64_GCC-9.3


Shared directory
----------------

The AEM FVP supports directory sharing between the target and the host OS. This provides a
convenient way to transfer files between the host and the device simulation.  When the FVP is run
using the *run-only* target from the *op-tee/build* repository, the shared directory is set-up
automatically. The whole "workspace" is shared to the FVP and mounted under ``/mnt/host``.


Running service level tests
---------------------------

Most test and demo applications are integrated into the OP-TEE build flow, and can be build using
the makefiles in the ``op-tee/build`` repository.

To build all such binaries build the ``ffa-test-all`` target. For available targets please refer to
`fvp-psa-sp.mk`. As an example to build the ``ts-service-test`` application execute the following
commands from the root of the workspace::

    make -C build ffa-ts-service-test

The executable includes service level test cases that exercise trusted services via their
standard interfaces.  Test cases use *libts* for locating services and establishing RPC
sessions.  *ts-service-test* provides a useful reference for understanding how *libts* may
be used for accessing trusted services.

Build output will be copied to ``out/ts-install``.

To build the applications without using the ``op-tee/build`` files refer to the instructions here:
:ref:`Build Instructions`

Run *ts-service-test*
'''''''''''''''''''''

To start the FVP, from the root directory of the workspace, enter::

  FVP_PATH=../Base_RevC_AEMvA_pkg/models/Linux64_GCC-9.3 make -C build run-only

Once it boots to the login prompt, log in as root and from the FVP terminal, enter::

  # Enter the mount target for the shared directory
  cd /mnt/host

  # Install the shared library and executables
  cp -vat /usr out/ts-install/arm-linux/lib out/ts-install/arm-linux/bin

  # Load the kernel module
  out/linux-arm-ffa-user/load_module.sh

  # Run the test application
  ts-service-test -v

Use the same flow for other user-space programs. Check the output of the ``cp`` command executed to see
executables copied under ``/usr/bin``.

If all is well, you should see something like::

    TEST(PsServiceTests, createAndSetExtended) - 0 ms
    TEST(PsServiceTests, createAndSet) - 0 ms
    TEST(PsServiceTests, storeNewItem) - 0 ms
    TEST(ItsServiceTests, storeNewItem) - 0 ms
    TEST(AttestationProvisioningTests, provisionedIak) - 1 ms
    TEST(AttestationProvisioningTests, selfGeneratedIak) - 1 ms
    TEST(AttestationServiceTests, repeatedOperation) - 75 ms
    TEST(AttestationServiceTests, invalidChallengeLen) - 0 ms
    TEST(AttestationServiceTests, checkTokenSize) - 2 ms
    TEST(CryptoKeyDerivationServicePackedcTests, deriveAbort) - 0 ms
    TEST(CryptoKeyDerivationServicePackedcTests, hkdfDeriveBytes) - 0 ms
    TEST(CryptoKeyDerivationServicePackedcTests, hkdfDeriveKey) - 0 ms
    TEST(CryptoMacServicePackedcTests, macAbort) - 0 ms
    TEST(CryptoMacServicePackedcTests, signAndVerify) - 1 ms
    TEST(CryptoCipherServicePackedcTests, cipherAbort) - 0 ms
    TEST(CryptoCipherServicePackedcTests, encryptDecryptRoundtrip) - 0 ms
    TEST(CryptoHashServicePackedcTests, hashAbort) - 0 ms
    TEST(CryptoHashServicePackedcTests, hashAndVerify) - 0 ms
    TEST(CryptoHashServicePackedcTests, calculateHash) - 0 ms
    TEST(CryptoServicePackedcTests, generateRandomNumbers) - 0 ms
    TEST(CryptoServicePackedcTests, asymEncryptDecryptWithSalt) - 14 ms
    TEST(CryptoServicePackedcTests, asymEncryptDecrypt) - 1 ms
    TEST(CryptoServicePackedcTests, signAndVerifyEat) - 4 ms
    TEST(CryptoServicePackedcTests, signAndVerifyMessage) - 4 ms
    TEST(CryptoServicePackedcTests, signAndVerifyHash) - 4 ms
    TEST(CryptoServicePackedcTests, exportAndImportKeyPair) - 1 ms
    TEST(CryptoServicePackedcTests, exportPublicKey) - 1 ms
    TEST(CryptoServicePackedcTests, purgeKey) - 0 ms
    TEST(CryptoServicePackedcTests, copyKey) - 1 ms
    TEST(CryptoServicePackedcTests, generatePersistentKeys) - 1 ms
    TEST(CryptoServicePackedcTests, generateVolatileKeys) - 0 ms
    TEST(CryptoServiceProtobufTests, generateRandomNumbers) - 1 ms
    TEST(CryptoServiceProtobufTests, asymEncryptDecryptWithSalt) - 15 ms
    TEST(CryptoServiceProtobufTests, asymEncryptDecrypt) - 1 ms
    TEST(CryptoServiceProtobufTests, signAndVerifyMessage) - 4 ms
    TEST(CryptoServiceProtobufTests, signAndVerifyHash) - 4 ms
    TEST(CryptoServiceProtobufTests, exportAndImportKeyPair) - 1 ms
    TEST(CryptoServiceProtobufTests, exportPublicKey) - 0 ms
    TEST(CryptoServiceProtobufTests, generatePersistentKeys) - 1 ms
    TEST(CryptoServiceProtobufTests, generateVolatileKeys) - 1 ms
    TEST(CryptoServiceLimitTests, volatileRsaKeyPairLimit) - 99 ms
    TEST(CryptoServiceLimitTests, volatileEccKeyPairLimit) - 22 ms
    TEST(DiscoveryServiceTests, checkServiceInfo) - 0 ms
    TEST(SmmVariableAttackTests, getCheckPropertyWithMaxSizeName) - 0 ms
    TEST(SmmVariableAttackTests, getCheckPropertyWithOversizeName) - 0 ms
    TEST(SmmVariableAttackTests, setCheckPropertyWithMaxSizeName) - 0 ms
    TEST(SmmVariableAttackTests, setCheckPropertyWithOversizeName) - 0 ms
    TEST(SmmVariableAttackTests, enumerateWithSizeMaxNameSize) - 0 ms
    TEST(SmmVariableAttackTests, enumerateWithOversizeName) - 0 ms
    TEST(SmmVariableAttackTests, setAndGetWithSizeMaxNameSize) - 0 ms
    TEST(SmmVariableAttackTests, setAndGetWithOversizeName) - 0 ms
    TEST(SmmVariableAttackTests, setWithSizeMaxNameSize) - 0 ms
    TEST(SmmVariableAttackTests, setWithOversizeName) - 0 ms
    TEST(SmmVariableAttackTests, setWithSizeMaxDataSize) - 0 ms
    TEST(SmmVariableAttackTests, setWithOversizeData) - 0 ms
    TEST(SmmVariableServiceTests, checkMaxVariablePayload) - 0 ms
    TEST(SmmVariableServiceTests, setSizeConstraint) - 0 ms
    TEST(SmmVariableServiceTests, enumerateStoreContents) - 0 ms
    TEST(SmmVariableServiceTests, getVarSizeNv) - 0 ms
    TEST(SmmVariableServiceTests, getVarSize) - 0 ms
    TEST(SmmVariableServiceTests, setAndGetNv) - 1 ms
    TEST(SmmVariableServiceTests, setAndGet) - 0 ms
    TEST(TestRunnerServiceTests, runSpecificTest) - 0 ms
    TEST(TestRunnerServiceTests, runConfigTests) - 0 ms
    TEST(TestRunnerServiceTests, listPlatformTests) - 0 ms
    TEST(TestRunnerServiceTests, runAllTests) - 0 ms
    TEST(TestRunnerServiceTests, listAllTests) - 0 ms

    OK (67 tests, 67 ran, 977 checks, 0 ignored, 0 filtered out, 261 ms)

--------------

.. _fvp-psa-sp.mk: https://github.com/OP-TEE/build/blob/master/fvp-psa-sp.mk

*Copyright (c) 2020-2023, Arm Limited and Contributors. All rights reserved.*

SPDX-License-Identifier: BSD-3-Clause
