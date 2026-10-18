#!/usr/bin/env python3
"""Run the shared deterministic runtime survey with HAWK's inventory."""

from pathlib import Path


shared = Path(__file__).resolve().parents[1] / "FAEST" / "survey_runtime.py"
exec(compile(shared.read_text(encoding="utf-8"), str(shared), "exec"))
