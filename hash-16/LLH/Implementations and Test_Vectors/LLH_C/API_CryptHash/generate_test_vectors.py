#!/usr/bin/env python3
"""Generate LLH API_CryptHash KAT files through implementation Makefiles.

Each LLH instance already provides a Makefile target that builds and runs its
normal KAT binary.  This wrapper only selects the implementation and instance,
then redirects that target's output copy step to the requested directory.
"""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


VARIANTS = ("LLH-256", "LLH-512", "LLH-768", "LLH-1024")
IMPL_DIRS = {
    "ref": "Reference_Implementation",
    "opt": "Optimized_Implementation",
}


def expand_variants(value: str) -> list[str]:
    if value == "all":
        return list(VARIANTS)

    selected: list[str] = []
    valid = set(VARIANTS)
    for item in value.split(","):
        item = item.strip()
        if item not in valid:
            raise SystemExit(f"unsupported variant: {item}")
        selected.append(item)
    return selected


def run_vectors_target(inst_dir: Path, output_dir: Path) -> None:
    if not (inst_dir / "Makefile").is_file():
        raise SystemExit(f"Makefile not found: {inst_dir}")

    output_dir.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        ["make", "vectors", f"TEST_VECTOR_DIR={output_dir}"],
        cwd=inst_dir,
        check=True,
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate LLH API_CryptHash test vectors through instance Makefiles.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument("--impl", choices=tuple(IMPL_DIRS), required=True,
                        help="implementation under test")
    parser.add_argument("--variant", default="all",
                        help="LLH variant, comma list, or all")
    parser.add_argument("--out", required=True,
                        help="directory where generated KAT files will be copied")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    api_root = Path(__file__).resolve().parent
    output_dir = Path(args.out).expanduser()
    if not output_dir.is_absolute():
        output_dir = Path.cwd() / output_dir

    impl_dir = api_root / "Implementations" / IMPL_DIRS[args.impl]
    variants = expand_variants(args.variant)
    for variant in variants:
        inst_dir = impl_dir / variant
        if not inst_dir.is_dir():
            raise SystemExit(f"implementation directory not found: {inst_dir}")

        print(f"{args.impl} {variant} -> {output_dir}")
        run_vectors_target(inst_dir, output_dir)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
