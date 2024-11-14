Versioning policy
==================

This document captures information about the version identifier used by the
project. It tells the meaning of each part, where the version information is
captured and how it is managed.

Format of version IDs
---------------------

The version identifier identifies the feature set supported by a specific
release, and captures compatibility information to other releases.

This project uses "Semantic Versioning", for details please refer to |SEMVER|.

The version number is constructed from three numbers, and an optional
pre-release identifier. The `MAJOR` number is changed when incompatible API
changes are introduced, the `MINOR` version when new functionality is added in a
backward compatible manner, and the `PATCH` version when backwards compatible
bug fixes are added. The pre-release identifier is appended after the numbers
separated with a ``-`` and can be the string ``alpha`` or ``beta``.

Each release will get a unique version id assigned. When a release is made, the
version number will get incremented in accordance with the compatibility rules
mentioned above.

Version ID hierarchy
--------------------

The project hosts multiple components which can be used separately and thus need
compatibility information expressed independently. Such components get a
dedicated version ID. Examples are :ref:`libs-libsp` :ref:`libs-libts` and
:ref:`libs-libpsats`.

Components are never released standalone but only part of a TS release. In that
sense a set of independent component version IDs are assigned to a TS release ID.

Storage and format
------------------

The version number of each release will be stored at two locations:
  #. In a tag of the version control system in the form of "vX.Y.Z" where X Y
     and Z are the major, minor and patch version numbers.
  #. In a file called version.txt. This file uses ASCII encoding and will
     contain the version number as "X.Y.Z"  where X Y and Z are the major,
     minor and patch version numbers.

.. note:: The version id is independent from version identifiers of the
          versioning system used to store the |TS| (i.e. git).

--------------

.. _`Semantic Versioning`: https://semver.org/spec/v2.0.0.html

*Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.*

SPDX-License-Identifier: BSD-3-Clause
