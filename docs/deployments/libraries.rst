Libraries
=========
Some deployments build common functionality into libraries that may be used by
other deployments or external applications. The following library deployments
are currently supported:

The libraries will have a build type-specific suffix added to the base name of
their binaries, allowing multiple binaries to be installed in the same tree.
(See :ref:`Selecting the build type`) This differentiation applies to the
``Debug`` and ``DebugCoverage`` build types. The system integrator must choose
which single ``Release<XXX>`` build type is to be supported in the system.
Package managers and mutually exclusive release packages can be used to add
further flexibility.

.. _libs-libts:

libts
-----

Userspace applications that depend on trusted services may use *libts* for handling
service discovery and RPC messaging. A major benefit to application developers is
that *libts* entirely decouples client applications from details of where a service
provider is deployed and how to communicate with it. All TS test executables and
tools that interact with service providers use *libts*.

To facilitate test and development within a native PC environment, the *libts*
deployment for the *linux-pc* environment integrates a set of service providers
into the library itself. From a client application's perspective, this looks
exactly the same as when running on a target platform with service providers
deployed in secure processing environments. For more information, see:
:ref:`Service Locator`.

.. list-table::
  :widths: 1 2
  :header-rows: 0

  * - Supported Environments
    - * | *linux-pc* - service providers integrated into library
      * | *arm-linux* - communicates with service providers in secure processing environment
  * - Used by
    - * Userspace applications.


.. _libs-libpsats:

libpsats
--------

Trusted Services implements the `PSA Certified APIs`_. Libpsats encapsulates the service client implementations
which implement this API. Linux uuser-space applications can use libpsats to easily access the PSA services
implemented by the project. Libpsats depends on libts for RPC and service discovery services.

.. list-table::
  :widths: 1 2
  :header-rows: 0

  * - Supported Environments
    - * | *linux-pc*
      * | *arm-linux*
  * - Used by
    - * Userspace applications
  * - Depends on
    - * `libts`_ library

Build and integration examples
..............................

Build as shared library::

  cmake -S ./trusted-services/deployments/libpsats/linux-pc/ -B ./build
  make -C build && make -C build install

.. warning::
    Building as static library is not yet supported.

To integrate the library libts shall also be integrated. To achieve this add the listed
lines to the application's cmake files::

  find_package(libpsats "1.0.0" REQUIRED PATHS "<install path>")
  find_package(libts "2.0.0" REQUIRED PATHS "<install path>")
  target_link_libraries(ts-demo PRIVATE libpsats::psats)

Initialization
..............

Before calling any function from libpsats the proper part of the library has to be initialized.
Before exiting the application (or when PSA services are no longer needed) the initialized
parts must be deinitialized. To access the library **libpsats.h** must be included::

   psa_status_t libpsats_init_crypto_context(const char *service_name);
   void libpsats_deinit_crypto_context(void);

   psa_status_t libpsats_init_attestation_context(const char *service_name);
   void libpsats_deinit_attestation_context(void);

   psa_status_t libpsats_init_its_context(const char *service_name);
   void libpsats_deinit_its_context(void);

   psa_status_t libpsats_init_ps_context(const char *service_name);
   void libpsats_deinit_ps_context(void);

The example below initializes and then deinitializes crypto::

    psa_status_t psa_status = libpsats_init_crypto_context("sn:trustedfirmware.org:crypto:0");
    if (psa_status) {
        printf("libpsats_init_crypto_context failed: %d\n", psa_status);
        return PSA_ERROR_GENERIC_ERROR;
    }

    libpsats_deinit_crypto_context();

Known issues and Limitations
............................

| The library is single client.
| The library is not thread safe.
| Only linux user-space is supported currently.

.. _libs-libsp:

libsp
-----

*libsp* provides a functional interface for using FF-A messaging and memory
management facilities. *libsp* is used in SP deployments. For more information, see:
:ref:`libsp`.

.. list-table::
  :widths: 1 2
  :header-rows: 0

  * - Supported Environments
    - * | *opteesp*
  * - Used by
    - * Secure partitions

Known issues and Limitations
............................

| The library is single client.
| The library is not thread safe.
| Only linux user-space is supported currently.

--------------

.. _`PSA Certified APIs`: https://arm-software.github.io/psa-api/

*Copyright (c) 2022-2023, Arm Limited and Contributors. All rights reserved.*

SPDX-License-Identifier: BSD-3-Clause
