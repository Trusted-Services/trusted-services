Build Instructions
==================
All trusted service builds use CMake to create native build files for building and installing service binaries
and other build products.  Details about the tools needed for building are specified here:
:ref:`Software Requirements`.

All top-level build files are located beneath the 'deployments' parent directory under a sub-directory
for each deployment.  For more information about the project directory structure, see:
:ref:`Project Structure`.

Build Flow
----------
All deployment builds follow a common flow that results in the creation of executable binaries or libraries
and the installation of files into an output directory.  Deploying the contents of the output directory into
the target environment is handled in an environment specific way and is not part of the common build
flow.  The build flow conforms to the conventional CMake process where building takes place in to following
two stages:

  1. Native build files, such as makefiles, are generated from CMake configuration files.
  2. Native build tools, such as make, are used to build and install items, ready for deployment.

The following activity diagram illustrates the common deployment build flow.  The green activity states
lie outside of the common build flow.  Environment specific instructions are provided for deploying into
different environments:

.. uml:: uml/BuildFlow.puml

.. _`Selecting the build type`:

Selecting the build type
-------------------------
The build type selects code optimization and debug information related compiler settings. The build system follows the
standard CMake methodology and can be controlled with the `CMAKE_BUILD_TYPE`_ variable.

The build system uses the following build types:

.. list-table:: Supported build types
   :header-rows: 1

   * - Build type
     - Purpose
     - Shared library suffix
     - Description
   * - Debug
     - For debugging purposes.
     - `d`
     - Optimization is off, debugging information generation is enabled.
   * - DebugCoverage
     - For coverage measurement purposes.
     - `c`
     - Optimization is off, debugging information generation is enabled, code is compiled to enable gcov based coverage
       measurement.
   * - MinSizeRel
     - Size optimized release build.
     - None.
     - Optimization is configured to prefer small code size, debugging information is not generated.
   * - MinSizWithDebInfo
     - For debugging size optimized release build.
     - None.
     - Optimization is configured to prefer small code size, debugging information generation is enabled.
   * - Release
     - Speed optimized release build.
     - None.
     - Optimization is configured to prefer execution speed, debugging information is not generated.
   * - RelWithDebugInfo
     - For debugging speed optimized release build.
     - None.
     - Optimization is configured to prefer execution speed, debugging information generation is enabled.

Build type of external components can be configured with command line arguments. Argument names follow this pattern:
``-D<upper case component name>_BUILD_TYPE=<value>`` e.g. ``-DNANOPB_BUILD_TYPE=Release``. Supported values are
component specific, please refer to the appropriate cmake file under ``<TS_ROOT>/external/<name>``.

Binaries of the ``Debug`` and ``DebugCoverage`` build types will have a suffix appended to their base name. This allows
multiple binaries to coexist on the same system at the same time. However the system integrator can only select
a single "release" build type.


Building and Installing
-----------------------
When building from a clean environment where no generated build files exist, it is necessary to run
the CMake command, specifying the source directory, the build directory and optionally, the install
directory where build output is installed.

To illustrate the steps involved, we will build the 'component-test' executable to run in the
'linux-pc' environment.  The built executable is a standalone program that uses the CppUTest
framework to run a set of component level tests on components from within the project.  For this
example, it is assumed that we are building under Linux and 'make' is used as the native build tool.

The described steps may be used for any of the deployments under the top-level *deployments* directory.

Starting from the project root directory, change directory to the relevant deployment directory::

  cd deployments/component-test/linux-pc

Build file generation is performed using the CMake command.  If no CMAKE_INSTALL_PREFIX path is
specified, build output will be installed in the default location (*build/install*).  To generate
build files that install to the default location, use::

  cmake -S . -B build

To generate build files that install to an alternative location, use::

  cmake -S . -B build -DCMAKE_INSTALL_PREFIX=<install_dir>

Having successfully generated build files, the native build tool may be run to build and install
files using::

  cd build
  make install

In the above example, all build output is written to a sub-directory called 'build'.  You
are free to choose any location for build output.

Dependencies on external components and in-tree built objects, such as libraries,
are handled automatically by the build system during the *generating* phase.  External components
are fetched from the relevant source repository and built as part of the build context for the
deployment binary being built.  This allows deployment specific configuration and compiler options
to be applied to the external component without impacting other builds.  Dependencies on in-tree
built libraries are handled in a similar manner.

For information on running tests, see:
:ref:`Running Tests`.

For more information on deployments, see:
:ref:`Deployments`.

Installed build output files
----------------------------
On successfully completing the *building* phase of the build flow, a set of build output files are
installed to the directory specified by CMAKE_INSTALL_PREFIX.  The set of installed files will
depend on the type of build and the environment in which the files will be deployed.  The following
table summarizes what files are installed for different typed of build during the *installing* phase
of the build flow:

.. list-table:: Example build output files
  :header-rows: 1

  * - Deployment type
    - Environment
    - Files installed
  * - Binary executable
    - linux-pc, arm-linux
    - | *bin/* - program binary
  * - Shared library
    - linux-pc, arm-linux
    - | *include/* - public header files
      | *lib/* - shared library
      | *lib/cmake/* - cmake target import file
  * - SP image
    - opteesp
    - | *bin/* - stripped elf file for SP
      | *lib/make* - OP-TEE helper makefile
  * - SP collection
    - opteesp
    - | *bin/* - set of stripped elf files
      | *lib/make/* - set of OP-TEE helper makefiles


Deploying installed files
-------------------------
Having built and installed build output files to a known directory, further steps may be needed to
deploy the files into the target processing environment.  The nature of these steps will be different
for different environments.

To avoid overly complicating the common Trusted Services build system, details of how installed files
are deployed into the target execution environment are handled separately and may rely on environment
specific tools.

Some example deployment methods are:

  * A filesystem share exists between a build machine and the target machine. Files installed into the shared directory are
    directly accessible by the target.
  * Installed files are incorporated into a third-party build process e.g. OP-TEE.

The following guides provide instructions on deploying services and running programs on FVP:

* :ref:`Deploying trusted services in S-EL0 Secure Partitions under OP-TEE`
* :ref:`Running User-space Programs on FVP`

Batch Building
--------------
To support batching building of a set of deployments, a tool called b-test is included.  For
more information, see
:doc:`b-test page <./b-test>`

--------------

.. _CMAKE_BUILD_TYPE: https://cmake.org/cmake/help/v3.18/variable/CMAKE_BUILD_TYPE.html

*Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.*

SPDX-License-Identifier: BSD-3-Clause
