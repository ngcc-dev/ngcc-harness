#!/usr/bin/env python3
"""Run interleaved deterministic Loom whole-scheme searches in parallel."""
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import tempfile
import time
from pathlib import Path


def stop(processes: list[subprocess.Popen[str]]) -> None:
    for proc in processes:
        if proc.poll() is None:
            proc.terminate()
    for proc in processes:
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path, help="linked loom_failure_search executable")
    parser.add_argument("--workers", type=int, default=os.cpu_count() or 1)
    parser.add_argument("--trials", type=int, default=2_000_000, help="total indexes to cover")
    parser.add_argument("--start", type=int, default=0)
    parser.add_argument("--witness", type=Path, required=True)
    parser.add_argument("--progress", type=int, default=100_000, help="progress interval per worker; 0 disables")
    args = parser.parse_args()
    if args.workers < 1 or args.trials < 1 or args.start < 0:
        parser.error("workers/trials must be positive and start must be nonnegative")
    binary = args.binary.resolve()
    if not binary.is_file():
        parser.error(f"binary does not exist: {binary}")

    args.witness.parent.mkdir(parents=True, exist_ok=True)
    processes: list[subprocess.Popen[str]] = []
    logs: list[Path] = []
    handles = []
    started = time.monotonic()
    with tempfile.TemporaryDirectory(prefix="loom-search-", dir=args.witness.parent) as tmp_name:
        tmp = Path(tmp_name)
        try:
            for worker in range(min(args.workers, args.trials)):
                count = (args.trials - 1 - worker) // args.workers + 1
                witness = tmp / f"witness-{worker}.txt"
                log = tmp / f"worker-{worker}.log"
                handle = log.open("w")
                command = [
                    str(binary), str(args.start + worker), str(args.workers),
                    str(count), str(witness), str(args.progress),
                ]
                processes.append(subprocess.Popen(command, stdout=handle, stderr=subprocess.STDOUT, text=True))
                logs.append(log)
                handles.append(handle)
            print(
                f"launched {len(processes)} workers over {args.trials} honest four-pass exchange attempts "
                f"(indexes {args.start}..{args.start + args.trials - 1})",
                flush=True,
            )
            remaining = set(range(len(processes)))
            while remaining:
                time.sleep(0.25)
                for worker in list(remaining):
                    rc = processes[worker].poll()
                    if rc is None:
                        continue
                    remaining.remove(worker)
                    handles[worker].flush()
                    witness = tmp / f"witness-{worker}.txt"
                    if rc == 0 and witness.is_file():
                        stop(processes)
                        for handle in handles:
                            handle.close()
                        shutil.copy2(witness, args.witness)
                        detail = logs[worker].read_text(errors="replace").strip().splitlines()
                        print(detail[-1] if detail else f"worker {worker} found a failure")
                        print(f"witness={args.witness} elapsed={time.monotonic() - started:.2f}s")
                        return 0
                    if rc not in (0, 1):
                        stop(processes)
                        for handle in handles:
                            handle.close()
                        print(logs[worker].read_text(errors="replace"))
                        raise SystemExit(f"worker {worker} failed with exit status {rc}")
        except KeyboardInterrupt:
            stop(processes)
            raise
        finally:
            for handle in handles:
                if not handle.closed:
                    handle.close()
    print(f"no failure in {args.trials} exchanges; elapsed={time.monotonic() - started:.2f}s")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
