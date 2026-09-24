#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0 or CC0-1.0
#
# Verify the DTRU ICCS-format KAT files shipped in Test_Vectors/.

import os
import sys

from interface import get_platform, parse_arguments
from mupq import mupq


IMPL_SUFFIX = {
    "ref": "",
    "m4fspeed": "_SPEED",
    "m4fstack": "_STACK",
}

TEST_VECTORS_DIR = "Test_Vectors"
PK_PACK_OPT_SUFFIX = "_PK_PACK_OPT"
PK_PACK_OPT_SCHEMES = {
    "dtru-648",
    "dtru-768",
    "dtru-1024",
    "dtru-1536",
    "dtru-2048",
}


def scheme_to_algname(scheme):
    parts = scheme.split("-")
    if len(parts) == 2 and parts[1].isdigit():
        return f"DTRU-{parts[1]}"
    if len(parts) == 2:
        return f"DTRU-{parts[1].capitalize()}"
    return scheme.upper()


def normalise(text):
    return text.strip().replace("\r\n", "\n")


def normalise_pk_pack_opt(value):
    value = str(value).strip().lower()
    if value in ("", "0", "false", "no", "off"):
        return "0"
    if value in ("1", "true", "yes", "on"):
        return "1"
    raise ValueError(f"Unsupported PK_PACK_OPT value: {value}")


def parse_kat_args(args):
    pk_pack_opt = os.environ.get("PK_PACK_OPT")
    filters = []

    for arg in args:
        if arg == "--pk-pack-opt":
            pk_pack_opt = "1"
        elif arg == "--no-pk-pack-opt":
            pk_pack_opt = "0"
        elif arg.startswith("--pk-pack-opt="):
            pk_pack_opt = arg.split("=", 1)[1]
        elif arg.startswith("PK_PACK_OPT="):
            pk_pack_opt = arg.split("=", 1)[1]
        else:
            filters.append(arg)

    return normalise_pk_pack_opt(pk_pack_opt or "0"), filters


def expected_path(implementation, pk_pack_opt):
    algname = scheme_to_algname(implementation.scheme)
    mode_suffix = PK_PACK_OPT_SUFFIX if pk_pack_opt == "1" else ""
    suffix = IMPL_SUFFIX.get(
        implementation.implementation,
        f"_{implementation.implementation.upper()}",
    )
    return os.path.join(TEST_VECTORS_DIR, f"KAT_KEM_{algname}{mode_suffix}{suffix}.txt")


def matches_filter(implementation, value):
    return value in {
        implementation.scheme,
        implementation.path,
        implementation.implementation,
        f"{implementation.primitive}/{implementation.scheme}/{implementation.implementation}",
    }


class KatVerifier(mupq.BoardTestCase):
    test_type = "testvectors_iccs"

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.pk_pack_opt = "0"

    def _set_pk_pack_opt(self, pk_pack_opt):
        self.pk_pack_opt = pk_pack_opt
        makeflags = [
            flag for flag in self.platform_settings.makeflags
            if not flag.startswith("PK_PACK_OPT=")
        ]
        makeflags.append(f"PK_PACK_OPT={pk_pack_opt}")
        self.platform_settings.makeflags = makeflags

    def run_test(self, implementation):
        output = super().run_test(implementation)
        if output == -1 or output == "ERROR" or "ERROR" in output:
            self.log.error("KAT %s failed to run on target", implementation)
            return -1

        path = expected_path(implementation, self.pk_pack_opt)
        if not os.path.isfile(path):
            self.log.error("Missing KAT file for %s: %s", implementation, path)
            return -1

        with open(path, "r", encoding="utf-8") as f:
            expected = f.read()

        if normalise(output) != normalise(expected):
            self.log.error("KAT %s does not match %s", implementation, path)
            return -1

        print(f"  KAT     {path}  == {implementation.implementation}")
        return 0

    def test_all(self, args):
        try:
            pk_pack_opt, filters = parse_kat_args(args)
        except ValueError as e:
            self.log.error("%s", e)
            return 1
        self._set_pk_pack_opt(pk_pack_opt)
        print(f"  MODE    PK_PACK_OPT={self.pk_pack_opt}")

        implementations = []
        exclude = "--exclude" in filters
        filters = [a for a in filters if a != "--exclude"]

        for implementation in self.get_implementations():
            if pk_pack_opt == "1" and implementation.scheme not in PK_PACK_OPT_SCHEMES:
                continue
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
