#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0 or CC0-1.0
#
# Verify the AFS-KEX ICCS-format KAT files shipped in Test_Vectors/.

import os
import sys

from mupq import mupq
from interface import parse_arguments, get_platform


IMPL_SUFFIX = {
    'ref': '',
    'm4fspeed': '_SPEED',
    'm4fstack': '_STACK',
}

TEST_VECTORS_DIR = 'Test_Vectors'


def scheme_to_algname(scheme):
    parts = scheme.split('-')
    return f"{parts[0].upper()}_{parts[1].upper()}_C{parts[2]}"


def expected_path(implementation):
    algname = scheme_to_algname(implementation.scheme)
    suffix = IMPL_SUFFIX.get(
        implementation.implementation,
        f"_{implementation.implementation.upper()}",
    )
    return os.path.join(TEST_VECTORS_DIR, f"KAT_KEX_{algname}{suffix}.txt")


def normalise(text):
    return text.strip().replace('\r\n', '\n')


def matches_filter(implementation, value):
    return value in {
        implementation.scheme,
        implementation.path,
        implementation.implementation,
        f"{implementation.primitive}/{implementation.scheme}/{implementation.implementation}",
    }


class KatVerifier(mupq.BoardTestCase):
    test_type = 'testvectors_iccs'

    def run_test(self, implementation):
        output = super().run_test(implementation)
        if output == -1 or output == 'ERROR' or 'ERROR' in output:
            self.log.error("KAT %s failed to run on target", implementation)
            return -1

        path = expected_path(implementation)
        if not os.path.isfile(path):
            self.log.error("Missing KAT file for %s: %s", implementation, path)
            return -1

        with open(path, 'r', encoding='utf-8') as f:
            expected = f.read()

        if normalise(output) != normalise(expected):
            self.log.error("KAT %s does not match %s", implementation, path)
            return -1

        print(f"  KAT     {path}  == {implementation.implementation}")
        return 0

    def test_all(self, args):
        implementations = []
        exclude = "--exclude" in args
        filters = [a for a in args if a != "--exclude"]

        for implementation in self.get_implementations():
            matched = any(matches_filter(implementation, f) for f in filters)
            if exclude and matched:
                continue
            if not exclude and filters and not matched:
                continue
            implementations.append(implementation)

        if not implementations:
            self.log.error("No implementations selected (filters: %s)", filters)
            return 1

        for implementation in implementations:
            if self.run_test(implementation) == -1:
                print(f"{implementation} FAILED")
                return 1
            print(f"{implementation} SUCCESSFUL")
        return 0


if __name__ == "__main__":
    args, rest = parse_arguments()
    platform, settings = get_platform(args)
    with platform:
        test = KatVerifier(settings, platform)
        sys.exit(test.test_all(rest))
