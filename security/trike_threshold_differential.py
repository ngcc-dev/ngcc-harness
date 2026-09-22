#!/usr/bin/env python3
"""Build the literal TRIKE PDF threshold and compare whole-KEM correctness."""

import argparse
import json
from pathlib import Path
import shutil
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "kem-36"
SWEEP = ROOT / "security/kem_roundtrip_sweep.py"


def sweep(library: Path, trials: int) -> dict:
    proc = subprocess.run(
        ["python3", str(SWEEP), str(library), "--trials", str(trials), "--repeats", "1"],
        cwd=ROOT, text=True, stdout=subprocess.PIPE, check=True,
    )
    marker = '{\n  "completed_trials"'
    return json.loads(proc.stdout[proc.stdout.rfind(marker):])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--trials", type=int, default=32)
    args = ap.parse_args()
    with tempfile.TemporaryDirectory(prefix="ngcc-trike-max-") as name:
        target = Path(name) / "kem-36"
        shutil.copytree(
            SOURCE, target,
            ignore=shutil.ignore_patterns("build", "lib", "results"),
            symlinks=True,
        )
        makefile = target / "Makefile"
        text = makefile.read_text()
        text = text.replace("include ../api/", f"include {ROOT}/api/")
        makefile.write_text(text)
        changed = 0
        old = "uint32_t mask = -(fs < t);\n\n    return (t & ~mask) | (fs & mask);"
        new = "uint32_t mask = -(fs > t);\n\n    return (t & ~mask) | (fs & mask);"
        for decoder in target.glob("Implementations and Test_Vectors/Implementations/Reference_Implementation/TRIKE-*/src/decoder.c"):
            source = decoder.read_text()
            if old not in source:
                raise RuntimeError(f"threshold expression not found in {decoder}")
            decoder.write_text(source.replace(old, new))
            changed += 1
        if changed != 4:
            raise RuntimeError(f"expected four decoders, changed {changed}")
        subprocess.run(
            ["make", "-s", "-C", str(target), f"NGCC_ROOT={ROOT}", "-j4"],
            cwd=ROOT, check=True,
        )
        shipped = sweep(ROOT / "kem-36/lib/libTRIKE-2.so", args.trials)
        specified = sweep(target / "lib/libTRIKE-2.so", args.trials)
    result = {
        "terminal": "TRIKE_THRESHOLD_DIFFERENTIAL",
        "trials": args.trials,
        "changed_decoders": changed,
        "shipped_min_failures": shipped["failure_count"],
        "specified_max_failures": specified["failure_count"],
        "confirmed": shipped["failure_count"] == 0 and specified["failure_count"] == args.trials,
    }
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0 if result["confirmed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
