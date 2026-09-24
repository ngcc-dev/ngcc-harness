#!/usr/bin/env python3
# Minimal single-binary runner: flash one .bin via StLink and print board output.
# Usage: python3 measure_one.py <path-to-bin> [tty] [baud]
import sys
from mupq import platforms

binpath = sys.argv[1]
tty = sys.argv[2] if len(sys.argv) > 2 else "/dev/ttyUSB0"
baud = int(sys.argv[3]) if len(sys.argv) > 3 else 38400

p = platforms.StLink(tty, baud=baud, timeout=15)
with p:
    out = p.run(binpath)
print(out)
