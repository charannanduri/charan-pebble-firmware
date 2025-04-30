#!/usr/bin/env python
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


from __future__ import print_function
import argparse
import logging
import pulse
import sys


def auto_int(num):
    return int(num, 0)


def main():
    parser = argparse.ArgumentParser(description="A tool to load binary data from Pebble.")
    parser.add_argument('-v', '--verbose', action='store_true', help='print verbose status output')
    parser.add_argument('-p', '--progress', action='store_true', help='print progress output')
    parser.add_argument('--operation', choices=['read', 'stat'], default='read')
    parser.add_argument('-o', '--out', help='filename to write', default=sys.stdout,
                        type=argparse.FileType('wb', 0))
    parser.add_argument('tty', metavar='TTY', help='the target serial port')

    subparsers = parser.add_subparsers(help='commands', dest='domain')

    external_flash_parser = subparsers.add_parser('external-flash')
    external_flash_parser.add_argument('offset', help='offset to read from', type=auto_int)
    external_flash_parser.add_argument('length', help='length to read', type=auto_int)

    memory_parser = subparsers.add_parser('memory')
    memory_parser.add_argument('offset', metavar='address', help='address to read from',
                               type=auto_int)
    memory_parser.add_argument('length', help='length to read', type=auto_int)

    coredump_parser = subparsers.add_parser('coredump')
    coredump_parser.add_argument('identifier', metavar='slot',
                                 help='slot index to retrieve coredump from', type=int)

    pfs_parser = subparsers.add_parser('pfs')
    pfs_parser.add_argument('identifier', metavar='filename', help='filename of file to retrieve')

    framebuffer_parser = subparsers.add_parser('framebuffer')

    args = parser.parse_args()

    if args.verbose:
        logging.basicConfig(level=logging.INFO)
    else:
        logging.basicConfig(level=logging.WARNING)

    args.domain = args.domain.replace('-', '_')

    with pulse.Connection.open_dbgserial(args.tty) as connection:
        connection.change_baud_rate(921600)

        try:
            method = getattr(connection.read, '_'.join((args.operation, args.domain)))
        except AttributeError:
            print("Domain {!r} doesn't support method {!r}".format(args.domain, args.operation),
                  file=sys.stderr)
            sys.exit(1)

        call_arg_keys = ['offset', 'length', 'identifier']
        call_args = {arg: vars(args)[arg] for arg in call_arg_keys if arg in args}
        data = method(**call_args)

        if args.operation == 'stat':
            data = str(data) + '\n'

        args.out.write(data)

if __name__ == '__main__':
    main()
