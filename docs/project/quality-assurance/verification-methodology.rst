Verification methodology
========================

This page discusses discusses verification tools and techniques used by the project.


Static Checks
-------------

This verification step checks quality by examining the source code. The project currently uses two tools which are
discussed in the chapters below.

Checkpatch
''''''''''

`Checkpatch`_ is a tool developed and maintained by the Linux Kernel community. It can look for errors related to:

  - C and C++ coding style
  - spelling mistakes
  - git commit message formatting

Please find the configuration of this tool in the :download:`TS repository.<../../../.checkpatch>` but do not forget
to set ``CODESPELL_FILE`` variable in the environment to the local copy of the `codespell dictionary v2.3.0`_.

Cppcheck tool
'''''''''''''

`CppCheck`_ is a C/C++ static analysis tool. It can detect code depending on implementation defined behavior, and
dangerous coding constructs and thus it verifies coding guidelines.

Please find the configuration of this tool in the :download:`TS repository.<../../../.cppcheck>`

Build verification
------------------

The :ref:`Build test runner` captures reference build configurations for all TS build products and can be used to verify
these.

Runtime verification
--------------------

During the runtime versification step various test and demo executables are executed on the host PC and/or on target
platforms.

Tests are targeting three different environment types:

  - ``arm-linux``: test executables to be run from Linux user-space on the target.
  - ``pc-linux``: executables to run on the host PC. These tests have a lower verification level, as the binary is likely
    not running on an arm target. Portability issues in the source may hide error or trigger false alarms. In turn
    this type of test is cheap,
  - ``sp`` and ``opteesp``: test executables targeting these environments run in the SWd and server as:

    - test payloads to help exercise trusted services
    - test payload to help platform porting

Each of these test applications manifest as a "deployment" in trusted services. For more details please see the
:ref:`Deployments` section.

Compliance testing
''''''''''''''''''

The project hosts deployment helping compliance testing. For more information please refer to
:ref:`Platform Certification`.

Measuring Quality of testing
''''''''''''''''''''''''''''

The primary metric to measure runtime testing quality is code coverage. The project uses two methods to measure code
coverage:

    - "Host side" coverage measurement, where code coverage is measured using Unit Tests and Component Test executed on
      the build host.

      The measurement relies on GCCs `-ftest-coverage` compilation switch and the lcov tool. In order to include
      coverage data of PSA client code and of service providers, `libpsats` and `libts` have to be built using
      the `DebugCoverage` build type. If the build type override is not used, the build system will automatically build
      and use the appropriate binaries.

    - Coverage measurement on the target. This feature is only supported by the :ref:`AEM FVP` platform and uses
      `qa-tools`_.

Coverage measurement is automated in the CI, which is not publicly accessible yet.

------------------

.. _`Checkpatch`: https://docs.kernel.org/dev-tools/checkpatch.html
.. _`CppCheck`: https://cppcheck.sourceforge.io/
.. _`codespell dictionary v2.3.0`: https://raw.githubusercontent.com/codespell-project/codespell/v2.3.0/codespell_lib/data/dictionary.txt
.. _`qa-tools`: https://gitlab.arm.com/tooling/qa-tools

*Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.*

SPDX-License-Identifier: BSD-3-Clause
