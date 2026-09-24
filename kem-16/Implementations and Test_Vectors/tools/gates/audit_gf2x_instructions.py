#!/usr/bin/env python3
"""Hard instruction gate for platform-specific HARE GF2X objects."""
import argparse
import re
import subprocess
from pathlib import Path
from typing import List


def find_objects(build: Path, mode: str) -> List[Path]:
    marker = (
        "x86_64/avx256/gf2x.c.o"
        if mode == "x86-pclmul"
        else "aarch64/sve/gf2x.c.o"
    )
    return sorted(
        p for p in build.rglob("gf2x.c.o")
        if marker in p.as_posix()
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", required=True, type=Path)
    parser.add_argument(
        "--mode",
        required=True,
        choices=("x86-pclmul", "arm-pmull-on", "arm-pmull-off"),
    )
    parser.add_argument("--expected-count", required=True, type=int)
    parser.add_argument("--objdump", default="objdump")
    parser.add_argument("--log", type=Path)
    args = parser.parse_args()

    build = args.build_dir.resolve()
    if not build.is_dir():
        print(f"instruction audit: FAIL: build directory missing: {build}")
        return 1

    objects = find_objects(build, args.mode)
    if len(objects) != args.expected_count:
        print(
            "instruction audit: FAIL: "
            f"expected {args.expected_count} GF2X objects, found {len(objects)}"
        )
        for obj in objects:
            print(f"  {obj}")
        return 1

    pattern = re.compile(r"pclmul", re.IGNORECASE) if args.mode == "x86-pclmul" \
        else re.compile(r"\bpmull2?\b", re.IGNORECASE)
    instruction_line = re.compile(r"^\s*[0-9a-fA-F]+:\s")
    require_present = args.mode != "arm-pmull-off"
    records = []
    failed = False

    for obj in objects:
        try:
            proc = subprocess.run(
                [args.objdump, "-d", str(obj)],
                check=False,
                universal_newlines=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
            )
        except OSError as exc:
            print(f"instruction audit: FAIL: cannot execute {args.objdump}: {exc}")
            return 1
        if proc.returncode != 0:
            print(f"instruction audit: FAIL: objdump failed for {obj}")
            print(proc.stdout)
            return 1
        # Restrict matching to actual disassembly instruction lines.  Object
        # headers and file paths may contain strings such as "pmull-off"; those
        # must not be interpreted as instructions.
        matches = [
            line for line in proc.stdout.splitlines()
            if instruction_line.match(line) and pattern.search(line)
        ]
        records.append(f"===== {obj} =====")
        records.extend(matches if matches else ["<no matching instruction>"])
        present = bool(matches)
        if present != require_present:
            failed = True
            expected = "present" if require_present else "absent"
            print(f"instruction audit: FAIL: expected instruction {expected}: {obj}")

    text = "\n".join(records) + "\n"
    if args.log:
        args.log.parent.mkdir(parents=True, exist_ok=True)
        args.log.write_text(text, encoding="utf-8")
    else:
        print(text, end="")

    if failed:
        return 1
    print(
        f"instruction audit: PASS mode={args.mode} "
        f"objects={len(objects)} expectation={'present' if require_present else 'absent'}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
