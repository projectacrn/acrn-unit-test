#!/usr/bin/env python

import sys, os
import argparse

version_string = "acrn-unit-test"

def main(args):
    out_f = open(args.out_file, "wb")
    binary_f = open(args.raw_file, "rb")
    binary_buf = binary_f.read()

    # Sector #1

    # # till 01F0: 0's
    out_f.write("\x00" * 0x1f1)

    # # 01F1/1: The size of the setup in sectors
    out_f.write("\x02")

    # # remaining till last 2 bytes of sector #1: 0's
    out_f.write("\x00" * (0x200 - 2 - 0x1f2))
    out_f.write("\x55\xAA")

    # Sector #2: zero page header

    # 0200/2: jump instruction
    out_f.write("\xEB\x66")

    # 0202/4: magic signature "HdrS"
    out_f.write("HdrS")

    # 0206/2: boot protocol version: 2.06
    out_f.write("\x06\x02")

    # 0208/4: realmode switch
    out_f.write("\x00" * 4)

    # 020C/2 (obsolete)
    out_f.write("\x00" * 2)

    # 020E/2: Pointer to kernel version string at the third sector
    out_f.write("\x00\x02")

    # 0210/1: Boot loader identifier (set by boot loader)
    out_f.write("\x00")

    # 0211/1: Boot protocol option flags: load at 1M
    out_f.write("\x01")

    # 0212/2: (for real-mode kernel)
    out_f.write("\x00" * 2)

    # 0214/4: code32_start: 1M
    out_f.write("\x00\x00\x10\x00")

    # 0218/4: ramdisk_image (set by boot loader)
    out_f.write("\x00" * 4)

    # 021C/4: initrd size (set by boot loader)
    out_f.write("\x00" * 4)

    # 0220/4: DO NOT USE
    out_f.write("\x00" * 4)

    # 0224/2: Free memory after setup end (write)
    out_f.write("\x00" * 2)

    # 0226/1: Extended boot loader version
    out_f.write("\x00")

    # 0227/1: Extended boot loader ID
    out_f.write("\x00")

    # 0228/4: 32-bit pointer to the kernel command line (write)
    out_f.write("\x00" * 4)

    # 022C/4: Highest legal initrd address (2G - 1)
    out_f.write("\xFF\xFF\xFF\x7F")

    # 0230/4: Physical addr alignment required for kernel (reloc)
    out_f.write("\x00\x00\x00\x00")

    # 0234/1: Whether kernel is relocatable or not (reloc)
    out_f.write("\x01")

    # 0235/1 (2.10+): Minimum alignment, as a power of two (reloc)
    out_f.write("\x00")

    # 0236/2 (2.12+): Boot protocol option flags
    out_f.write("\x00" * 2)

    # 0238/4: Maximum size of the kernel command line (0x07FF)
    out_f.write("\xFF\x07\x00\x00")

    # 023C/4 (2.07+): Hardware subarchitecture
    # 0240/8 (2.07+): Subarchitecture-specific data
    # Both defaults to 0
    out_f.write("\x00" * (4 + 8))

    # 0248/4 (2.08+): Offset of kernel payload
    # 024C/4 (2.08+): Length of kernel payload
    # Both 0
    out_f.write("\x00" * (4 + 4))

    # 0250/8 (2.09+): 64-bit physical pointer to linked list
    out_f.write("\x00" * 8)

    # 0258/8 (2.10+): Preferred loading address
    #
    # ACRN unit test images are statically linked at 4M and prepended by 4
    # sectors (i.e. 2K). Tell a bootloader that the preferred load address is 4M - 2K.
    pref_addr = 0x400000 - 512 * 4
    for b in [((pref_addr >> i) & 0xff) for i in (0,8,16,24)]:
        out_f.write(chr(b))
    out_f.write("\x00" * 4)

    # 0260/4 (2.10+): Linear memory required during initialization
    out_f.write("\x00" * 4)

    # 0264/4 (2.11+): Offset of handover entry point
    out_f.write("\x00" * 4)

    # remaining (except last 4 bytes): all 0
    out_f.write("\x00" * (0x400 - 0x268 - 4))

    # 03FC/4 (customized): size of the binary
    binary_size = len(binary_buf)
    for b in [((binary_size >> i) & 0xff) for i in (0,8,16,24)]:
        out_f.write(chr(b))

    # Sector #3: The version string at 0400, max 0200 bytes
    out_f.write(version_string + "\x00" * (0x200 - len(version_string)))

    # Sector $4: relocator code
    with open(args.relocator, "rb") as in_f:
        buf = in_f.read()
        max_size = 0x200
        if len(buf) > max_size:
            print "Relocator too large: %d bytes (max %d bytes)" % (len(buf), max_size)
            sys.exit(1)
        out_f.write(buf)
        out_f.write("\x00" * (max_size - len(buf)))

    # Sector #5 and onwards: The binary
    out_f.write(binary_buf)

    out_f.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Convert an test in raw to a bzImage")
    parser.add_argument("raw_file")
    parser.add_argument("relocator")
    parser.add_argument("out_file")
    args = parser.parse_args()

    main(args)
