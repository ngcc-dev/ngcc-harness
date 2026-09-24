#!/usr/bin/env python3
"""MAMBA-NIKE x86 implementation self-evaluation runner.

This program evaluates the code against the claims in the x86 self-evaluation
report.  It is intentionally read-only with respect to cryptographic parameters:
``parameters.json`` and every submitted ``params.h`` file are hashed before the
run and checked again at the end.

The runner has no third-party Python dependency.  Optional rejection analysis
uses the repository's ``compute_rejection.py`` and therefore requires NumPy.

Typical use:

    python3 ci/self_evaluation.py --mode conformance \
        --json /tmp/mamba-self-evaluation.json \
        --markdown /tmp/mamba-self-evaluation.md

A complete run, including 5,000 agreement trials per profile, sanitizer checks,
benchmarks, and the current rejection-analysis program, is available through
``--mode full``.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import importlib.util
import json
import os
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import textwrap
import time
from dataclasses import asdict, dataclass, field
from datetime import datetime, timezone
from pathlib import Path
from typing import Callable, Iterable, Sequence


ROOT = Path(__file__).resolve().parents[1]
API_ROOT = ROOT / "API_PKC"
REF_ROOT = API_ROOT / "Implementations" / "Reference_Implementation"
OPT_ROOT = API_ROOT / "Implementations" / "Optimized_Implementation"
MANIFEST = ROOT / "parameters.json"
EXPECTED_PARAMETERS_SHA256 = (
    "a34156a3dcc6c80328e5bf8814d7c1aa4764dd1ac8e257c455000e124194ffdd"
)
PROFILE_ORDER = [
    "MAMBA-NIKE-128",
    "MAMBA-NIKE-192",
    "MAMBA-NIKE-256",
    "MAMBA-NIKE-384",
    "MAMBA-NIKE-512",
]

BASE_CFLAGS = [
    "-std=c99",
    "-Wall",
    "-Wextra",
    "-Werror",
    "-O3",
    "-fomit-frame-pointer",
    "-DKAT_BUILD",
]
AVX2_CFLAGS = BASE_CFLAGS + ["-mavx2", "-march=native"]


class EvaluationFailure(RuntimeError):
    """Raised when a self-evaluation assertion fails."""


class EvaluationSkip(RuntimeError):
    """Raised when a test cannot run because an optional capability is absent."""


@dataclass(frozen=True)
class Profile:
    """Derived profile data loaded from the immutable parameter manifest."""

    name: str
    level: int
    n: int
    q: int
    log2q: int
    delta_pk: int
    delta_u: int
    t_pk: int
    t_u: int
    t_v: int
    eta_s: int
    eta_r: int
    ss_bytes: int

    @property
    def pk_bytes(self) -> int:
        return 32 + ((self.n * self.t_pk) + 7) // 8

    @property
    def helper_bytes(self) -> int:
        return self.n // 4

    @property
    def m1_bytes(self) -> int:
        return 32 + ((self.n * self.t_u) + 7) // 8 + self.helper_bytes

    @property
    def sk_api_bytes(self) -> int:
        return 2 * self.n + self.pk_bytes

    @property
    def kat_suffix(self) -> int:
        return self.pk_bytes + self.m1_bytes


@dataclass
class TestResult:
    """One self-evaluation result recorded in machine-readable form."""

    name: str
    status: str
    required: bool
    duration_seconds: float
    details: str = ""
    command: list[str] = field(default_factory=list)


@dataclass
class CommandResult:
    """Captured subprocess result used by tests and report generation."""

    command: list[str]
    returncode: int
    stdout: str
    stderr: str
    duration_seconds: float


class Runner:
    """Coordinates tests, temporary build products, and final reporting."""

    @staticmethod
    def _resolve_compiler(value: str) -> str:
        """Resolve the requested compiler to an absolute executable path."""

        path = shutil.which(value)
        if not path:
            raise EvaluationFailure(f"C compiler not found: {value}")
        return str(Path(path).resolve())

    def __init__(self, args: argparse.Namespace) -> None:
        self.args = args
        self.results: list[TestResult] = []
        self.metadata: dict[str, object] = {}
        self.evidence: dict[str, object] = {}
        self.temp = tempfile.TemporaryDirectory(prefix="mamba-self-evaluation-")
        self.temp_root = Path(self.temp.name)
        self.cc = self._resolve_compiler(args.cc)
        self.profiles = load_profiles(args.profiles)
        self.avx2_available = detect_avx2(self.cc)
        self.sanitizers_available = detect_sanitizers(self.cc, self.temp_root)
        self.protected_before = snapshot_protected_files(self.profiles)

    def close(self) -> None:
        self.temp.cleanup()

    def run_command(
        self,
        command: Sequence[str],
        *,
        cwd: Path | None = None,
        timeout: int | None = None,
        env: dict[str, str] | None = None,
        check: bool = True,
    ) -> CommandResult:
        """Run one command with deterministic capture and useful diagnostics."""

        cmd = [str(x) for x in command]
        if self.args.verbose:
            location = f" (cwd={cwd})" if cwd else ""
            print(f"+ {' '.join(cmd)}{location}", flush=True)

        merged_env = os.environ.copy()
        merged_env["PYTHONDONTWRITEBYTECODE"] = "1"
        if env:
            merged_env.update(env)

        started = time.monotonic()
        proc = subprocess.run(
            cmd,
            cwd=str(cwd) if cwd else None,
            env=merged_env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=timeout or self.args.timeout,
            check=False,
        )
        elapsed = time.monotonic() - started
        result = CommandResult(cmd, proc.returncode, proc.stdout, proc.stderr, elapsed)
        if check and proc.returncode != 0:
            raise EvaluationFailure(format_command_failure(result))
        return result

    def test(
        self,
        name: str,
        func: Callable[[], str | None],
        *,
        required: bool = True,
    ) -> None:
        """Run a test and convert exceptions into PASS/FAIL/SKIP records."""

        print(f"[{name}]", flush=True)
        started = time.monotonic()
        status = "PASS"
        details = ""
        try:
            returned = func()
            if returned:
                details = returned
        except EvaluationSkip as exc:
            status = "SKIP"
            details = str(exc)
        except Exception as exc:  # noqa: BLE001 - the report must preserve failures.
            status = "FAIL"
            details = str(exc)
        elapsed = time.monotonic() - started
        self.results.append(TestResult(name, status, required, elapsed, details))
        print(f"  {status}: {first_line(details) if details else 'completed'}", flush=True)
        if status == "FAIL" and self.args.fail_fast:
            raise EvaluationFailure(f"{name}: {details}")

    def required_failures(self) -> list[TestResult]:
        return [r for r in self.results if r.required and r.status == "FAIL"]

    def finalize_metadata(self) -> None:
        """Collect environment information after all tests have completed."""

        self.metadata = {
            "scheme": "MAMBA-NIKE",
            "scope": "x86 Reference and AVX2 Optimized implementations",
            "mode": self.args.mode,
            "timestamp_utc": datetime.now(timezone.utc).isoformat(),
            "repository_root": str(ROOT),
            "platform": platform.platform(),
            "machine": platform.machine(),
            "python": platform.python_version(),
            "compiler": compiler_identity(self.cc),
            "compiler_path": self.cc,
            "avx2_available": self.avx2_available,
            "sanitizers_available": self.sanitizers_available,
            "parameters_sha256": sha256_file(MANIFEST),
            "selected_profiles": [p.name for p in self.profiles],
            "agreement_trials": self.args.agreement_trials,
        }

    def write_reports(self) -> None:
        """Write optional JSON and Markdown reports without touching source files."""

        self.finalize_metadata()
        overall = "PASS" if not self.required_failures() else "FAIL"
        payload = {
            "overall_status": overall,
            "metadata": self.metadata,
            "profiles": [profile_to_dict(p) for p in self.profiles],
            "evidence": self.evidence,
            "results": [asdict(r) for r in self.results],
        }

        if self.args.json:
            path = Path(self.args.json).expanduser().resolve()
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n")
            print(f"JSON report: {path}")

        if self.args.markdown:
            path = Path(self.args.markdown).expanduser().resolve()
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(render_markdown(payload))
            print(f"Markdown report: {path}")


def first_line(text: str) -> str:
    """Return a compact one-line status message."""

    for line in text.splitlines():
        if line.strip():
            return line.strip()[:240]
    return ""


def format_command_failure(result: CommandResult) -> str:
    """Build a bounded error message for a failed subprocess."""

    stdout = result.stdout[-6000:]
    stderr = result.stderr[-6000:]
    return textwrap.dedent(
        f"""
        command failed with exit code {result.returncode}: {' '.join(result.command)}
        --- stdout (tail) ---
        {stdout}
        --- stderr (tail) ---
        {stderr}
        """
    ).strip()


def sha256_file(path: Path) -> str:
    """Return the SHA-256 digest of one file."""

    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _log2_power_of_two(value: int) -> int:
    if value <= 0 or value & (value - 1):
        raise EvaluationFailure(f"{value} is not a positive power of two")
    return value.bit_length() - 1


def load_profiles(selected: Sequence[str] | None) -> list[Profile]:
    """Load and validate profiles from parameters.json without rewriting it."""

    data = json.loads(MANIFEST.read_text())
    if data.get("scheme") != "MAMBA-NIKE":
        raise EvaluationFailure("parameters.json has an unexpected scheme name")

    q = int(data["q"])
    log2q = int(data["log2q"])
    if q != 1 << log2q:
        raise EvaluationFailure("parameters.json q/log2q values are inconsistent")

    profiles: list[Profile] = []
    names: set[str] = set()
    for item in data["profiles"]:
        name = str(item["name"])
        if name not in PROFILE_ORDER:
            raise EvaluationFailure(f"unexpected profile in parameters.json: {name}")
        if name in names:
            raise EvaluationFailure(f"duplicate profile in parameters.json: {name}")
        names.add(name)
        delta_pk = int(item["delta_pk"])
        delta_u = int(item["delta_u"])
        profile = Profile(
            name=name,
            level=int(item["level"]),
            n=int(item["n"]),
            q=q,
            log2q=log2q,
            delta_pk=delta_pk,
            delta_u=delta_u,
            t_pk=log2q - _log2_power_of_two(delta_pk),
            t_u=log2q - _log2_power_of_two(delta_u),
            t_v=int(data["t_v"]),
            eta_s=int(item["eta_s"]),
            eta_r=int(item["eta_r"]),
            ss_bytes=int(item["ss_bytes"]),
        )
        if profile.eta_s != profile.eta_r:
            raise EvaluationFailure(f"{name}: eta_s and eta_r differ")
        profiles.append(profile)

    if names != set(PROFILE_ORDER):
        raise EvaluationFailure("parameters.json does not contain the exact submitted profile set")

    profiles.sort(key=lambda p: PROFILE_ORDER.index(p.name))
    if selected:
        unknown = sorted(set(selected) - set(PROFILE_ORDER))
        if unknown:
            raise EvaluationFailure(f"unknown profile selection: {', '.join(unknown)}")
        profiles = [p for p in profiles if p.name in set(selected)]
    return profiles


def profile_to_dict(profile: Profile) -> dict[str, int | str]:
    """Return profile values, including derived byte lengths, for reports."""

    data = asdict(profile)
    data.update(
        {
            "pk_bytes": profile.pk_bytes,
            "sk_api_bytes": profile.sk_api_bytes,
            "m1_bytes": profile.m1_bytes,
            "helper_bytes": profile.helper_bytes,
            "kat_suffix": profile.kat_suffix,
        }
    )
    return data


def _resolve_executable(name: str) -> str | None:
    path = shutil.which(name)
    return str(Path(path).resolve()) if path else None


def compiler_identity(compiler: str) -> str:
    """Return the first line of the compiler version string."""

    try:
        proc = subprocess.run(
            [compiler, "--version"],
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        return first_line(proc.stdout) or "unknown"
    except OSError:
        return "unavailable"


def detect_avx2(compiler: str) -> bool:
    """Require both compiler support and AVX2 support from the running CPU."""

    try:
        compile_probe = subprocess.run(
            [compiler, "-mavx2", "-E", "-x", "c", "-"],
            input="int x;\n",
            text=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )
        if compile_probe.returncode != 0:
            return False
    except OSError:
        return False

    machine = platform.machine().lower()
    if machine not in {"x86_64", "amd64", "i386", "i686"}:
        return False

    cpuinfo = Path("/proc/cpuinfo")
    if cpuinfo.exists():
        return "avx2" in cpuinfo.read_text(errors="ignore").lower().split()

    if sys.platform == "darwin":
        try:
            proc = subprocess.run(
                ["sysctl", "-n", "machdep.cpu.leaf7_features"],
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.DEVNULL,
                check=False,
            )
            return "AVX2" in proc.stdout.upper()
        except OSError:
            return False
    return False


def detect_sanitizers(compiler: str, temp_root: Path) -> bool:
    """Compile and run a tiny ASan/UBSan program to test toolchain support."""

    source = temp_root / "sanitizer_probe.c"
    binary = temp_root / "sanitizer_probe"
    source.write_text("int main(void) { return 0; }\n")
    command = [
        compiler,
        "-std=c99",
        "-fsanitize=address,undefined",
        "-fno-omit-frame-pointer",
        str(source),
        "-o",
        str(binary),
    ]
    proc = subprocess.run(
        command,
        text=True,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        check=False,
    )
    if proc.returncode != 0:
        return False
    run = subprocess.run(
        [str(binary)],
        env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"},
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        check=False,
    )
    return run.returncode == 0


def snapshot_protected_files(profiles: Iterable[Profile]) -> dict[Path, str]:
    """Hash files that the evaluation process must never rewrite."""

    protected = [MANIFEST, API_ROOT / "KAT_MANIFEST.sha256"]
    for profile in profiles:
        protected.extend(
            [
                REF_ROOT / profile.name / "params.h",
                OPT_ROOT / profile.name / "params.h",
                API_ROOT / "Test_Vectors" / f"KAT_KEX_{profile.name}.txt",
                API_ROOT
                / "KAT"
                / profile.name
                / f"PQCkexKAT_{profile.kat_suffix}.req",
                API_ROOT
                / "KAT"
                / profile.name
                / f"PQCkexKAT_{profile.kat_suffix}.rsp",
            ]
        )
    missing = [str(path) for path in protected if not path.is_file()]
    if missing:
        raise EvaluationFailure("missing protected files:\n" + "\n".join(missing))
    return {path: sha256_file(path) for path in protected}


def parse_numeric_macros(path: Path) -> dict[str, int]:
    """Parse simple integer #define values from a profile header."""

    macros: dict[str, int] = {}
    pattern = re.compile(r"^\s*#define\s+([A-Za-z_][A-Za-z0-9_]*)\s+([0-9]+)[uUlL]*\s*$")
    for line in path.read_text().splitlines():
        match = pattern.match(line)
        if match:
            macros[match.group(1)] = int(match.group(2))
    return macros


