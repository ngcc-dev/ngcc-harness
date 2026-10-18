#!/usr/bin/env python3
"""Exercise SQIsign verification after a gluing failure."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import shlex
import shutil
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCHEME = ROOT / "SQIsign"
REFERENCE = SCHEME / "submission/SQIsign/Reference_Implementation"
GLUING = REFERENCE / "src/hd/ref/lvlx/gluing.c"
THETA = REFERENCE / "src/hd/ref/lvlx/theta_isogenies.c"
MESSAGE = b"SQIsign unchecked gluing failure witness"
PINNED = {
    "gluing.c": "4e33062d338ed81073aaeb110326e45becc81f1499fd116480639c51cc99b2a5",
    "theta_isogenies.c": "6cfd8e5e397cc611e62786183038a2214ac4bb9bd125f97623c517ef6248cf9f",
}


class Meta(ctypes.Structure):
    _fields_ = [
        ("abi", ctypes.c_uint32), ("scheme", ctypes.c_char_p),
        ("parameter", ctypes.c_char_p), ("implementation", ctypes.c_char_p),
        ("pk", ctypes.c_uint64), ("sk", ctypes.c_uint64),
        ("sig", ctypes.c_uint64),
    ]


def buffer(data: bytes | int):
    if isinstance(data, int):
        return (ctypes.c_ubyte * data)()
    return (ctypes.c_ubyte * len(data)).from_buffer_copy(data)


def instrument_sources(directory: Path) -> tuple[Path, Path]:
    gluing = GLUING.read_text(encoding="utf-8")
    gluing = gluing.replace(
        '#include "gluing.h"',
        '#include "gluing.h"\nint sqisign_witness_gluing_failure;',
        1,
    )
    replacements = (
        (
            'debug_print("gluing failed as kernel does not have correct order");\n            return 0;',
            'debug_print("gluing failed as kernel does not have correct order");\n'
            '            sqisign_witness_gluing_failure = 1;\n            return 0;',
        ),
        (
            'debug_print("gluing failed TT1.t or TT2.t is not zero");\n        return 0;',
            'debug_print("gluing failed TT1.t or TT2.t is not zero");\n'
            '        sqisign_witness_gluing_failure = 2;\n        return 0;',
        ),
        (
            'debug_print("unexpected zeroes in gluing theta point.");\n        return 0; // invalid input',
            'debug_print("unexpected zeroes in gluing theta point.");\n'
            '        sqisign_witness_gluing_failure = 3;\n        return 0; // invalid input',
        ),
    )
    for old, new in replacements:
        if gluing.count(old) != 1:
            raise RuntimeError(f"could not instrument gluing failure: {old}")
        gluing = gluing.replace(old, new, 1)
    gluing_path = directory / "gluing.c"
    gluing_path.write_text(gluing, encoding="utf-8")

    theta = THETA.read_text(encoding="utf-8")
    declaration = "    theta_gluing_t first_step;"
    if theta.count(declaration) != 1:
        raise RuntimeError("could not locate first_step declaration")
    theta = theta.replace(
        declaration,
        declaration + "\n    memset(&first_step, 0xA5, sizeof(first_step));",
        1,
    )
    theta = theta.replace(
        '#include "gluing.h"',
        '#include "gluing.h"\n'
        'extern int sqisign_witness_gluing_failure;\n'
        'int sqisign_witness_continued;\n'
        'unsigned char sqisign_witness_first_read;\n'
        'void sqisign_witness_reset(void) {\n'
        '    sqisign_witness_gluing_failure = 0;\n'
        '    sqisign_witness_continued = 0;\n'
        '    sqisign_witness_first_read = 0;\n'
        '}\n'
        'int sqisign_witness_status(unsigned char *value) {\n'
        '    *value = sqisign_witness_first_read;\n'
        '    return 10 * sqisign_witness_gluing_failure + sqisign_witness_continued;\n'
        '}',
        1,
    )
    call = (
        "    current = gluing_start_chain(&first_step, thetaQ1, thetaQ2, pts, "
        "todo, space, numP, E12, P12, ker, mode);"
    )
    if theta.count(call) != 1:
        raise RuntimeError("could not locate gluing_start_chain call")
    theta = theta.replace(
        call,
        call
        + "\n    if (sqisign_witness_gluing_failure) {\n"
        + "        sqisign_witness_continued = 1;\n"
        + "        sqisign_witness_first_read = "
        + "*((volatile unsigned char *)&first_step.codomain);\n"
        + "    }",
        1,
    )
    theta_path = directory / "theta_isogenies.c"
    theta_path.write_text(theta, encoding="utf-8")
    return gluing_path, theta_path


def compile_instrumented(output: Path, gluing: Path, theta: Path) -> None:
    dry = subprocess.run(
        ["make", "--no-print-directory", "-n", "-B", "-C", str(SCHEME), "lib/p324_3.so"],
        cwd=ROOT, capture_output=True, text=True,
    )
    if dry.returncode:
        raise RuntimeError(dry.stdout + dry.stderr)
    line = next((item for item in dry.stdout.splitlines() if " -shared " in item), None)
    if line is None:
        raise RuntimeError("could not obtain SQIsign reference compile command")
    command = shlex.split(line)
    substitutions = {
        "submission/SQIsign/Reference_Implementation/src/hd/ref/lvlx/gluing.c": str(gluing),
        "submission/SQIsign/Reference_Implementation/src/hd/ref/lvlx/theta_isogenies.c": str(theta),
    }
    command = [substitutions.get(item, item) for item in command]
    command.insert(1, f"-I{REFERENCE / 'src/hd/ref/lvlx'}")
    output_index = command.index("-o") + 1
    command[output_index] = str(output)
    completed = subprocess.run(command, cwd=SCHEME, capture_output=True, text=True)
    if completed.returncode:
        raise RuntimeError(completed.stdout + completed.stderr)


def load(path: Path):
    lib = ctypes.CDLL(str(path))
    bytep = ctypes.POINTER(ctypes.c_ubyte)
    lib.nist_sig_get_api.restype = ctypes.POINTER(Meta)
    lib.nist_sig_seed.argtypes = [bytep]
    lib.nist_sig_seed.restype = ctypes.c_int
    lib.nist_sig_keypair.argtypes = [bytep, bytep]
    lib.nist_sig_keypair.restype = ctypes.c_int
    lib.nist_sig_sign.argtypes = [
        bytep, ctypes.POINTER(ctypes.c_uint64), bytep, ctypes.c_uint64, bytep
    ]
    lib.nist_sig_sign.restype = ctypes.c_int
    lib.nist_sig_open.argtypes = [
        bytep, ctypes.POINTER(ctypes.c_uint64), bytep, ctypes.c_uint64, bytep
    ]
    lib.nist_sig_open.restype = ctypes.c_int
    lib.sqisign_witness_reset.restype = None
    lib.sqisign_witness_status.argtypes = [bytep]
    lib.sqisign_witness_status.restype = ctypes.c_int
    return lib


def open_and_status(lib, signed: bytes, public_key: bytes) -> tuple[int, int, int]:
    lib.sqisign_witness_reset()
    out = buffer(len(signed))
    outlen = ctypes.c_uint64()
    rc = lib.nist_sig_open(
        out, ctypes.byref(outlen), buffer(signed), len(signed), buffer(public_key)
    )
    first_read = buffer(1)
    status = lib.sqisign_witness_status(first_read)
    return rc, status, first_read[0]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    parser.add_argument("--attempts", type=int, default=300)
    args = parser.parse_args()
    if not shutil.which("cc") or not shutil.which("make"):
        print("SKIP SQIsign-3: compiler or make unavailable")
        return 77
    source_hashes = {
        "gluing.c": hashlib.sha256(GLUING.read_bytes()).hexdigest(),
        "theta_isogenies.c": hashlib.sha256(THETA.read_bytes()).hexdigest(),
    }
    if source_hashes != PINNED:
        raise RuntimeError(f"submitted source hashes changed: {source_hashes}")

    with tempfile.TemporaryDirectory(prefix="sqisign-3-") as temporary:
        directory = Path(temporary)
        gluing, theta = instrument_sources(directory)
        library = directory / "p324_3-witness.so"
        compile_instrumented(library, gluing, theta)
        lib = load(library)
        meta = lib.nist_sig_get_api().contents
        if [meta.pk, meta.sk, meta.sig] != [83, 270, 200]:
            raise RuntimeError("unexpected p324_3 API sizes")
        if lib.nist_sig_seed(buffer(bytes((7 + 29 * i) & 255 for i in range(48)))):
            raise RuntimeError("seeding failed")
        pk, sk = buffer(meta.pk), buffer(meta.sk)
        if lib.nist_sig_keypair(pk, sk):
            raise RuntimeError("key generation failed")
        if lib.nist_sig_seed(buffer(bytes((11 + 31 * i) & 255 for i in range(48)))):
            raise RuntimeError("signing seed failed")
        signed = buffer(meta.sig + len(MESSAGE))
        signed_len = ctypes.c_uint64()
        if lib.nist_sig_sign(
            signed, ctypes.byref(signed_len), buffer(MESSAGE), len(MESSAGE), sk
        ):
            raise RuntimeError("signing failed")
        signed_bytes = bytes(signed[:signed_len.value])
        public_key = bytes(pk)
        baseline = open_and_status(lib, signed_bytes, public_key)
        if baseline[:2] != (0, 0):
            raise RuntimeError(f"honest baseline failed: {baseline}")

        observations = []
        gluing_failures = 0
        gluing_false_acceptances = 0
        # The first 182 signature bytes contain the auxiliary curve and response
        # matrix.  Cycle deterministic single-bit changes through that region.
        for attempt in range(args.attempts):
            byte_index = attempt % 182
            bit_index = (attempt // 182) % 8
            candidate = bytearray(signed_bytes)
            candidate[byte_index] ^= 1 << bit_index
            rc, status, first_read = open_and_status(lib, bytes(candidate), public_key)
            if status:
                gluing_failures += 1
                gluing_false_acceptances += rc == 0
                if status // 10 != 1 or status % 10 != 1 or first_read != 0xA5:
                    raise RuntimeError(
                        "gluing failure did not continue from site 1 with the "
                        f"expected sentinel: attempt={attempt}, status={status}, "
                        f"first_read={first_read}"
                    )
                if len(observations) < 3:
                    observations.append({
                        "attempt": attempt,
                        "byte": byte_index,
                        "bit": bit_index,
                        "verify_rc": rc,
                        "failure_site": status // 10,
                        "continued_after_failure": status % 10,
                        "unset_codomain_sentinel_read": first_read,
                    })
        if not observations:
            raise RuntimeError(
                f"no unchecked reachable gluing failure in {args.attempts} mutations"
            )
        if gluing_false_acceptances:
            raise RuntimeError("unexpected malformed-signature acceptance")
        result = {
            "format": "SQIsign-3-unchecked-gluing-witness-v1",
            "status": "PASS",
            "parameter": "p324_3",
            "source_sha256": source_hashes,
            "instrumented_library_sha256": hashlib.sha256(library.read_bytes()).hexdigest(),
            "honest_baseline": {"verify_rc": baseline[0], "instrumentation_status": baseline[1]},
            "attempt_limit": args.attempts,
            "reachable_gluing_failures": gluing_failures,
            "accepted_after_gluing_failure": gluing_false_acceptances,
            "observations": observations,
            "observation": (
                "Malformed signatures reached an early gluing rejection, after which "
                "the caller continued and read the unchanged 0xA5 codomain sentinel."
            ),
            "limitations": [
                "The witness instruments a temporary copy of two pinned source files and initializes first_step with a visible sentinel.",
                "All observed malformed signatures were rejected; no false acceptance was found.",
                "Only the level-1 p324_3 reference build was executed; the same shared source is compiled for all levels.",
            ],
        }
    rendered = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")
    print(f"PASS SQIsign-3: reached unchecked gluing failure in {gluing_failures} mutation(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
