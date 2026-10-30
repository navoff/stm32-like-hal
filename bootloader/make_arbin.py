#!/usr/bin/env python3
"""
Build an .arbin image for an ARB bootloader from a firmware .bin.

The firmware .bin produced by the linker ends with a 128-byte fw_info_t structure
(see version/fw_info.h). Its last field, fw_crc, is the CRC-32 of the whole image computed so
that the CRC of the complete .bin (body followed by the structure) is zero. version/add_crc.py
stamps that field after the build.

The .arbin file is the same data with the structure moved to the front:

    fw_info_t (128 bytes) | firmware body (the .bin without the last 128 bytes)

so a bootloader can read the platform type, hardware type and version before receiving the
body, and verify the body by restoring the original order and checking that the CRC is zero.

If fw_crc has not been stamped yet (the field is 0xFFFFFFFF), the script stamps it exactly like
add_crc.py does. If it has been stamped, the script verifies it and refuses to produce an image
with a wrong checksum.
"""

import argparse
import struct
import sys
from zlib import crc32

FW_INFO_SIZE = 128
FW_INFO_MAGIC = 0x2B3386EC
FW_CRC_OFFSET = FW_INFO_SIZE - 4
CRC_NOT_STAMPED = 0xFFFFFFFF


def crc32_ethernet(data):
    """CRC-32 with the Ethernet polynomial, reflected, init 0xFFFFFFFF, no final XOR.
    Matches checksum_crc32_ethernet_calculate() in utils/checksum.c and add_crc.py."""
    return crc32(data) ^ 0xFFFFFFFF


def create_arbin(bin_data):
    """Return the .arbin image for a firmware .bin, stamping or verifying fw_crc."""
    if len(bin_data) <= FW_INFO_SIZE:
        raise ValueError("firmware image is shorter than the fw_info structure")

    body = bin_data[:-FW_INFO_SIZE]
    info = bytearray(bin_data[-FW_INFO_SIZE:])

    (magic,) = struct.unpack_from("<L", info, 0)
    if magic != FW_INFO_MAGIC:
        raise ValueError(
            "fw_info magic number mismatch: 0x%08X, expected 0x%08X; "
            "is the .fw_info section placed at the end of the image?" % (magic, FW_INFO_MAGIC))

    (fw_crc,) = struct.unpack_from("<L", info, FW_CRC_OFFSET)
    if fw_crc == CRC_NOT_STAMPED:
        fw_crc = crc32_ethernet(bin_data[:-4])
        struct.pack_into("<L", info, FW_CRC_OFFSET, fw_crc)
        print("fw_crc stamped: 0x%08X" % fw_crc)
    elif crc32_ethernet(bin_data) != 0:
        raise ValueError("fw_crc 0x%08X does not match the image" % fw_crc)
    else:
        print("fw_crc verified: 0x%08X" % fw_crc)

    return bytes(info) + body


def main():
    parser = argparse.ArgumentParser(
        description="Create an .arbin file for flashing through an ARB bootloader")
    parser.add_argument("bin_file", help="source firmware .bin")
    parser.add_argument("--out", "-o", help="output file (default: <bin_file>.arbin)")
    args = parser.parse_args()

    out = args.out or (args.bin_file.rsplit(".", 1)[0] + ".arbin")

    with open(args.bin_file, "rb") as f:
        bin_data = f.read()

    try:
        arbin = create_arbin(bin_data)
    except ValueError as e:
        print("error: %s" % e, file=sys.stderr)
        return 1

    with open(out, "wb") as f:
        f.write(arbin)
    print("written %s (%d bytes)" % (out, len(arbin)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