def expected_header_macros(profile: Profile) -> dict[str, int]:
    return {
        "NIKE_LEVEL": profile.level,
        "NIKE_SECURITY_BITS": profile.level,
        "PARAM_N": profile.n,
        "PARAM_K": profile.eta_s,
        "PARAM_Q": profile.q,
        "LOG2Q": profile.log2q,
        "PARAM_T_PK": profile.t_pk,
        "PARAM_T_U": profile.t_u,
        "PARAM_T_V": profile.t_v,
        "PARAM_H_PK": profile.log2q - profile.t_pk,
        "PARAM_H_U": profile.log2q - profile.t_u,
        "PARAM_H_V": profile.log2q - profile.t_v,
        "NIKE_SSBYTES": profile.ss_bytes,
    }


def check_dependencies(runner: Runner) -> str:
    """Check only dependencies used by the selected mode."""

    required = {
        "C compiler": runner.cc,
        "GNU Make": _resolve_executable("make"),
        "Bash": _resolve_executable("bash"),
    }
    missing = [name for name, path in required.items() if not path]
    if sys.version_info < (3, 9):
        missing.append("Python 3.9 or newer")
    if missing:
        raise EvaluationFailure("missing required dependencies: " + ", ".join(missing))

    numpy_state = "installed" if importlib.util.find_spec("numpy") else "not installed"
    return (
        f"compiler={compiler_identity(runner.cc)}; make={required['GNU Make']}; "
        f"bash={required['Bash']}; AVX2={runner.avx2_available}; "
        f"ASan/UBSan={runner.sanitizers_available}; NumPy={numpy_state}"
    )


