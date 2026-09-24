#!/usr/bin/env python3
"""Require complete decapsulation-match lines in a HARE benchmark log."""
import argparse
import re
from pathlib import Path

PATTERN = re.compile(r"^decaps_shared_secret_match_count=(\d+)/(\d+)$")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("log", type=Path)
    parser.add_argument("--expected-lines", type=int, required=True)
    parser.add_argument("--expected-total", type=int, required=True)
    args = parser.parse_args()

    if not args.log.is_file():
        print(f"benchmark match gate: FAIL: missing log: {args.log}")
        return 1
    matches = []
    for line in args.log.read_text(encoding="utf-8", errors="replace").splitlines():
        match = PATTERN.match(line.strip())
        if match:
            matches.append((int(match.group(1)), int(match.group(2))))
    if len(matches) != args.expected_lines:
        print(
            "benchmark match gate: FAIL: "
            f"expected {args.expected_lines} lines, found {len(matches)}"
        )
        return 1
    for index, (good, total) in enumerate(matches, 1):
        if total != args.expected_total or good != total:
            print(
                "benchmark match gate: FAIL: "
                f"line {index} has {good}/{total}, expected "
                f"{args.expected_total}/{args.expected_total}"
            )
            return 1
    print(
        "benchmark match gate: PASS: "
        f"{args.expected_lines} lines, each {args.expected_total}/{args.expected_total}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
