UEFI disk image creation instructions
=====================================

This directory contains a set of UEFI disk images, formatted with a
protective MBR and GPT. The images are intended for test purposes.
For convenience, there is a C version of the disk image that can be
built into test deployments.

Once the MBR and GPT have been added, for a sector size of 512 bytes,
the first usable LBA is 34. 34 free blocks are also needed at the top
of the LBA space to accommodate theh backup MBR and GPT.

Tools used to create images were:

  - gdisk
  - sgdisk
  - srec_cat (to generate C from binary image - http://srecord.sourceforge.net/)

Steps to create:

 1. Create a file to represent an empty flash device. All data set to
    0xff (the normal erased value). Use ('bs' is block size, 'count' is
    number of blocks)::

        dd if=/dev/zero bs=512 count=267 | tr "\000" "\377" >flash.img

 2. Create MBR+GPT using::

        # -N 1 - create partition 1 with max available size
        # -c 1:<name> Set the name of partition 1
        # -u 1:<GUID>> #set the uniq GUID
        sgdisk -N 1 -c 1:"PARTITION_1" -u 1:92f7d53b-127e-432b-815c-9a95b80d69b7 flash.img

    or alternatively run the interative ``gdisk`` tool::

        gdisk flash.img
          use 'n' command to add partitions
          use 'c' to define partition names
          use 'x' to allow extra functionality
          use 'c' to change partition GUID (see value above)
          use 'm' to return to normal mode
          use 'w' to save to file

 3. Convert the disk to C code::

        srec_cat flash.img -Binary -o ref_partition_data.c -C-Array ref_partition_data -INClude

Disk Image Descriptions
-----------------------

.. list-table::
    :header-rows: 1
    :widths: 30 70

    * - Image file
      - Description
    * - components/media/disk/disk_images/ref_partition.img
      - Block storage tests assume a reference partition configuration consisting of a set of four
        partitions of varying size. This is a UEFI disk image where two of the partitions are big
        enough for providing backend storage for the Internal Trusted Storage and Protected Storage
        services, at least from a TS test perspective.
    * - components/media/disk/disk_images/single_location_fw.img
      - This UEFI disk image contains the set of partitions you'd expect to find in flash for a
        device where firmware is stored in a single A/B pair of partitions.
    * - components/media/disk/disk_images/multi_location_fw.img
      - This UEFI disk image contains the set of partitions you'd expect to find in flash for a
        device where firmware is distributed across multiple locations. In this case, there are
        locations for AP firmware, SCP firmware and RSE firmware.


--------------

*Copyright (c) 2022-2023, Arm Limited and Contributors. All rights reserved.*

SPDX-License-Identifier: BSD-3-Clause