def check_package_layout(runner: Runner) -> str:
    """Validate the x86-only submission layout without creating directories."""

    required_dirs = [REF_ROOT, OPT_ROOT, API_ROOT / "KAT", API_ROOT / "Test_Vectors"]
    for path in required_dirs:
        if not path.is_dir():
            raise EvaluationFailure(f"required directory is missing: {path.relative_to(ROOT)}")

    forbidden = [
        ROOT / "m4",
        API_ROOT / "Implementations" / "Additional_Implementation",
    ]
    present = [str(path.relative_to(ROOT)) for path in forbidden if path.exists()]
    if present:
        raise EvaluationFailure("abandoned implementation paths are present: " + ", ".join(present))

    expected = set(PROFILE_ORDER)
    ref_profiles = {p.name for p in REF_ROOT.iterdir() if p.is_dir() and p.name.startswith("MAMBA-NIKE-")}
    opt_profiles = {p.name for p in OPT_ROOT.iterdir() if p.is_dir() and p.name.startswith("MAMBA-NIKE-")}
    if ref_profiles != expected:
        raise EvaluationFailure(f"Reference profile set mismatch: {sorted(ref_profiles)}")
    if opt_profiles != expected:
        raise EvaluationFailure(f"Optimized profile set mismatch: {sorted(opt_profiles)}")

    return "only the Reference and Optimized x86 tracks are present with all five profiles"


def check_parameter_alignment(runner: Runner) -> str:
    """Verify manifest hash and profile headers against derived values."""

    actual_sha = sha256_file(MANIFEST)
    if actual_sha != EXPECTED_PARAMETERS_SHA256:
        raise EvaluationFailure(
            "parameters.json SHA-256 mismatch: "
            f"expected {EXPECTED_PARAMETERS_SHA256}, got {actual_sha}"
        )

    for profile in runner.profiles:
        ref_header = REF_ROOT / profile.name / "params.h"
        opt_header = OPT_ROOT / profile.name / "params.h"
        if ref_header.read_bytes() != opt_header.read_bytes():
            raise EvaluationFailure(f"{profile.name}: Reference and Optimized params.h differ")

        macros = parse_numeric_macros(ref_header)
        for name, expected in expected_header_macros(profile).items():
            actual = macros.get(name)
            if actual != expected:
                raise EvaluationFailure(
                    f"{profile.name}: {name} expected {expected}, got {actual}"
                )

    return f"parameters.json and {2 * len(runner.profiles)} submitted headers are aligned"


def scan_reference_boundary(runner: Runner) -> str:
    """Enforce the requirement that Reference remains Toom-Cook-only."""

    banned_patterns = {
        "AVX2 intrinsic header": "immintrin.h",
        "AVX2 vector type": "__m256",
        "AVX2 intrinsic": "_mm256_",
        "AVX2 compile guard": "__AVX2__",
        "AVX2 compiler flag": "-mavx2",
        "native compiler flag": "-march=native",
        "small-CBD optimized symbol": "poly_mul_small",
    }

    for profile in runner.profiles:
        directory = REF_ROOT / profile.name
        files = [
            path
            for path in directory.iterdir()
            if path.is_file() and (path.suffix in {".c", ".h", ".S", ".s"} or path.name == "Makefile")
        ]
        for path in files:
            text = path.read_text(errors="ignore")
            for description, token in banned_patterns.items():
                if token in text:
                    raise EvaluationFailure(
                        f"{profile.name}: {description} found in {path.name}: {token}"
                    )

        poly_text = (directory / "poly.c").read_text(errors="ignore")
        if "poly_convolution" not in poly_text or "toom4_mul" not in poly_text:
            raise EvaluationFailure(
                f"{profile.name}: Reference poly.c does not expose the Toom-Cook convolution path"
            )
        if (directory / "chacha.S").exists():
            raise EvaluationFailure(f"{profile.name}: Reference unexpectedly contains chacha.S")

    return "Reference is portable coefficient-domain C and uses Toom-Cook-4 only"


