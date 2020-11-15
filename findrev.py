#!/usr/bin/env python
import sys

f = sys.argv[1]
board = sys.argv[2]
f = open(f)
official = False
if len(sys.argv) > 3 and sys.argv[3] == "official":
    official = True
board_found = False
for l in f:
    if l.startswith("#if (ARCH & ARCH_") or l.startswith("#elif (ARCH & ARCH_"):
        b = l.split("_", 1)[1].rstrip(") \n\r\t").lower()
        if b == board:
            board_found = True

    if board_found:
        if l.strip().startswith("#define VERS_MAJOR"):
            major = l.split()[2]
        if l.strip().startswith("#define VERS_MINOR"):
            minor = l.split()[2]
        if l.strip().startswith("#define VERS_REV"):
            rev = l.split()[2]
        if l.strip().startswith("#define VERS_RC"):
            rc = l.split()[2]
        if l.strip().startswith("#define FIRMWARE_VERSION"):
            version = l.split()[2]
            version = version.strip('"')
            print "%s" % version
            board_found = False

