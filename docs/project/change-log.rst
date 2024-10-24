Change Log & Release Notes
==========================

This document contains a summary of the new features, changes, fixes and known issues in each release of Trusted
Services.

Version <next>
--------------

- Trustedfirmware.org has deprecated Phabricator, the wiki and issue tracking provider. This functionality has been
  moved to GitHub. See the `Trusted Services organization`_. The project wiki has been moved
  `to here <https://github.com/Trusted-Services/trusted-services/wiki>`_


Feature Highlights
^^^^^^^^^^^^^^^^^^

- Add documentation covering the :ref:`OP-TEE SPMC tests`.
- Extend the :ref:`UEFI SMM Services` to support Authenticated Variables.
- Introduce a work in progress RPMB implementation and integrate it to the Block Storage service. The current
  configuration uses an SWd RAM buffer for data storage, and is not connected to the RPMB provider running in the NWd.
  This is the first baseline to implement a scenario where the RPMB owner is an S-EL0 SP.
- Introduce the :ref:`Logging Service`.
- Add crypto key store partitioning support to the Crypto Service. This feature allows isolating clients running in
  different SPs.
- Add the "RSE Com" protocol. RSE Com is a new protocol used between secure partitions and security subsystems like the
  Secure Enclave on Corstone-1000 or Runtime Security Engine on `RD-1 AE`_.


Deprecations
^^^^^^^^^^^^

- This OpenAMP based psa_ipc implementation is deprecated and will be removed in the future.


Updated external components
^^^^^^^^^^^^^^^^^^^^^^^^^^^

- MbedTLS version int the Crypto service is updated to v3.5.1.
- NanoPB has been upgraded to v0.4.8.

Breaking changes
^^^^^^^^^^^^^^^^


Resolved issues
^^^^^^^^^^^^^^^

- Fix psa-ps-api-test test case 414.
- Crypto key store partitioning has been implemented, and clients accessing the crypto service from different FF-A
  endpoints can not access each other's assets anymore.

Known limitations
^^^^^^^^^^^^^^^^^

- UEFI private variable authentication is not implemented in an UEFI compliant way. Currently such variables are
  authenticated by the certificates stored on the `DB` variable which is incorrect. Instead an implementation defined
  variable should be used to store the needed certificates. The plan is to follow ``EDK2`` use ``certdb`` and
  ``certdbv`` variables for this purpose.

Version 1.0.0
-------------

The first stabilised release of the project from previously prototype releases ready for product use.

Feature Highlights
^^^^^^^^^^^^^^^^^^

- Introduce the :doc:`Block Storage Service </services/block-storage-service-description>`. The Block Storage service
  can be used to share a block-oriented storage device such as a QSPI flash between a set of independent secure world
  clients.

- Introduce the :doc:`Firmware Update Service </services/fwu/index>`. The FWU service implements the Update Agent
  defined in the `Arm FWU-A specification`_ and allows replacing FW components with newer versions.

- Refactor FF-A UUID policy. Reinterpret the FF-A UUID to identify the protocol supported by TS SP instead of the
  service. This removes the maintenance burden of keeping an up to date UUID list in the service locator. All SPs start
  using the same protocol UUID, and implement a new discovery service (see the next point).

- Overhaul the RPC protocol. The main driver is to remove the single shared memory region limitation, which does not
  allow separating shared regions of clients running over Linux in the user-space. The second driver is to add
  versioning support to the RPC layer.

    - Allow multiple shared memory regions between endpoints.
    - Implement the discovery service in the RPC layer.
    - Allow assigning a UUID to interfaces. This mechanism replaces the protocol identifier used earlier. Each protocol
      of a service is represented as a dedicated interface.
    - Add versioning support to the RPC layer.

- Refactor the discovery service. The is removing the runtime overhead of memory sharing during the discovery and
  decreases code size and duplication using the same code for service discovery.

    - Implement the discovery service in the RPC layer for efficiency reasons.
    - Implement service identity discovery for all services.
    - Remove the encoding type entity and use service UUIDs to represent the serialization type.
    - Service property discovery is to be implemented in the future.

- Add support for the Corstone-1000 platform. For more information about this platform please see: `Corstone-1000 product homepage`_

- SPs now indicate support of :term:`Normal World` interrupt preemption capability in their SP manifest and allow the SPMC to enable
  preemption if possible. This removes NWd interrupts being disabled for long periods due to long service calls.

- Add support for the Armv8-A CRC32 feature for :term:`Secure World` and :term:`Normal World` components.

- Extend FF-A support with:

    - FF-A v1.1 boot protocol between the SPM and SPs.
    - FF-A v1.2 FFA_CONSOLE_LOG call. This allows SPs to emit log messages in an SPMC agonistic way.