def scan_optimized_boundary(runner: Runner) -> str:
    """Check that AVX2 code is confined to the Optimized track with a fallback."""

    for profile in runner.profiles:
        directory = OPT_ROOT / profile.name
        poly_text = (directory / "poly.c").read_text(errors="ignore")
        make_text = (directory / "Makefile").read_text(errors="ignore")
        required_poly_tokens = [
            "poly_mul_small",
            "__AVX2__",
            "_mm256_mullo_epi16",
            "toom4_mul",
        ]
        for token in required_poly_tokens:
            if token not in poly_text:
                raise EvaluationFailure(f"{profile.name}: optimized poly.c lacks {token}")
        if "-mavx2" not in make_text or "AVX2" not in make_text:
            raise EvaluationFailure(f"{profile.name}: optimized Makefile lacks AVX2 mode")
        if not (directory / "chacha.S").is_file():
            raise EvaluationFailure(f"{profile.name}: optimized chacha.S is missing")

    return "Optimized profiles contain AVX2 small-CBD multiplication and portable Toom fallback"


def verify_kat_manifest(_: Runner) -> str:
    """Verify all canonical KAT files against KAT_MANIFEST.sha256."""

    manifest_path = API_ROOT / "KAT_MANIFEST.sha256"
    checked = 0
    for raw_line in manifest_path.read_text().splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split(maxsplit=2)
        if len(parts) != 3:
            raise EvaluationFailure(f"invalid KAT manifest line: {raw_line}")
        expected_hash, expected_size_text, relative = parts
        path = API_ROOT / relative
        if not path.is_file():
            raise EvaluationFailure(f"KAT manifest file is missing: {relative}")
        expected_size = int(expected_size_text)
        actual_size = path.stat().st_size
        if actual_size != expected_size:
            raise EvaluationFailure(
                f"KAT size mismatch for {relative}: expected {expected_size}, got {actual_size}"
            )
        actual_hash = sha256_file(path)
        if actual_hash != expected_hash:
            raise EvaluationFailure(
                f"KAT hash mismatch for {relative}: expected {expected_hash}, got {actual_hash}"
            )
        checked += 1
    if checked != 15:
        raise EvaluationFailure(f"expected 15 canonical KAT records, verified {checked}")
    return "15 canonical KAT files match their recorded sizes and SHA-256 digests"


def verify_security_records(runner: Runner) -> str:
    """Check the code-aligned estimator record without rerunning Sage estimators."""

    path = ROOT / "estimator" / "results" / "NIKE_security_summary_20260630.md"
    if not path.is_file():
        raise EvaluationFailure("security-estimator summary is missing")
    text = path.read_text()
    expected = {
        "MAMBA-NIKE-128": (272.1, 251.1),
        "MAMBA-NIKE-192": (279.3, 258.4),
        "MAMBA-NIKE-256": (294.3, 273.9),
        "MAMBA-NIKE-384": (538.6, 512.5),
        "MAMBA-NIKE-512": (590.2, 512.5),
    }
    for profile in runner.profiles:
        classical, quantum = expected[profile.name]
        pattern = re.compile(
            rf"\|\s*{re.escape(profile.name)}\s*\|[^\n]*\|\s*{classical:.1f}\s*\|\s*{quantum:.1f}\s*\|"
        )
        if not pattern.search(text):
            raise EvaluationFailure(
                f"{profile.name}: expected MATZOV C/Q record {classical:.1f}/{quantum:.1f} not found"
            )
    return "recorded MATZOV classical/quantum values match the code-aligned report"


def clean_profile(runner: Runner, directory: Path, avx2: int | None = None) -> None:
    command = ["make", "clean"]
    if avx2 is not None:
        command.append(f"AVX2={avx2}")
    runner.run_command(command, cwd=directory, check=False)


def make_profile(runner: Runner, directory: Path, cflags: Sequence[str], avx2: int | None) -> None:
    """Perform a clean KAT build in one profile directory."""

    clean_profile(runner, directory, avx2)
    command = [
        "make",
        f"CC={runner.cc}",
        f"CFLAGS={' '.join(cflags)}",
    ]
    if avx2 is not None:
        command.append(f"AVX2={avx2}")
    runner.run_command(command, cwd=directory)
    if not (directory / "KAT_KEX").is_file():
        raise EvaluationFailure(f"build did not produce {directory / 'KAT_KEX'}")


def build_matrix(runner: Runner) -> str:
    """Clean-build each selected profile in every supported submitted mode."""

    modes: list[tuple[str, Path, Sequence[str], int | None]] = [
        ("Reference", REF_ROOT, BASE_CFLAGS, None),
        ("Optimized portable", OPT_ROOT, BASE_CFLAGS, 0),
    ]
    if runner.avx2_available:
        modes.append(("Optimized AVX2", OPT_ROOT, AVX2_CFLAGS, 1))

    built = 0
    for label, root, flags, avx2 in modes:
        for profile in runner.profiles:
            directory = root / profile.name
            print(f"    build {label}: {profile.name}", flush=True)
            try:
                make_profile(runner, directory, flags, avx2)
                built += 1
            finally:
                clean_profile(runner, directory, avx2)
    return f"{built} clean profile builds completed across {len(modes)} implementation modes"


def run_kat_and_compare(
    runner: Runner,
    profile: Profile,
    *,
    root: Path,
    label: str,
    cflags: Sequence[str],
    avx2: int | None,
) -> None:
    """Build one KAT binary, run it, and compare output byte-for-byte."""

    directory = root / profile.name
    canonical = API_ROOT / "Test_Vectors" / f"KAT_KEX_{profile.name}.txt"
    try:
        make_profile(runner, directory, cflags, avx2)
        output_dir = directory / "output"
        if output_dir.exists():
            shutil.rmtree(output_dir)
        runner.run_command([str(directory / "KAT_KEX")], cwd=directory)
        produced = output_dir / f"KAT_KEX_{profile.name}.txt"
        if not produced.is_file():
            raise EvaluationFailure(f"{label} did not produce {produced}")
        if produced.read_bytes() != canonical.read_bytes():
            raise EvaluationFailure(f"{label} KAT differs from canonical Reference output")
    finally:
        clean_profile(runner, directory, avx2)


