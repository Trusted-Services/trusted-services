Contributing
============

Reporting Security Issues
-------------------------

Please follow the directions of the `Trusted Firmware Security Center`_

Getting Started
---------------

- Make sure you have a GitHub account and you are logged in at `review.trustedfirmware.org`_.
- Make sure the following is configured in `Gerrit settings`_:

    - Your public key is set under ``SSH Keys``. This will be needed when pushing changes.
      Please see `Public keys section`_ of Gerrit documentation if you need new keys.
    - The email address you will use in git is added under ``Email Addresses``.

- Clone the |TS_REPO| on your own machine.

Making Changes
--------------

- Send an email to the |TS_MAIL_LIST| about your work. This gives everyone visibility of whether others are working on
  something similar.
- Make commits of logical units. See these general `Git guidelines`_ for contributing to a project.
- Follow the :ref:`Coding Style & Guidelines`.
- Keep the commits on topic. If you need to fix another bug or make another enhancement, please create a separate
  change.
- Avoid long commit series. If you do have a long series, consider whether some
  commits should be squashed together or addressed in a separate topic.
- Make sure your commit messages are in the proper format. Please keep the 50/72 rule (for details see
  `Tim Popes blog entry`_.)
- Where appropriate, please update the documentation.

   - Consider which documents or other in-source documentation needs updating.
   - For topics with multiple commits, you should make all documentation changes (and nothing else) in the last commit
     of the series. Otherwise, include the documentation changes within the single commit.

- Ensure that files have the correct copyright and license information. Files that entirely consist of contributions to
  this project should have a copyright notice and BSD-3-Clause SPDX license identifier of the form as shown in
  :ref:`license`. Example copyright and license comment blocks are shown in :ref:`Coding Style & Guidelines`. Files that
  contain changes to imported Third Party IP files should retain their original copyright and license
  notices. For significant contributions you may add your own copyright notice in following format::

        Portions copyright (c) [XXXX-]YYYY, <OWNER>. All rights reserved.

  where `XXXX` is the year of first contribution (if different to `YYYY`) and `YYYY` is the year of most recent
  contribution. `<OWNER>` is your name or your company name.
- If you are submitting new files that you intend to be the technical sub-maintainer for (for example, a new platform
  port), then also update the :ref:`maintainers` file.

- Please test your changes.

Submitting Changes
------------------

- Ensure that each commit in the series has at least one ``Signed-off-by:`` line, using your real name and email
  address. The names in the ``Signed-off-by:`` and ``Author:`` lines must match. If anyone else contributes to the
  commit, they must also add their own ``Signed-off-by:`` line. By adding this line the contributor certifies the
  contribution is made under the terms of the :download:`Developer Certificate of Origin <../../dco.txt>`.

  More details may be found in the `Gerrit Signed-off-by Lines guidelines`_.
- Ensure that each commit also has a unique ``Change-Id:`` line. If you have cloned the repository with the "`Clone with
  commit-msg hook`" clone method, this should already be the case.

  More details may be found in the `Gerrit Change-Ids documentation`_.

- Select your target branch.

    - If all commits of your change compile and run ok, then your review can target the ``integration`` branch.
    - If not, a topic branch is needed. The name of the topic branch has to be kept reasonably sort and has to follow
      this format: ``topics/<user-id>/<topic>``.

        - `user-id` is unique ID of the user (e.g. nick name, <first name>_<last name>, etc...).
        - `topic` is a title reflecting the purpose of the change.
- Push your changes to Gerrit. Refer to the `Gerrit Uploading Changes documentation`_ to see how this can be done.

- The changes will then undergo further review and testing by the :ref:`maintainers`. Any review comments will be made
  directly on your patch. This may require you to do some rework.

- When the changes are accepted, the :ref:`maintainers` will integrate them.

- Typically, the :ref:`maintainers` will merge the changes into the target branch.

- If the changes are not based on a sufficiently-recent commit, or if they cannot be automatically rebased, then the
  :ref:`maintainers` may rebase it ask you to do so.

- After final integration testing, the changes will make their way into the ``main`` branch. If a problem is found
  during integration, the merge commit will be removed from the ``integration`` branch and the :ref:`maintainers` will
  ask you to create a new patch set to resolve the problem.

--------------

.. _review.trustedfirmware.org: https://review.trustedfirmware.org
.. _Git guidelines: http://git-scm.com/book/ch5-2.html
.. _Gerrit Uploading Changes documentation: https://review.trustedfirmware.org/Documentation/user-upload.html
.. _Gerrit Signed-off-by Lines guidelines: https://review.trustedfirmware.org/Documentation/user-signedoffby.html
.. _Gerrit Change-Ids documentation: https://review.trustedfirmware.org/Documentation/user-changeid.html
.. _`Tim Popes blog entry`: https://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html
.. _`Trusted Firmware Security Center`: https://trusted-firmware-docs.readthedocs.io/en/latest/security_center/index.html
.. _`Gerrit settings`: https://review.trustedfirmware.org/settings/
.. _`Public keys section`: https://review.trustedfirmware.org/Documentation/user-upload.html#configure_ssh_public_keys

*Copyright (c) 2020-2022, Arm Limited and Contributors. All rights reserved.*

SPDX-License-Identifier: BSD-3-Clause