- Improve the build system to allow setting the build steps of external components to be verbose.

- Add support for runtime (dynamic) psa-acs test case configuration.

Updated external components
^^^^^^^^^^^^^^^^^^^^^^^^^^^

- MbedTLS version integration into the Crypto service is updated to v3.4.0.
- The PSA Arch test is updated to version `74dc6646ff594e131a726a5305aba77bac30eceb`.

Breaking changes
^^^^^^^^^^^^^^^^

- The new RPC ABI is not backwards compatible and needs recent version of all depending components.

Resolved issues
^^^^^^^^^^^^^^^

- The new RPC version allows having multiple shared memory regions between endpoints. This allows each NWd client
  running in Linux user-space to use a dedicated buffer.

Known limitations
^^^^^^^^^^^^^^^^^

  - Crypto key store partitioning by client is not yet supported. This means multiple clients running at the same FF-A
    endpoint use a shared key space.
  - The full firmware update process implementation and testing is work-in-progress. The FWU process relies on the
    cooperation of multiple FW components owned by multiple FW projects. Some 3rd party components do not implement the
    needed features yet and thus, the FWU service was validated in "isolation" and exercised by TS test on the FVP
    platform and on the host PC only.
  - Service property discovery is not implemented yet.
  - Discovering the maximum payload size of a service is not supported yet and buffer sizes are hardcoded.

Version 1.0.0-Beta
------------------

The first tagged release of the project.

Feature Highlights
^^^^^^^^^^^^^^^^^^

The project supports the following services:

  - Secure Storage
  - Crypto
  - Initial Attestation
  - Smm Variable

Services may be accessed using client components that implement "`Psacertified v1.0 APIs`_". The project includes deployments
that integrate `PSA API certification tests`_ with API clients to facilitate end-to-end PSA certification testing.

Known limitations
'''''''''''''''''

  - Crypto key store partitioning by client is not yet supported.
  - Discovery support is only currently integrated into the Crypto service provider. In case of services not supporting
    this feature yet, communication parameters (e.g. maximum buffer size) and supported feature set needs to be hardcode
    to the service provider and service client.

Supported Trusted Environments
''''''''''''''''''''''''''''''

In the default configuration each service is deployed to a dedicated FF-A Secure Partition and executes isolated.
Service implementations are platform, trusted environment and service deployment agonistic. With appropriate enablement
work services can be enabled to work in any combination of these.

The reference integration uses the SPMC implemented in OP-TEE OS to manage TS SPs. This release supports `OP-TEE v3.19`_.

Supported Integration Systems
'''''''''''''''''''''''''''''

The reference solution uses the OP-TEE integration methodology. This relies on the google repo tool for high-level dependency
management and a set of makefiles to capture the build configuration information. For details please refer to
`OP-TEE git repo documentation`_.

The project is officially enabled in `Yocto meta-arm`_.

Supported Target Platforms
''''''''''''''''''''''''''

The only reference platform supported by this release is the `AEM FVP`_ build using the OP-TEE integration method.

Known limitations:

  - Non-volatile backend secure storage is not currently provided.

Test Report
^^^^^^^^^^^

Please find the Test Report covering this release in the `project wiki`_.


--------------

.. _`FF-A Specification v1.0`: https://developer.arm.com/documentation/den0077/a
.. _`Psacertified v1.0 APIs`: https://www.psacertified.org/development-resources/building-in-security/specifications-implementations/
.. _`OP-TEE v3.19`: https://github.com/OP-TEE/optee_os/tree/3.19.0
.. _`Yocto meta-arm` : https://gitlab.oss.arm.com/engineering/yocto/meta-arm/-/tree/master/meta-arm/recipes-security/trusted-services
.. _`project wiki`: https://github.com/Trusted-Services/trusted-services/wiki/Trusted-Services-test-reports
.. _`AEM FVP`: https://developer.arm.com/-/media/Files/downloads/ecosystem-models/FVP_Base_RevC-2xAEMvA_11.22_14_Linux64.tgz
.. _`PSA API certification tests`: https://github.com/ARM-software/psa-arch-tests
.. _`OP-TEE git repo documentation`: https://optee.readthedocs.io/en/latest/building/gits/build.html
.. _`Corstone-1000 product homepage`: https://developer.arm.com/Processors/Corstone-1000
.. _`Arm FWU-A specification`: https://developer.arm.com/documentation/den0118
.. _`Trusted Services organization`: https://github.com/Trusted-Services
.. _`RD-1 AE`: https://developer.arm.com/Tools%20and%20Software/Arm%20Reference%20Design-1%20AE

*Copyright (c) 2020-2024, Arm Limited and Contributors. All rights reserved.*

SPDX-License-Identifier: BSD-3-Clause