def kat_conformance(runner: Runner) -> str:
    """Check Reference, portable Optimized, and AVX2 KAT compatibility."""

    modes: list[tuple[str, Path, Sequence[str], int | None]] = [
        ("Reference", REF_ROOT, BASE_CFLAGS, None),
        ("Optimized portable", OPT_ROOT, BASE_CFLAGS, 0),
    ]
    if runner.avx2_available:
        modes.append(("Optimized AVX2", OPT_ROOT, AVX2_CFLAGS, 1))

    compared = 0
    for label, root, flags, avx2 in modes:
        for profile in runner.profiles:
            print(f"    KAT {label}: {profile.name}", flush=True)
            run_kat_and_compare(
                runner,
                profile,
                root=root,
                label=f"{label} {profile.name}",
                cflags=flags,
                avx2=avx2,
            )
            compared += 1
    return f"{compared} fresh KAT runs matched the canonical Reference vectors byte-for-byte"


def source_list(directory: Path, *, include_chacha_asm: bool) -> list[str]:
    """Return sources needed for NGCC API conformance and agreement programs."""

    sources = [
        "KEX_AlgorithmInstance.c",
        "crypto_stream_chacha20.c",
        "poly.c",
        "toom.c",
        "error_correction.c",
        "nike.c",
        "reduce.c",
        "fips202.c",
        "drng.c",
    ]
    if include_chacha_asm:
        sources.append("chacha.S")
    return [str(directory / name) for name in sources]


def compile_and_run_api_check(
    runner: Runner,
    profile: Profile,
    *,
    root: Path,
    label: str,
    flags: Sequence[str],
    include_chacha_asm: bool,
    sanitizer: bool = False,
) -> None:
    directory = root / profile.name
    binary = runner.temp_root / f"api-check-{label}-{profile.name}"
    command = [
        runner.cc,
        *flags,
        f"-I{directory}",
        *source_list(directory, include_chacha_asm=include_chacha_asm),
        str(API_ROOT / "Internal_Tests" / "NGCC_API" / "api_check.c"),
        "-lm",
        "-o",
        str(binary),
    ]
    runner.run_command(command)
    environment = {"ASAN_OPTIONS": "detect_leaks=0"} if sanitizer else None
    runner.run_command([str(binary)], env=environment)


def api_conformance(runner: Runner) -> str:
    """Run normal and negative NGCC API checks for both submitted tracks."""

    modes: list[tuple[str, Path, Sequence[str], bool]] = [
        ("reference", REF_ROOT, BASE_CFLAGS, False),
        ("optimized-portable", OPT_ROOT, BASE_CFLAGS, False),
    ]
    if runner.avx2_available:
        modes.append(("optimized-avx2", OPT_ROOT, AVX2_CFLAGS, True))

    count = 0
    for label, root, flags, include_asm in modes:
        for profile in runner.profiles:
            print(f"    API {label}: {profile.name}", flush=True)
            compile_and_run_api_check(
                runner,
                profile,
                root=root,
                label=label,
                flags=flags,
                include_chacha_asm=include_asm,
            )
            count += 1
    return f"{count} NGCC API normal/negative test executions passed"


def sampler_checks(runner: Runner) -> str:
    """Compile and run the submitted centered-binomial sampler test."""

    count = 0
    source = ROOT / "tests" / "sampler_check.c"
    for profile in runner.profiles:
        directory = REF_ROOT / profile.name
        binary = runner.temp_root / f"sampler-{profile.name}"
        command = [
            runner.cc,
            "-std=c99",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-O2",
            f"-I{directory}",
            str(source),
            str(directory / "poly.c"),
            str(directory / "toom.c"),
            str(directory / "fips202.c"),
            str(directory / "crypto_stream_chacha20.c"),
            "-lm",
            "-o",
            str(binary),
        ]
        runner.run_command(command)
        runner.run_command([str(binary)])
        count += 1
    return f"centered-binomial sampler tests passed for {count} profiles"


POLY_DIFFERENTIAL_SOURCE = r'''
/*
 * Differential test for the AVX2 small-CBD polynomial multiplier.
 *
 * The generic Toom-Cook convolution is used as the independent oracle.  The
 * test covers a negacyclic basis wrap, random operands, and both supported
 * input-alias cases.  Loop counts and random seeds are fixed for reproducible
 * self-evaluation.
 */
#include "poly.h"
#include "params.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint32_t state = 0x7f4a7c15u;

static uint32_t next_u32(void)
{
    uint32_t x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state = x;
    return x;
}

static uint16_t encode_small(int value)
{
    return (uint16_t)((value + PARAM_Q) & (PARAM_Q - 1));
}

static int equal_poly(const poly *expected, const poly *actual,
                      int trial, const char *kind)
{
    int i;
    for (i = 0; i < PARAM_N; ++i) {
        if (expected->coeffs[i] != actual->coeffs[i]) {
            fprintf(stderr,
                    "%s mismatch: trial=%d index=%d expected=%u actual=%u\n",
                    kind, trial, i, (unsigned)expected->coeffs[i],
                    (unsigned)actual->coeffs[i]);
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    poly a, small, expected, actual, alias;
    int trial;
    int i;

    memset(&a, 0, sizeof(a));
    memset(&small, 0, sizeof(small));
    a.coeffs[0] = 1;
    small.coeffs[PARAM_N - 1] = encode_small(1);
    poly_convolution(&expected, &a, &small);
    poly_mul_small(&actual, &a, &small);
    if (!equal_poly(&expected, &actual, -1, "basis-wrap"))
        return 1;

    for (trial = 0; trial < 24; ++trial) {
        for (i = 0; i < PARAM_N; ++i) {
            int small_value =
                (int)(next_u32() % (2u * PARAM_K + 1u)) - PARAM_K;
            a.coeffs[i] = (uint16_t)(next_u32() & (PARAM_Q - 1));
            small.coeffs[i] = encode_small(small_value);
        }

        poly_convolution(&expected, &a, &small);
        poly_mul_small(&actual, &a, &small);
        if (!equal_poly(&expected, &actual, trial, "normal"))
            return 1;

        alias = a;
        poly_mul_small(&alias, &alias, &small);
        if (!equal_poly(&expected, &alias, trial, "alias-a"))
            return 1;

        alias = small;
        poly_mul_small(&alias, &a, &alias);
        if (!equal_poly(&expected, &alias, trial, "alias-small"))
            return 1;
    }

    printf("poly_mul_small differential PASS: n=%d eta=%d trials=24\n",
           PARAM_N, PARAM_K);
    return 0;
}
'''


