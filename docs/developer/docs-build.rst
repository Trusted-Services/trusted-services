Documentation Build Instructions
================================

To create a rendered copy of this documentation locally you can use the
`Sphinx`_ tool to build and package the plain-text documents into HTML-formatted
pages.

If you are building the documentation for the first time then you will need to
check that you have the required software packages, as described in the
*Prerequisites* section that follows.

Prerequisites
-------------

For building a local copy of the |TS| documentation you will need, at minimum:

- GNUMake
- Python 3 (3.5 or later)
- PlantUML (1.2024.7 or later)

You must also install the Python modules that are specified in the
``requirements.txt`` file in the root of the ``docs`` directory. These modules
can be installed using ``pip3`` (the Python Package Installer). Passing this
requirements file as an argument to ``pip3`` automatically installs the specific
module versions required.

Example environment
-------------------

An example set of installation commands for Linux with the following assumptions:
    #. OS and version: Ubuntu 22.04 LTS
    #. `virtualenv` is used to separate the python dependencies
    #. pip is used for python dependency management
    #. `bash` is used as the shell.

.. code:: shell

    sudo apt install make python3 python3-pip virtualenv python3-virtualenv plantuml
    wget https://github.com/plantuml/plantuml/releases/download/v1.2024.7/plantuml-1.2024.7.jar -O $HOME/plantuml.jar
    virtualenv -p python3 ~/sphinx-venv
    . ~/sphinx-venv/bin/activate
    pip3 install -r requirements.txt
    deactivate

.. note::
   More advanced usage instructions for *pip* are beyond the scope of this
   document but you can refer to the `pip homepage`_ for detailed guides.

.. note::
   For more information on Virtualenv please refer to the `Virtualenv documentation`_

Building rendered documentation
-------------------------------

From the ``docs`` directory of the project, run the following commands.

.. code:: shell

   . ~/sphinx-venv/bin/activate
   export PLANTUML_JAR_PATH=$HOME/plantuml.jar
   make clean
   make
   deactivate

Output from the build process will be placed in:

::

   <TS root>/docs/_build

Configuring the documentation build
-----------------------------------

The plantuml binary used during the documentation build can be controlled using
the following environment variables:

    - ``PLANTUML``: specifies the location of a wrapper script. This must be
      executable and shall run plantuml.jar with all arguments passed over to
      the tool. If set, will override other settings.

    - ``PLANTUML_JAR_PATH``: full path to the plantuml.jar file to use. If set,
      an internal wrapper will be used to call plantuml.
    - ``JAVA_HOME``: used only is ``PLANTUML_JAR_PATH`` is set to specify the
      JVM executable location to be used by the internal wrapper. The JVM
      binary should be JAVA_HOME/bin/java.

      If ``JAVA_HOME`` is not set, then ``java`` available from the ``PATH``
      will be used. If the executable can not be found on the ``PATH`` the
      build will fail.

If no environment variable is set, then the default setting of
``sphinxcontrib.plantuml`` will be used, which is to run a wrapper called
``plantuml`` from the ``PATH``.

Please find the configuration process implementation in `docs/conf.py``.

--------------

.. _Sphinx: http://www.sphinx-doc.org/en/master/
.. _pip homepage: https://pip.pypa.io/en/stable/
.. _`Virtualenv documentation`: https://virtualenv.pypa.io/en/latest/

*Copyright (c) 2020-2021, Arm Limited and Contributors. All rights reserved.*

SPDX-License-Identifier: BSD-3-Clause
