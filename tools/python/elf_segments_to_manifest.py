#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
#
# Copyright (c) 2021-2024, Arm Limited. All rights reserved.

"""
This module's goal is to take an ELF file as an input and extract the memory
regions that need to be configured on load. The memory region description is put
into the manifest file so the SPMC set up the memory layout when loading a
binary format SP.
"""

from enum import IntFlag
from math import ceil
from elftools import __version__ as module_version
from elftools.elf.elffile import ELFFile
from elftools.elf.constants import P_FLAGS

assert module_version == "0.31"

class ElfSegmentsToManifest:
    """
    The class loads an ELF file and builds up an internal represention of its
    memory layout then it can write this information into a manifest file.
    """
    PAGE_SIZE = 4096

    class GnuNotePropertySection:
        """ Provides an API to process GNU note property section. """

        GNU_PROPERTY_AARCH64_FEATURE_1_BTI = 1

        def __init__(self, section):
            self.section = section

        def is_bti_enabled(self):
            """ Returns whether any of the notes has a BTI enable property """

            def is_bti_property(prop):
                return prop['pr_type'] == "GNU_PROPERTY_AARCH64_FEATURE_1_AND" and \
                    prop['pr_data'] & self.GNU_PROPERTY_AARCH64_FEATURE_1_BTI

            def has_bti_property(note):
                return note["n_name"] == 'GNU' and note["n_type"] == "NT_GNU_PROPERTY_TYPE_0" and \
                    any(is_bti_property(p) for p in note["n_desc"])

            return any(has_bti_property(n) for n in self.section.iter_notes())

        @staticmethod
        def is_matching_section(section):
            """ Checks if the section is a GNU note property section """
            return section.name == '.note.gnu.property'


    class Region:
        """ Describes a memory region and its attributes. """

        class ManifestMemAttr(IntFlag):
            """ Type for describing the memory flags of a region. """
            R = 0x01
            W = 0x02
            X = 0x04
            S = 0x08
            GP = 0x10

            def get_attr(self):
                """ Queries the value of the attributes in manifest format. """
                return self.value

            def __str__(self):
                return ",".join([str(f.name) for f in __class__ if f in self])

            @staticmethod
            def from_p_flags(p_flags):
                """ Creates an instanced initialized by p_flags. """
                instance = 0
                instance |= __class__.R if p_flags & P_FLAGS.PF_R else 0
                instance |= __class__.W if p_flags & P_FLAGS.PF_W else 0
                instance |= __class__.X if p_flags & P_FLAGS.PF_X else 0
                return instance

        class LoadFlags(IntFlag):
            """ Type for describing the memory load flags of a region. """
            NO_BITS = 0x01

            def get_flags(self):
                """ Queries the value of the flags in manifest format. """
                return self.value

            def is_compatible(self, other):
                """ Return true of the other flags can be merged with self. """
                return (self & self.NO_BITS) == (other & self.NO_BITS)

            def __str__(self):
                return ",".join([str(f.name) for f in __class__ if f in self])

        def __init__(self, segment, section):
            segment = segment.segment
            sec_h = section.header
            self.start_address = sec_h.sh_addr
            self.end_address = sec_h.sh_addr + sec_h.sh_size
            self.attributes = self.ManifestMemAttr.from_p_flags(segment.header.p_flags)
            self.load_flags = self.LoadFlags(0)
            self.load_flags |= self.LoadFlags.NO_BITS if sec_h.sh_type == "SHT_NOBITS" else 0
            self.sections = [section.name]

        def is_compatible_region(self, region):
            """ Checks if the other region has compatible attributes/flags. """
            return self.load_flags.is_compatible(region.load_flags)

        def append_region(self, region):
            """ Extends the region by the other region. """
            self.end_address = region.end_address
            self.sections += region.sections

        def set_bti_if_executable(self):
            """ Sets GP flag if the region is executable. """
            if self.attributes & self.ManifestMemAttr.X:
                self.attributes |= self.ManifestMemAttr.GP

        def write_manifest(self, load_base_addr, manifest_file):
            """
            Writes the region into the manifest file. The address is adjusted by load_base_address.
            """
            manifest_file.write(f"{self.generate_region_name(load_base_addr)} {{\n")
            manifest_file.write(f"\t/* {self.generate_section_list()} */\n")
            manifest_file.write(f"\t{self.serialize_offset(load_base_addr)}\n")
            manifest_file.write(f"\t{self.serialize_pages_count()}\n")
            manifest_file.write(f"\t{self.serialize_attributes()}\n")
            manifest_file.write(f"\t{self.serialize_load_flags()}\n")
            manifest_file.write("};\n")

        def generate_region_name(self, load_base_addr):
            """ Generates a name for the region using the region_[start address] pattern. """
            return f"region_{self.start_address - load_base_addr:x}"

        def generate_section_list(self):
            """ Lists the name of member sections of the region. """
            return ", ".join(self.sections)

        def serialize_offset(self, load_base_addr):
            """ Calculates and outputs the offset of the region in manifest format. """
            base = self.start_address - load_base_addr
            end = self.end_address - load_base_addr
            high, low = (base >> 32) & 0xffffffff, base & 0xffffffff
            return f"load-address-relative-offset = <0x{high:x} 0x{low:x}>;\t" + \
                f"/* 0x{base:x} - 0x{end:x} */"

        def serialize_pages_count(self):
            """ Calculates and outputs the page count of the region in manifest format. """
            region_length = self.end_address - self.start_address
            pages_count = ceil(region_length / ElfSegmentsToManifest.PAGE_SIZE)
            return f"pages-count = <{pages_count}>;\t/* {region_length} bytes */"

        def serialize_attributes(self):
            """ Generates the memory region attribute value in manifest format. """
            return f"attributes = <{self.attributes.get_attr()}>;\t/* {self.attributes} */"

        def serialize_load_flags(self):
            """ Generates the memory region load flags value in manifest format. """
            return f"load-flags = <{self.load_flags.get_flags()}>;\t/* {self.load_flags} */"

    class Segment:
        """ Stores a segment and its sections. Able to produce a region list. """
        def __init__(self, segment, sections):
            def is_aligned(segment):
                return segment.header.p_align == ElfSegmentsToManifest.PAGE_SIZE
            assert is_aligned(segment), "Segments must be 4k aligned, check LD script"
            self.segment = segment
            self.sections = []
            self.gnu_note = None

            for section in sections:
                if self.segment.section_in_segment(section):
                    if ElfSegmentsToManifest.GnuNotePropertySection.is_matching_section(section):
                        self.gnu_note = ElfSegmentsToManifest.GnuNotePropertySection(section)
                    else:
                        self.sections.append(section)

            self.regions = []
            self.merge_sections_to_regions()

        def get_load_address(self):
            """ Queries the load address of the region. """
            return self.segment.header.p_vaddr

        def merge_sections_to_regions(self):
            """ Merges consecutive sections with comptabile attributes/flags into regions. """
            current_region = None
            for section in self.sections:
                region = ElfSegmentsToManifest.Region(self, section)
                if current_region and current_region.is_compatible_region(region):
                    current_region.append_region(region)
                else:
                    self.regions.append(region)
                    current_region = region

            # Set GP only for the executable regions if BTI is enabled in the segment
            if self.gnu_note and self.gnu_note.is_bti_enabled():
                for region in self.regions:
                    region.set_bti_if_executable()

        def write_manifest(self, load_base_addr, manifest_file):
            """ Writes the regions into the manifest file. """
            for region in self.regions:
                region.write_manifest(load_base_addr, manifest_file)

    def __init__(self):
        self.segments = []
        self.load_base_addr = None

    def read_elf(self, elf_file_fp):
        """ Reads and parses the sections and segments of the ELF file. """
        elf_file = ELFFile(elf_file_fp)
        segments = elf_file.iter_segments()

        def is_load(segment):
            return segment.header.p_type == "PT_LOAD"
        self.segments = [self.Segment(s, elf_file.iter_sections()) for s in segments if is_load(s)]
        self.load_base_addr = min([s.get_load_address() for s in self.segments])

    def write_manifest(self, manifest_file):
        """ Writes the memory regions of each segment into the manifest. """
        for segment in self.segments:
            segment.write_manifest(self.load_base_addr, manifest_file)


if __name__ == "__main__":
    import sys

    ELF_SEGMENTS_TO_MANIFEST = ElfSegmentsToManifest()

    with open(sys.argv[1], "rb") as fp:
        ELF_SEGMENTS_TO_MANIFEST.read_elf(fp)

    with open(sys.argv[2], "wt", encoding="ascii") as fp:
        ELF_SEGMENTS_TO_MANIFEST.write_manifest(fp)