def polynomial_differential(runner: Runner) -> str:
    """Compare AVX2 small-CBD multiplication against Toom-Cook."""

    if not runner.avx2_available:
        raise EvaluationSkip("AVX2 is not available on this compiler/CPU")

    source = runner.temp_root / "poly_differential.c"
    source.write_text(POLY_DIFFERENTIAL_SOURCE)
    count = 0
    for profile in runner.profiles:
        directory = OPT_ROOT / profile.name
        binary = runner.temp_root / f"poly-differential-{profile.name}"
        command = [
            runner.cc,
            *AVX2_CFLAGS,
            f"-I{directory}",
            str(source),
            str(directory / "poly.c"),
            str(directory / "toom.c"),
            str(directory / "fips202.c"),
            str(directory / "crypto_stream_chacha20.c"),
            str(directory / "chacha.S"),
            "-lm",
            "-o",
            str(binary),
        ]
        runner.run_command(command)
        runner.run_command([str(binary)])
        count += 1
    return f"AVX2/Toom-Cook differential, wrap, and alias checks passed for {count} profiles"


def sanitizer_checks(runner: Runner) -> str:
    """Run ASan/UBSan on representative low/high-dimension API profiles."""

    if not runner.sanitizers_available:
        raise EvaluationSkip("the selected compiler cannot run ASan/UBSan")

    profiles = [
        p for p in runner.profiles if p.name in {"MAMBA-NIKE-256", "MAMBA-NIKE-512"}
    ]
    if not profiles:
        raise EvaluationSkip("selected profiles do not include MAMBA-NIKE-256 or -512")

    flags = [
        "-std=c99",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-O1",
        "-g",
        "-DKAT_BUILD",
        "-fsanitize=address,undefined",
        "-fno-omit-frame-pointer",
    ]
    count = 0
    for profile in profiles:
        compile_and_run_api_check(
            runner,
            profile,
            root=REF_ROOT,
            label="reference-sanitized",
            flags=flags,
            include_chacha_asm=False,
            sanitizer=True,
        )
        count += 1
        if runner.avx2_available:
            compile_and_run_api_check(
                runner,
                profile,
                root=OPT_ROOT,
                label="optimized-avx2-sanitized",
                flags=flags + ["-mavx2", "-march=native"],
                include_chacha_asm=True,
                sanitizer=True,
            )
            count += 1
    return f"ASan/UBSan API tests passed in {count} representative builds"


AGREEMENT_SOURCE = r'''
/*
 * Deterministic NGCC API agreement test for MAMBA-NIKE.
 *
 * The test uses the submitted deterministic KAT DRNG so that every run is
 * reproducible.  Each iteration creates fresh key pairs, executes the single
 * online pass, derives both shared secrets, validates every reported length,
 * and compares the shared secrets byte-for-byte.
 */
#include "KEX_AlgorithmInstance.h"
#include "drng.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DRNG_ctx drng_algorithm;

static unsigned char *allocate_bytes(unsigned long long length)
{
    if (length == 0)
        length = 1;
    return (unsigned char *)calloc((size_t)length, 1);
}

static void seed_drng(void)
{
    unsigned char seed[64];
    unsigned int i;
    for (i = 0; i < sizeof(seed); ++i)
        seed[i] = (unsigned char)(0x5aU ^ (unsigned char)(29U * i));
    init_random_number(&drng_algorithm, seed, sizeof(seed));
}

int main(int argc, char **argv)
{
    unsigned long long trials = 5000;
    unsigned long long pk_len = kex_get_pk_len_bytes();
    unsigned long long sk_len = kex_get_sk_len_bytes();
    unsigned long long ss_len = kex_get_ss_len_bytes();
    unsigned long long m1_len_expected = kex_get_total_msg_len_bytes();
    unsigned long long sta_capacity = kex_get_sta_len_bytes();
    unsigned long long stb_capacity = kex_get_stb_len_bytes();
    unsigned long long agreements = 0;
    unsigned long long failures = 0;
    unsigned long long trial;

    unsigned char *pka = allocate_bytes(pk_len);
    unsigned char *ska = allocate_bytes(sk_len);
    unsigned char *pkb = allocate_bytes(pk_len);
    unsigned char *skb = allocate_bytes(sk_len);
    unsigned char *sta = allocate_bytes(sta_capacity);
    unsigned char *stb = allocate_bytes(stb_capacity);
    unsigned char *m1 = allocate_bytes(m1_len_expected);
    unsigned char *ssa = allocate_bytes(ss_len);
    unsigned char *ssb = allocate_bytes(ss_len);

    if (argc == 2)
        trials = strtoull(argv[1], NULL, 10);
    if (trials == 0) {
        fprintf(stderr, "trial count must be positive\n");
        return 2;
    }
    if (!(pka && ska && pkb && skb && sta && stb && m1 && ssa && ssb)) {
        fprintf(stderr, "allocation failure\n");
        return 2;
    }

    seed_drng();
    for (trial = 0; trial < trials; ++trial) {
        unsigned long long pka_out = 0, ska_out = 0, sta_out = 0;
        unsigned long long pkb_out = 0, skb_out = 0, stb_out = 0;
        unsigned long long m1_out = 0, ssa_out = 0, ssb_out = 0;
        int rc;

        rc = kex_init_a(pka, &pka_out, ska, &ska_out, sta, &sta_out);
        if (rc != 0 || pka_out != pk_len || ska_out != sk_len || sta_out != 0)
            goto protocol_failure;

        rc = kex_init_b(pkb, &pkb_out, skb, &skb_out, stb, &stb_out);
        if (rc != 0 || pkb_out != pk_len || skb_out != sk_len || stb_out != 0)
            goto protocol_failure;

        rc = kex_generate_pass1_msg_a(ska, sk_len, pkb, pk_len,
                                      sta, &sta_out, m1, &m1_out);
        if (rc != 1 || sta_out != ss_len || m1_out != m1_len_expected)
            goto protocol_failure;

        rc = kex_derive_ss_b(skb, sk_len, pka, pk_len, m1, m1_out,
                             stb, stb_out, ssb, &ssb_out);
        if (rc != 0 || ssb_out != ss_len)
            goto protocol_failure;

        rc = kex_derive_ss_a(ska, sk_len, pkb, pk_len, NULL, 0,
                             sta, sta_out, ssa, &ssa_out);
        if (rc != 0 || ssa_out != ss_len)
            goto protocol_failure;

        if (memcmp(ssa, ssb, (size_t)ss_len) != 0)
            goto protocol_failure;

        ++agreements;
        continue;

protocol_failure:
        ++failures;
    }

    printf("trials=%llu agreements=%llu failures=%llu rate=%.8f%%\n",
           trials, agreements, failures,
           100.0 * (double)agreements / (double)trials);

    free(pka); free(ska); free(pkb); free(skb); free(sta);
    free(stb); free(m1); free(ssa); free(ssb);
    return failures == 0 ? 0 : 1;
}
'''


