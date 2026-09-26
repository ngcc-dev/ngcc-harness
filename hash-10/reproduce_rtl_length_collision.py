#!/usr/bin/env python3
"""Compile the four FEILIAN RTL cores and test the length-domain collision."""

import shutil
import subprocess
import tempfile
from pathlib import Path


HERE = Path(__file__).resolve().parent
RTL = HERE / "FEILIAN" / "Implementations" / "Additional_Implementation"
TB = HERE / "reproduce_rtl_length_collision.sv"


def simulate(name: str, expected: str, port_i: bool = False) -> None:
    source = RTL / name
    with tempfile.TemporaryDirectory(prefix=f"feilian-{name}-") as directory:
        obj = Path(directory) / "obj"
        command = [
            "verilator",
            "--binary",
            "--timing",
            "-Wno-fatal",
            "--top-module",
            "tb",
            "--Mdir",
            str(obj),
        ]
        if port_i:
            command.append("-DPORT_I")
        command.extend(
            str(source / filename)
            for filename in (
                "feilian_pkg.sv",
                "subcolumn.sv",
                "round.sv",
                "compression.sv",
                "core.sv",
            )
        )
        command.append(str(TB))
        built = subprocess.run(command, text=True, capture_output=True)
        if built.returncode:
            raise RuntimeError(built.stdout + built.stderr)
        run = subprocess.run([str(obj / "Vtb")], text=True, capture_output=True, check=True)
        if expected not in run.stdout:
            raise RuntimeError(f"{name}: expected {expected!r}, got {run.stdout!r}")

    if expected == "COLLISION":
        print(f"ATTACK hash-10-2 {name} CONFIRMED: H(61) = H(6100)")
    else:
        print(f"CONTROL hash-10-2 {name} NOT-CONFIRMED: inclusive counter separates the pair")


def main() -> None:
    if shutil.which("verilator") is None:
        raise SystemExit("verilator is required for the RTL witness")
    simulate("FEILIAN_1SC", "COLLISION")
    simulate("FEILIAN_2SC", "DISTINCT")
    simulate("FEILIAN_4SC", "COLLISION")
    simulate("FEILIAN_8SC", "COLLISION", port_i=True)


if __name__ == "__main__":
    main()
