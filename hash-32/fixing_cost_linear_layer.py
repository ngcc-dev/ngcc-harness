#!/usr/bin/env python3
"""Fixing-cost helper for hash-32: the ZC-m linear-layer checks of
hash-31/fixing_cost_linear_layer.py, with the submitted offsets read from the
ZC-EDMC reference permutations (ZC-EDMC and ZC-DMC ship identical ZuD1280-plain.c
and ZuD1536-plain.c). Only the linear layers are modelled.

Run from the repository root:
  python3 -B hash-32/fixing_cost_linear_layer.py [--min-weight]
"""
import sys

sys.dont_write_bytecode = True

import importlib.util
from pathlib import Path

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location(
    "zc_linear_layer", HERE.parent / "hash-31" / "fixing_cost_linear_layer.py")
zc = importlib.util.module_from_spec(spec)
spec.loader.exec_module(zc)
zc.SRC = HERE / "ZC-EDMC/Implementations/lib/low"

if __name__ == "__main__":
    zc.main()