def choose_agreement_mode(runner: Runner) -> tuple[str, Path, Sequence[str], bool]:
    """Resolve the requested agreement implementation into build parameters."""

    choice = runner.args.agreement_track
    if choice == "auto":
        choice = "optimized-avx2" if runner.avx2_available else "reference"
    if choice == "optimized-avx2":
        if not runner.avx2_available:
            raise EvaluationSkip("optimized-avx2 agreement testing requested but AVX2 is unavailable")
        return choice, OPT_ROOT, AVX2_CFLAGS, True
    if choice == "optimized-portable":
        return choice, OPT_ROOT, BASE_CFLAGS, False
    if choice == "reference":
        return choice, REF_ROOT, BASE_CFLAGS, False
    raise EvaluationFailure(f"unsupported agreement track: {choice}")


def agreement_tests(runner: Runner) -> str:
    """Execute deterministic complete exchanges for every selected profile."""

    label, root, flags, include_asm = choose_agreement_mode(runner)
    source = runner.temp_root / "agreement_test.c"
    source.write_text(AGREEMENT_SOURCE)
    total = 0
    for profile in runner.profiles:
        directory = root / profile.name
        binary = runner.temp_root / f"agreement-{label}-{profile.name}"
        command = [
            runner.cc,
            *flags,
            f"-I{directory}",
            *source_list(directory, include_chacha_asm=include_asm),
            str(source),
            "-lm",
            "-o",
            str(binary),
        ]
        runner.run_command(command)
        result = runner.run_command(
            [str(binary), str(runner.args.agreement_trials)],
            timeout=max(runner.args.timeout, 600),
        )
        runner.evidence.setdefault("agreement", []).append(
            {
                "profile": profile.name,
                "track": label,
                "trials": runner.args.agreement_trials,
                "output": first_line(result.stdout),
            }
        )
        total += runner.args.agreement_trials
    return f"{total} deterministic complete exchanges passed on {label} with zero failures"


def compile_benchmark(
    runner: Runner,
    profile: Profile,
    *,
    optimized: bool,
) -> Path:
    """Compile one benchmark into the temporary directory."""

    directory = (OPT_ROOT if optimized else REF_ROOT) / profile.name
    name = "avx2" if optimized else "reference"
    binary = runner.temp_root / f"benchmark-{name}-{profile.name}"
    harness = ROOT / "benchmarks" / ("bench_nike_avx2.c" if optimized else "bench_nike.c")
    flags = [
        "-std=c99",
        "-O3",
        "-Wall",
        "-Wextra",
        "-fomit-frame-pointer",
    ]
    if optimized:
        flags += ["-mavx2", "-march=native"]

    sources = [
        str(harness),
        str(directory / "nike.c"),
        str(directory / "poly.c"),
        str(directory / "toom.c"),
        str(directory / "error_correction.c"),
        str(directory / "reduce.c"),
        str(directory / "fips202.c"),
        str(directory / "crypto_stream_chacha20.c"),
    ]
    if optimized:
        sources.insert(1, str(ROOT / "benchmarks" / "randombytes_opt.c"))
        sources.append(str(directory / "chacha.S"))
    else:
        sources.insert(1, str(directory / "KEX_AlgorithmInstance.c"))

    command = [
        runner.cc,
        *flags,
        f'-DBENCH_PROFILE_NAME="{profile.name}"',
        f"-I{ROOT / 'benchmarks'}",
        f"-I{directory}",
        *sources,
        "-lm",
        "-o",
        str(binary),
    ]
    runner.run_command(command)
    return binary


def read_benchmark_csv(path: Path) -> dict[str, str]:
    with path.open(newline="") as handle:
        rows = list(csv.DictReader(handle))
    if len(rows) != 1:
        raise EvaluationFailure(f"expected one benchmark row in {path}, got {len(rows)}")
    return dict(rows[0])


def benchmark_tests(runner: Runner) -> str:
    """Run report-compatible 1,000-iteration Reference/AVX2 benchmarks."""

    if not runner.avx2_available:
        raise EvaluationSkip("paired benchmark requires an AVX2-capable compiler and CPU")

    rows: list[dict[str, str]] = []
    for profile in runner.profiles:
        work = runner.temp_root / f"benchmark-output-{profile.name}"
        work.mkdir(exist_ok=True)
        for optimized in (False, True):
            binary = compile_benchmark(runner, profile, optimized=optimized)
            runner.run_command([str(binary)], cwd=work, timeout=max(runner.args.timeout, 600))
            csv_name = "nike_avx2.csv" if optimized else "nike_ref.csv"
            row = read_benchmark_csv(work / csv_name)
            if int(row.get("fail", "-1")) != 0:
                raise EvaluationFailure(f"{profile.name} {csv_name}: benchmark recorded failures")
            rows.append(row)

    runner.evidence["benchmarks"] = rows
    return f"paired 1,000-iteration RDTSC benchmarks passed for {len(runner.profiles)} profiles"


def rejection_analysis(runner: Runner) -> str:
    """Run the current double-precision rejection tool when NumPy is installed."""

    if importlib.util.find_spec("numpy") is None:
        raise EvaluationSkip("NumPy is not installed; rejection analysis is optional")

    result = runner.run_command(
        [sys.executable, str(ROOT / "compute_rejection.py")],
        cwd=ROOT,
        timeout=max(runner.args.timeout, 900),
    )
    rows: list[str] = []
    for profile in runner.profiles:
        matching = [line for line in result.stdout.splitlines() if line.startswith(profile.name + ",")]
        if len(matching) != 1:
            raise EvaluationFailure(f"{profile.name}: rejection output row missing or duplicated")
        if "TBD(double_floor)" not in matching[0]:
            raise EvaluationFailure(
                f"{profile.name}: current tool no longer reports the documented double floor"
            )
        rows.append(matching[0])
    runner.evidence["rejection_analysis"] = rows
    return "all selected profiles report the documented TBD(double_floor) status"


def verify_protected_files(runner: Runner) -> str:
    """Prove that the self-evaluation did not rewrite parameters or canonical KATs."""

    changed: list[str] = []
    for path, before in runner.protected_before.items():
        if not path.is_file():
            changed.append(f"deleted: {path.relative_to(ROOT)}")
            continue
        after = sha256_file(path)
        if before != after:
            changed.append(f"changed: {path.relative_to(ROOT)}")
    if changed:
        raise EvaluationFailure("protected-file mutation detected:\n" + "\n".join(changed))
    return f"{len(runner.protected_before)} protected files remained byte-for-byte unchanged"


