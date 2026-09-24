#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0 or CC0-1.0
import argparse

from mupq import mupq
from mupq import platforms


def parse_arguments():
    parser = argparse.ArgumentParser(description="DTRU ARM Cortex-M4 delivery runner")
    parser.add_argument(
        "-p",
        "--platform",
        choices=["stm32f4discovery"],
        default="stm32f4discovery",
        help="Target platform",
    )
    parser.add_argument("-u", "--uart", default="/dev/ttyUSB0", help="UART device used for board output")
    parser.add_argument("--baud", type=int, default=38400, help="UART baud rate")
    parser.add_argument("-i", "--iterations", type=int, default=1, help="Benchmark iterations requested by the runner")
    return parser.parse_known_args()


def get_platform(args):
    if args.platform != "stm32f4discovery":
        raise NotImplementedError(f"Unsupported platform: {args.platform}")
    platform = platforms.StLink(args.uart, baud=args.baud)
    settings = M4Settings(args.platform, args.iterations)
    return platform, settings


class M4Settings(mupq.PlatformSettings):
    scheme_folders = [
        ("pqm4", "crypto_kem", ""),
    ]

    platform_memory = {
        "stm32f4discovery": 128 * 1024,
    }

    def __init__(self, platform, iterations=1):
        super().__init__()
        import skiplist

        self.iterations = iterations
        self.skip_list = []
        for impl in skiplist.skip_list:
            if impl.get("estmemory", 0) > self.platform_memory[platform]:
                impl = impl.copy()
                del impl["estmemory"]
                self.skip_list.append(impl)
        self.makeflags = []
        self.binary_type = "bin"
        self.makeflags.append(f"PLATFORM={platform}")
        self.makeflags.append(f"MUPQ_ITERATIONS={iterations}")
