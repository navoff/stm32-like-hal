#!/usr/bin/env python3

from argparse import ArgumentParser
from zlib import crc32


def add_crc(filename, dry_run):
    with open(filename, 'rb') as file:
        content = file.read()
    crc = crc32(content[:-4]) ^ 0xFFFFFFFF

    print(f'crc:{crc:08X}')

    if dry_run:
        crc = crc32(content) ^ 0xFFFFFFFF
        print(f'full crc:{crc:08X}')
        return

    with open(filename, 'rb+') as file:
        file.seek(len(content) - 4)
        file.write(crc.to_bytes(4, 'little'))


def main():
    parser = ArgumentParser(
        description='Calculate crc32 of file (excluding last 4 bytes) and replace last 4 bytes of file by crc32'
    )
    parser.add_argument('filename', help='path to file')
    parser.add_argument('--dry-run', action='store_true', help='just print crc value and don\'t write to file')
    args = parser.parse_args()
    add_crc(args.filename, args.dry_run)


if __name__ == '__main__':
    main()