def render_markdown(payload: dict[str, object]) -> str:
    """Render a compact English report suitable for submission records."""

    metadata = payload["metadata"]
    results = payload["results"]
    profiles = payload["profiles"]
    lines = [
        "# MAMBA-NIKE x86 Self-Evaluation Results",
        "",
        f"**Overall status:** {payload['overall_status']}",
        "",
        "## Environment",
        "",
        f"- Timestamp (UTC): `{metadata['timestamp_utc']}`",
        f"- Mode: `{metadata['mode']}`",
        f"- Platform: `{metadata['platform']}`",
        f"- Compiler: `{metadata['compiler']}`",
        f"- Python: `{metadata['python']}`",
        f"- AVX2 available: `{metadata['avx2_available']}`",
        f"- ASan/UBSan available: `{metadata['sanitizers_available']}`",
        f"- parameters.json SHA-256: `{metadata['parameters_sha256']}`",
        "",
        "## Profiles",
        "",
        "| Profile | n | (eta_s, eta_r; t_pk, t_u, t_v) | pk | sk_API | M1 | ss |",
        "|---|---:|---|---:|---:|---:|---:|",
    ]
    for profile in profiles:
        lines.append(
            "| {name} | {n} | ({eta_s}, {eta_r}; {t_pk}, {t_u}, {t_v}) | "
            "{pk_bytes} | {sk_api_bytes} | {m1_bytes} | {ss_bytes} |".format(**profile)
        )

    lines.extend(
        [
            "",
            "## Test Results",
            "",
            "| Test | Status | Required | Time (s) | Details |",
            "|---|---|:---:|---:|---|",
        ]
    )
    for result in results:
        details = str(result["details"]).replace("|", "\\|").replace("\n", "<br>")
        lines.append(
            f"| {result['name']} | {result['status']} | "
            f"{'yes' if result['required'] else 'no'} | "
            f"{result['duration_seconds']:.3f} | {details} |"
        )
    benchmark_rows = payload.get("evidence", {}).get("benchmarks", [])
    if benchmark_rows:
        lines.extend(
            [
                "",
                "## Measured Performance",
                "",
                "| Profile | Implementation | KeyGen (kCy) | Pass1 (kCy) | Derive (kCy) | Failures |",
                "|---|---|---:|---:|---:|---:|",
            ]
        )
        for row in benchmark_rows:
            lines.append(
                f"| {row.get('preset', '')} | {row.get('impl', '')} | "
                f"{row.get('keygen_kCy', '')} | {row.get('sharedb_kCy', '')} | "
                f"{row.get('shareda_kCy', '')} | {row.get('fail', '')} |"
            )

    lines.extend(
        [
            "",
            "## Interpretation",
            "",
            "A PASS means every required test completed successfully within the selected mode. "
            "A SKIP is used only for optional host capabilities such as AVX2, sanitizers, or NumPy. "
            "The runner does not claim a finite rejection bound when the repository tool reports "
            "`TBD(double_floor)`.",
            "",
        ]
    )
    return "\n".join(lines)


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run code-aligned self-evaluation for MAMBA-NIKE x86 implementations."
    )
    parser.add_argument(
        "--mode",
        choices=("quick", "conformance", "full"),
        default="conformance",
        help=(
            "quick: static checks plus one-profile build/API smoke tests; "
            "conformance: all profiles, KAT/API/differential checks; "
            "full: conformance plus sanitizers, 5,000-trial agreement, benchmarks, and rejection analysis"
        ),
    )
    parser.add_argument(
        "--profile",
        dest="profiles",
        action="append",
        choices=PROFILE_ORDER,
        help="limit the run to one profile; repeat the option to select multiple profiles",
    )
    parser.add_argument(
        "--cc",
        default=os.environ.get("CC", "gcc"),
        help="C compiler executable (default: $CC or gcc)",
    )
    parser.add_argument(
        "--agreement-trials",
        type=int,
        default=5000,
        help="complete exchanges per selected profile in full mode (default: 5000)",
    )
    parser.add_argument(
        "--agreement-track",
        choices=("auto", "reference", "optimized-portable", "optimized-avx2"),
        default="auto",
        help="implementation used for full-mode agreement tests (default: auto)",
    )
    parser.add_argument("--json", help="optional JSON report path")
    parser.add_argument("--markdown", help="optional Markdown report path")
    parser.add_argument(
        "--timeout",
        type=int,
        default=300,
        help="default subprocess timeout in seconds (default: 300)",
    )
    parser.add_argument("--verbose", action="store_true", help="print every subprocess command")
    parser.add_argument("--fail-fast", action="store_true", help="stop after the first failure")
    args = parser.parse_args(argv)
    if args.agreement_trials <= 0:
        parser.error("--agreement-trials must be positive")
    return args


def run_evaluation(runner: Runner) -> None:
    """Schedule tests according to the selected evaluation mode."""

    runner.test("Dependencies", lambda: check_dependencies(runner))
    runner.test("x86-only package layout", lambda: check_package_layout(runner))
    runner.test("Frozen parameter alignment", lambda: check_parameter_alignment(runner))
    runner.test("Reference implementation boundary", lambda: scan_reference_boundary(runner))
    runner.test("Optimized implementation boundary", lambda: scan_optimized_boundary(runner))
    runner.test("Canonical KAT manifest integrity", lambda: verify_kat_manifest(runner))
    runner.test("Security-estimator record alignment", lambda: verify_security_records(runner))

    if runner.args.mode == "quick":
        original = runner.profiles
        runner.profiles = [original[0]]
        runner.test("Quick clean-build smoke test", lambda: build_matrix(runner))
        runner.test("Quick NGCC API smoke test", lambda: api_conformance(runner))
        runner.profiles = original
    else:
        runner.test("Clean build matrix", lambda: build_matrix(runner))
        runner.test("Centered-binomial sampler checks", lambda: sampler_checks(runner))
        runner.test("AVX2 polynomial differential checks", lambda: polynomial_differential(runner))
        runner.test("NGCC API conformance", lambda: api_conformance(runner))
        runner.test("Fresh KAT byte-for-byte conformance", lambda: kat_conformance(runner))

    if runner.args.mode == "full":
        runner.test("ASan/UBSan representative checks", lambda: sanitizer_checks(runner), required=False)
        runner.test("Deterministic agreement testing", lambda: agreement_tests(runner))
        runner.test("Paired x86 performance benchmark", lambda: benchmark_tests(runner), required=False)
        runner.test("Current rejection-analysis status", lambda: rejection_analysis(runner), required=False)

    # This test must run last.  It detects accidental writes performed by any
    # build, KAT, analysis, or reporting step above.
    runner.test("Protected-file immutability", lambda: verify_protected_files(runner))


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv or sys.argv[1:])
    runner: Runner | None = None
    try:
        runner = Runner(args)
        run_evaluation(runner)
        runner.write_reports()
        failures = runner.required_failures()
        print()
        if failures:
            print(f"SELF-EVALUATION: FAIL ({len(failures)} required test(s) failed)")
            for result in failures:
                print(f"- {result.name}: {first_line(result.details)}")
            return 1
        print("SELF-EVALUATION: PASS")
        return 0
    except EvaluationFailure as exc:
        print(f"SELF-EVALUATION SETUP ERROR: {exc}", file=sys.stderr)
        return 2
    finally:
        if runner is not None:
            runner.close()


if __name__ == "__main__":
    raise SystemExit(main())
