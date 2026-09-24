#!/usr/bin/env python3
"""Build all implementations and apps for selected schemes.

Usage:
    python3 build_schemes.py
    python3 build_schemes.py all
    python3 build_schemes.py PLATFORM=mps2-an386 DKE-128 DKE-512
    python3 build_schemes.py PLATFORM=nucleo-l4r5zi DKE-128 USE_SM3_ASM=1 -j8
    python3 build_schemes.py PLATFORM=stm32f4discovery DKE-128 DKE-512

The script mirrors the implementation/app discovery used by mk/scheme.mk:
implementations are directories containing the family entry source, and apps
are top-level .c files inside each crypto_* family directory.
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


FAMILIES = {
    "crypto_kem": "KEM_AlgorithmInstance.c",
    "crypto_kex": "KEX_AlgorithmInstance.c",
    "crypto_sign": "SIGN_AlgorithmInstance.c",
}

SUPPORTED_TARGET_PLATFORMS = ("mps2-an386", "nucleo-l4r5zi", "stm32f4discovery")
DEFAULT_TARGET_PLATFORM = "nucleo-l4r5zi"


@dataclass(frozen=True)
class Implementation:
    family: str
    scheme: str
    name: str
    path: Path

    @property
    def stem(self) -> str:
        return f"{self.family}_{self.scheme}_{self.name}"


def parse_supported_platforms(root: Path) -> set[str]:
    config = root / "mk" / "config.mk"
    platforms: set[str] = set()

    if not config.exists():
        return platforms

    for line in config.read_text(encoding="utf-8").splitlines():
        line = line.split("#", 1)[0].strip()
        if line.startswith("SUPPORTED_PLATFORMS"):
            _, _, value = line.partition(":=")
            platforms.update(value.split())
            break

    return platforms


def family_apps(root: Path, family: str) -> list[str]:
    family_dir = root / family
    if not family_dir.is_dir():
        return []

    return sorted(source.stem for source in family_dir.glob("*.c"))


def discover_implementations(root: Path, requested_schemes: set[str]) -> list[Implementation]:
    implementations: list[Implementation] = []

    for family, entry_file in FAMILIES.items():
        family_dir = root / family
        if not family_dir.is_dir():
            continue

        for scheme_dir in sorted(path for path in family_dir.iterdir() if path.is_dir()):
            if requested_schemes and scheme_dir.name not in requested_schemes:
                continue

            for impl_dir in sorted(path for path in scheme_dir.iterdir() if path.is_dir()):
                if (impl_dir / entry_file).is_file():
                    implementations.append(
                        Implementation(
                            family=family,
                            scheme=scheme_dir.name,
                            name=impl_dir.name,
                            path=impl_dir,
                        )
                    )

    return implementations


def parse_args(argv: list[str]) -> tuple[argparse.Namespace, str, list[str], list[str]]:
    parser = argparse.ArgumentParser(
        description="Build all existing implementations and family apps for selected schemes.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "schemes",
        nargs="*",
        help="Scheme names to build, for example DKE-128 DKE-256. Use 'all' or omit schemes to build all discovered schemes.",
    )
    parser.add_argument(
        "-j",
        "--jobs",
        default=None,
        help="Forwarded make parallelism, for example -j8.",
    )
    parser.add_argument(
        "-n",
        "--dry-run",
        action="store_true",
        help="Print the discovered build commands without running make.",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="Only list discovered implementations and apps.",
    )
    parser.add_argument(
        "--fail-fast",
        action="store_true",
        help="Stop after the first failed make command.",
    )

    platform = ""
    make_vars: list[str] = []
    parser_args: list[str] = []

    for arg in argv:
        if "=" in arg and not arg.startswith("-"):
            key, value = arg.split("=", 1)
            if not key or not value:
                raise SystemExit(f"invalid make variable argument: {arg!r}")
            if key == "PLATFORM":
                platform = value
            else:
                make_vars.append(arg)
        else:
            parser_args.append(arg)

    args = parser.parse_args(parser_args)

    if not platform:
        platform = os.environ.get("PLATFORM", DEFAULT_TARGET_PLATFORM)

    return args, platform, make_vars, args.schemes


def normalize_requested_schemes(schemes: list[str]) -> set[str]:
    if not schemes or any(scheme.lower() == "all" for scheme in schemes):
        return set()
    return set(schemes)


def build_command(
    platform: str,
    impl: Implementation,
    apps: list[str],
    make_vars: list[str],
    jobs: str | None,
) -> list[str]:
    targets = [f"{impl.stem}_{app}" for app in apps]
    command = ["make", f"PLATFORM={platform}", *make_vars, *targets]

    if jobs:
        command.append(jobs if jobs.startswith("-j") else f"-j{jobs}")

    return command


def main(argv: list[str]) -> int:
    root = Path(__file__).resolve().parent
    args, platform, make_vars, schemes = parse_args(argv)

    supported_platforms = set(SUPPORTED_TARGET_PLATFORMS)
    if platform not in supported_platforms:
        print(
            f"error: unsupported PLATFORM={platform!r}; supported: "
            f"{' '.join(SUPPORTED_TARGET_PLATFORMS)}",
            file=sys.stderr,
        )
        return 2

    requested_schemes = normalize_requested_schemes(schemes)
    implementations = discover_implementations(root, requested_schemes)
    discovered_schemes = {impl.scheme for impl in implementations}
    missing_schemes = sorted(requested_schemes - discovered_schemes)

    if missing_schemes:
        print(
            "warning: no implementations found for scheme(s): "
            + ", ".join(missing_schemes),
            file=sys.stderr,
        )

    if not implementations:
        print("error: no matching implementations found", file=sys.stderr)
        return 1

    print(f"PLATFORM={platform}")
    print(f"Selected implementations: {len(implementations)}")

    failures: list[tuple[Implementation, int]] = []

    for impl in implementations:
        apps = family_apps(root, impl.family)
        if not apps:
            print(f"skip: {impl.family}/{impl.scheme}/{impl.name}: no family apps found")
            continue

        command = build_command(platform, impl, apps, make_vars, args.jobs)
        print(f"\n==> {impl.family}/{impl.scheme}/{impl.name}")
        print("apps: " + " ".join(apps))
        print("+ " + " ".join(command))

        if args.list or args.dry_run:
            continue

        result = subprocess.run(command, cwd=root)
        if result.returncode != 0:
            failures.append((impl, result.returncode))
            if args.fail_fast:
                break

    if args.list or args.dry_run:
        return 0

    if failures:
        print("\nFailed builds:", file=sys.stderr)
        for impl, code in failures:
            print(f"  {impl.family}/{impl.scheme}/{impl.name}: exit {code}", file=sys.stderr)
        return 1

    print("\nAll requested builds completed successfully.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
