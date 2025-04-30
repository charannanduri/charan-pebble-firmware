#! /usr/bin/env python
# Copyright 2024 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


# elf_sections.py
# This script analyses the specified .elf and provides output similar to readelf -S.
# The sections are sorted by start address and gaps/overlaps in the memory map are displayed.

import argparse

from elftools.elf.elffile import ELFFile

EXCLUDED_SECTIONS = ['.log_strings']

BT_DIALOG_SECTION_START = ('** RAM Start **', 0x7FC0000, 0)
BT_DIALOG_SECTION_END = ('** RAM End **',   0x7FE3FFF, 0)


# Return a list of sections as a tuple (name, start address, size)
def _get_sections(elf, all_sections):
    headers = []

    for nsec, section in enumerate(elf.iter_sections()):
        if not all_sections:
            if section['sh_addr'] == 0 or section['sh_size'] == 0 or \
                    section.name in EXCLUDED_SECTIONS:
                continue
        headers.append((section.name, section['sh_addr'], section['sh_size']))

    return headers


def _process_elf(filename, verbose=False, all_sections=False, bt=False):
    with open(filename, 'rb') as f:
        elffile = ELFFile(f)

        # Sort by the second element (start address)
        sections = sorted(_get_sections(elffile, all_sections), key=lambda x: x[1])
        if bt:
            sections.insert(0, BT_DIALOG_SECTION_START)
            sections.append(BT_DIALOG_SECTION_END)

        print '%-20s   %10s   %7s   %16s\n' % \
            ('Section Name', 'Start Addr', 'Size', 'Gap Before Section')
        previous_section_end_addr = None
        for s in sections:
            # Handle the first sections
            if previous_section_end_addr is None:
                previous_section_end_addr = s[1] - 1
                gap = 0
            else:
                gap = s[1] - previous_section_end_addr - 1

            if gap == 0:
                print '%-20s   0x%08X   0x%05X' % (s[0], s[1], s[2])
            elif gap > 0:
                print '%-20s   0x%08X   0x%05X             0x%06X' % (s[0], s[1], s[2], gap)
            elif gap < 0:
                gap *= -1
                print '%-20s   0x%08X   0x%05X             0x%06X *** OVERLAP ***' % \
                    (s[0], s[1], s[2], gap)

            previous_section_end_addr = s[1] + s[2] - 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--verbose', action='store_true')
    parser.add_argument('-a', '--all', action='store_true', help='Show all sections')
    parser.add_argument('-b', '--bt', action='store_true', help='Dialog BT .elf')
    parser.add_argument('elf_file', help='Extracts section info from elf file.')
    args = parser.parse_args()

    _process_elf(args.elf_file, verbose=args.verbose, all_sections=args.all, bt=args.bt)
