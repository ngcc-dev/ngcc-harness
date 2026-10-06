#!/usr/bin/env python3
"""Find a natural zero challenge with the unmodified Sigurd-128 signer."""

import os
from pathlib import Path
import re
import subprocess
import tempfile
import time


ROOT = Path(__file__).resolve().parent
TREE = ROOT / "Implementations/Reference_Implementation/Sigurd-128"
WORKERS = min(14, os.cpu_count() or 1)
TRIALS_PER_WORKER = 10000


def stop(jobs):
    for process, handle, _, _ in jobs:
        if process.poll() is None:
            process.terminate()
    for process, handle, _, _ in jobs:
        try:
            process.wait(timeout=3)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
        handle.close()


with tempfile.TemporaryDirectory(prefix="ngcc-sigurd-natural-") as scratch_name:
    scratch = Path(scratch_name)
    binary = scratch / "natural"
    subprocess.run([
        "cc", "-O2", "-std=c11", "-I", str(TREE),
        str(ROOT / "reproduce_zero_zeta.c"),
        *(str(TREE / name) for name in (
            "SIG_AlgorithmInstance.c", "auxfunc.c", "drng.c",
            "rsencode.c", "rsencode_common.c")),
        "-o", str(binary), "-lm",
    ], check=True)

    for batch in range(8):
        jobs = []
        for slot in range(WORKERS):
            worker = batch * WORKERS + slot + 1
            public = scratch / f"public-{worker}.txt"
            truth = scratch / f"truth-{worker}.txt"
            handle = (scratch / f"worker-{worker}.log").open("w")
            process = subprocess.Popen([
                str(binary), "5", str(public), str(truth),
                str(worker), str(TRIALS_PER_WORKER),
            ], stdout=handle, stderr=subprocess.STDOUT)
            jobs.append((process, handle, public, truth))

        winner = None
        while any(process.poll() is None for process, _, _, _ in jobs):
            for process, handle, public, truth in jobs:
                if process.poll() == 0:
                    winner = (process, handle, public, truth)
                    break
            if winner:
                break
            time.sleep(0.2)
        if winner is None:
            for process, handle, public, truth in jobs:
                if process.poll() == 0:
                    winner = (process, handle, public, truth)
                    break
        stop(jobs)

        if winner:
            process, handle, public, truth = winner
            log = handle.name.read_text() if isinstance(handle.name, Path) else Path(handle.name).read_text()
            assert "signature verifier return = 0" in log, log
            assert "derived challenge zeta = 0" in log, log
            recovered = subprocess.run([
                "python3", str(ROOT / "solve_zero_zeta.py"), str(public),
            ], capture_output=True, text=True, check=True).stdout.splitlines()[-1]
            assert recovered == truth.read_text().strip(), "public witness did not match the secret"
            match = re.search(r"derived challenge zeta = 0 after (\d+) signatures", log)
            assert match, log
            print(f"CONFIRMED sign-24-2: unmodified Sigurd-128 signer produced a zero challenge "
                  f"after {match.group(1)} signatures in worker {batch * WORKERS + jobs.index(winner) + 1}; "
                  "public-data witness recovery matches the true secret")
            break
        print(f"No zero challenge in batch {batch + 1} ({WORKERS * TRIALS_PER_WORKER} "
              "same-key signatures maximum)", flush=True)
    else:
        raise RuntimeError("natural zero challenge not found within scan limit")
