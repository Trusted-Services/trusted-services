Supported Architectural Features
================================

.. _branch_protection:

Branch Protection
-----------------

Branch protection covers two architecture features for mitigating Return Oriented Programming (ROP) and Jump Oriented
Programming (JOP) attacks. For generic overview of these features please see
`Providing protection for complex software`_.

Branch Protection related hardware features can not be controlled from S-EL0 and to enable these co-operation with
components running at higher exception levels is needed. Please refer to the documentation of the these components for
information on how to enable these HW features.

Pointer Authentication (FEAT_PAUTH)
'''''''''''''''''''''''''''''''''''

Pointer Authentication (PAC) is an ARMv8.3 feature where the return address of each function is signed. The signature is
calculated when the function is called and verified upon returning. This is to ensure the return address has not been
tampered with.

Branch Target Identification (FEAT_BTI)
'''''''''''''''''''''''''''''''''''''''

Branch Target Identification (BTI) is an ARMv8.5 feature which is used to guard against the execution of instructions
that are not the intended target of an indirect branch.

For raw-binary SPs the TS build system will will mark the executable regions with the BTI attribute in the SP manifest
file. The SPMC implementation running the SP is required to support this memory attribute, or has to unconditionally
enable BTI for all executable regions to get the feature working.

Although GCC has BTI support since v9, no BTI enabled libraries are shipped with the current GNUARM releases. A possible
workaround is to compile from source as described in the `FAQ section of the OP-TEE documentation`_.

Configuring Branch Protection in the TS build system
''''''''''''''''''''''''''''''''''''''''''''''''''''

The feature can be controlled using the ``BRANCH_PROTECTION`` variable. This can be set e.g. on the cmake command line
by passing ``-DBRANCH_PROTECTION=<value>``. The default value is ``unset``. Please refer to `GCC manual`_ for the
detailed description of the values.

.. list-table:: Supported values of ``BRANCH_PROTECTION``
   :name: branch_protection_table
   :header-rows: 1

   * - Value
     - GCC option
     - FEAT_PAUTH
     - FEAT_BTI
   * - unset
     -
     - usually `N` (depends on how GCC is compiled)
     - usually `N` (depends on how GCC is compiled)
   * - 0
     - "none"
     - N
     - N
   * - 1
     - "standard"
     - Y
     - Y
   * - 2
     - "pac-ret"
     - Y
     - N
   * - 3
     - "pac-ret+leaf"
     - Y
     - N
   * - 4
     - "bti"
     - N
     - Y

--------------

.. _`FAQ section of the OP-TEE documentation`: https://optee.readthedocs.io/en/stable/faq/faq.html#faq-gcc-bti
.. _`Providing protection for complex software` : https://developer.arm.com/documentation/102433/0200
.. _`GCC Manual`: https://gcc.gnu.org/onlinedocs/gcc-9.1.0/gcc/AArch64-Options.html

*Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.*

SPDX-License-Identifier: BSD-3-Clause
