Target Platforms
================

Target platforms are emulated or physically implemented hardware. This chapter discusses platform
related information.

Platforms can be categorized by level of support as:

   - Reference platforms
     Reference platforms are "easily" accessible for testing and quality gate-keeping of the main
     branch includes tests executed on these platforms.

   - Active
     Platforms in this category are updated and tested by their owners for each release.

   - Obsolete
     Platforms which are not tested for the current release are put into this category.

   - Deprecated Platforms not tested for more than one two releases are threated as obsolete, and
     will be removed for the next release.

The quality of the platform, known issues, feature limitations, extra features, etc... are defined
in the sub-chapters below.

Reference platforms
-------------------

.. toctree::
    :maxdepth: 1

    ./aem-fvp

Active Platforms
----------------

These platforms are not maintained by the Trusted Services project and are contributed externally. Instructions about
building and running the software stacks of these platforms is out of scope for TS documentation.

.. toctree::
    :maxdepth: 1

    ./Corstone1000

--------------

*Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.*

SPDX-License-Identifier: BSD-3-Clause
