#!/usr/bin/env python3

import argparse
import socket
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Tuple


REPO_ROOT = Path(__file__).resolve().parents[1]
TESTDATA_ROOT = REPO_ROOT / "testdata"


@dataclass(frozen=True)
class TaskSpec:
    name: str
    cmd_hex: str
    input_slots: Tuple[Tuple[str, str], ...]
    output_slots: Tuple[Tuple[str, str], ...]


TASK_SPECS: Dict[str, TaskSpec] = {
    "keygen": TaskSpec(
        name="keygen",
        cmd_hex="a0",
        input_slots=(("seed_in", "keygen_seed.mem"),),
        output_slots=(("pk_out", "keygen_pk_packed.mem"), ("sk_out", "keygen_sk_packed.mem")),
    ),
    "enc": TaskSpec(
        name="enc",
        cmd_hex="a1",
        input_slots=(("pk_in", "enc_pk_packed.mem"), ("msg_in", "enc_msg.mem"), ("seed_in", "enc_seed.mem")),
        output_slots=(("ct_out", "enc_ct_packed.mem"),),
    ),
    "dec": TaskSpec(
        name="dec",
        cmd_hex="a2",
        input_slots=(("ct_in", "dec_ct_packed.mem"), ("sk_in", "dec_sk_packed.mem")),
        output_slots=(("msg_out", "dec_msg.mem"),),
    ),
}


PROFILE_DIRS: Dict[str, Path] = {
    "swift128": TESTDATA_ROOT,
}

PROFILE_BASE_SEEDS: Dict[str, int] = {
    "swift128": 0x11,
}


class TaskServiceClient:
    def __init__(self, host: str, port: int, timeout_s: float) -> None:
        self.sock = socket.create_connection((host, port), timeout=timeout_s)
        self.sock.settimeout(timeout_s)
        self.io = self.sock.makefile("rwb")

    def close(self) -> None:
        try:
            self.io.close()
        finally:
            self.sock.close()

    def command(self, line: str) -> str:
        self.io.write((line + "\n").encode("ascii"))
        self.io.flush()
        resp = self.io.readline()
        if not resp:
            raise RuntimeError(f"no response for command: {line}")
        text = resp.decode("ascii", errors="strict").strip()
        if not text.startswith("OK"):
            raise RuntimeError(f"command failed: {line} -> {text}")
        return text


def load_mem_bytes(path: Path) -> bytes:
    tokens: List[int] = []
    with path.open("r", encoding="ascii") as fh:
        for raw_line in fh:
            line = raw_line.strip()
            if not line or line.startswith("//"):
                continue
            for token in line.split():
                if token.startswith("//"):
                    break
                if token.startswith("@"):
                    continue
                tokens.append(int(token, 16) & 0xFF)
    return bytes(tokens)


def to_hex(data: bytes) -> str:
    return data.hex()


def compare_bytes(got: bytes, exp: bytes) -> Tuple[bool, str]:
    if got == exp:
        return True, ""
    limit = min(len(got), len(exp))
    for idx in range(limit):
        if got[idx] != exp[idx]:
            return False, f"idx={idx} got=0x{got[idx]:02x} exp=0x{exp[idx]:02x}"
    return False, f"length mismatch got={len(got)} exp={len(exp)}"


def deterministic_bytes(length: int, seed: int) -> bytes:
    return bytes(((seed + idx * 17) & 0xFF) for idx in range(length))


def build_task_run_line(profile: str, spec: TaskSpec) -> str:
    inputs = ["none", "none", "none"]
    outputs = ["none", "none"]
    for idx, (slot_name, _) in enumerate(spec.input_slots):
        inputs[idx] = slot_name
    for idx, (slot_name, _) in enumerate(spec.output_slots):
        outputs[idx] = slot_name
    return (
        f"TASK_RUN {spec.cmd_hex} {profile} "
        f"{inputs[0]} {inputs[1]} {inputs[2]} {outputs[0]} {outputs[1]} 0"
    )


def build_task_bench_line(profile: str, spec: TaskSpec, repeat: int, timeout_ms: int) -> str:
    inputs = ["none", "none", "none"]
    outputs = ["none", "none"]
    for idx, (slot_name, _) in enumerate(spec.input_slots):
        inputs[idx] = slot_name
    for idx, (slot_name, _) in enumerate(spec.output_slots):
        outputs[idx] = slot_name
    return (
        f"TASK_BENCH {repeat} {spec.cmd_hex} {profile} "
        f"{inputs[0]} {inputs[1]} {inputs[2]} {outputs[0]} {outputs[1]} 0 {timeout_ms}"
    )


def run_reference_task(client: TaskServiceClient,
                       profile: str,
                       dataset_dir: Path,
                       spec: TaskSpec,
                       timeout_ms: int) -> float:
    for slot_name, rel_path in spec.input_slots:
        payload = load_mem_bytes(dataset_dir / rel_path)
        client.command(f"TASK_SLOT_WR {profile} {slot_name} {to_hex(payload)}")

    started = time.perf_counter()
    client.command(build_task_run_line(profile, spec))
    wait_resp = client.command(f"WAIT {timeout_ms}")
    elapsed_ms = (time.perf_counter() - started) * 1000.0
    if wait_resp != "OK DONE":
        raise RuntimeError(f"{profile} {spec.name} reported accelerator error: {wait_resp}")

    for slot_name, rel_path in spec.output_slots:
        exp = load_mem_bytes(dataset_dir / rel_path)
        resp = client.command(f"TASK_SLOT_RD {profile} {slot_name}")
        _, got_hex = resp.split(maxsplit=1)
        got = bytes.fromhex(got_hex)
        ok, detail = compare_bytes(got, exp)
        if not ok:
            raise RuntimeError(f"{profile} {spec.name} {slot_name} mismatch: {detail}")

    return elapsed_ms


def run_reference_task_session_repeat(client: TaskServiceClient,
                                      profile: str,
                                      dataset_dir: Path,
                                      spec: TaskSpec,
                                      repeat: int,
                                      timeout_ms: int) -> float:
    for slot_name, rel_path in spec.input_slots:
        payload = load_mem_bytes(dataset_dir / rel_path)
        client.command(f"TASK_SLOT_WR {profile} {slot_name} {to_hex(payload)}")

    started = time.perf_counter()
    client.command(build_task_bench_line(profile, spec, repeat, timeout_ms))
    elapsed_ms = (time.perf_counter() - started) * 1000.0

    for slot_name, rel_path in spec.output_slots:
        exp = load_mem_bytes(dataset_dir / rel_path)
        resp = client.command(f"TASK_SLOT_RD {profile} {slot_name}")
        _, got_hex = resp.split(maxsplit=1)
        got = bytes.fromhex(got_hex)
        ok, detail = compare_bytes(got, exp)
        if not ok:
            raise RuntimeError(f"{profile} {spec.name} {slot_name} mismatch: {detail}")

    return elapsed_ms


def run_waited_task(client: TaskServiceClient, task_run_line: str, timeout_ms: int) -> float:
    started = time.perf_counter()
    client.command(task_run_line)
    wait_resp = client.command(f"WAIT {timeout_ms}")
    elapsed_ms = (time.perf_counter() - started) * 1000.0
    if wait_resp != "OK DONE":
        raise RuntimeError(f"virtual task reported accelerator error: {wait_resp}")
    return elapsed_ms


def read_slot_hex(client: TaskServiceClient, profile: str, slot_name: str) -> bytes:
    resp = client.command(f"TASK_SLOT_RD {profile} {slot_name}")
    _, got_hex = resp.split(maxsplit=1)
    return bytes.fromhex(got_hex)


def run_kem_roundtrip(client: TaskServiceClient, profile: str, timeout_ms: int) -> float:
    base_seed = PROFILE_BASE_SEEDS[profile]
    pke_seed = deterministic_bytes(64, base_seed)
    keygen_random = deterministic_bytes(64, base_seed + 0x20)
    enc_random = deterministic_bytes(64, base_seed + 0x40)
    total_elapsed_ms = 0.0

    client.command(f"TASK_SLOT_WR {profile} seed_in {to_hex(pke_seed)}")
    client.command(f"TASK_SLOT_WR {profile} rand_in {to_hex(keygen_random)}")
    total_elapsed_ms += run_waited_task(
        client,
        f"TASK_RUN b0 {profile} seed_in rand_in none pk_out kem_sk_out 0",
        timeout_ms,
    )

    client.command(f"TASK_SLOT_WR {profile} seed_in {to_hex(enc_random)}")
    total_elapsed_ms += run_waited_task(
        client,
        f"TASK_RUN b1 {profile} pk_in seed_in none ct_out ss_out 0",
        timeout_ms,
    )
    ss_enc = read_slot_hex(client, profile, "ss_out")

    total_elapsed_ms += run_waited_task(
        client,
        f"TASK_RUN b2 {profile} ct_in kem_sk_in none ss_out status_out 0",
        timeout_ms,
    )
    ss_dec = read_slot_hex(client, profile, "ss_out")
    status_bytes = read_slot_hex(client, profile, "status_out")
    if len(status_bytes) != 4:
        raise RuntimeError(f"{profile} status_out length mismatch: got={len(status_bytes)} exp=4")
    if status_bytes[0] != 0 or any(status_bytes[1:]):
        raise RuntimeError(f"{profile} verify/status mismatch: status_out={status_bytes.hex()}")
    ok, detail = compare_bytes(ss_dec, ss_enc)
    if not ok:
        raise RuntimeError(f"{profile} shared-secret mismatch: {detail}")

    return total_elapsed_ms


def iter_profiles(profile_arg: str) -> Iterable[str]:
    if profile_arg == "all":
        return ("swift128",)
    return (profile_arg,)


def iter_tasks(task_arg: str) -> Iterable[str]:
    if task_arg == "all":
        return ("keygen", "enc", "dec", "kem_roundtrip")
    return (task_arg,)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run task-level reference validation against zen_fpga_accel_tcp_service."
    )
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=3333)
    parser.add_argument("--profile", choices=("swift128", "all"), default="all")
    parser.add_argument("--task", choices=("keygen", "enc", "dec", "kem_roundtrip", "all"), default="all")
    parser.add_argument("--timeout-ms", type=int, default=30000)
    parser.add_argument("--repeat", type=int, default=1)
    parser.add_argument("--session-repeat", action="store_true")
    parser.add_argument("--keep-going", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    failures = 0
    timeout_s = max(5.0, args.timeout_ms / 1000.0 + 5.0)
    if args.session_repeat and args.repeat > 1:
        timeout_s = max(timeout_s, args.repeat * (args.timeout_ms / 1000.0) + 5.0)

    try:
        client = TaskServiceClient(args.host, args.port, timeout_s=timeout_s)
    except OSError as exc:
        print(f"connect failed: {exc}", file=sys.stderr)
        return 2

    try:
        print(client.command("PING"))
        for profile in iter_profiles(args.profile):
            dataset_dir = PROFILE_DIRS[profile]
            for task_name in iter_tasks(args.task):
                if args.session_repeat and args.repeat > 1 and task_name != "kem_roundtrip":
                    spec = TASK_SPECS[task_name]
                    label = f"{profile} {task_name} session_repeat={args.repeat}"
                    try:
                        elapsed_ms = run_reference_task_session_repeat(
                            client,
                            profile,
                            dataset_dir,
                            spec,
                            args.repeat,
                            args.timeout_ms,
                        )
                        avg_ms = elapsed_ms / args.repeat
                        print(f"PASS {label} elapsed_ms={elapsed_ms:.2f} avg_ms={avg_ms:.2f}")
                        print(f"SUMMARY {profile} {task_name} pass={args.repeat} avg_ms={avg_ms:.2f}")
                    except Exception as exc:  # noqa: BLE001
                        failures += 1
                        print(f"FAIL {label}: {exc}", file=sys.stderr)
                        if not args.keep_going:
                            return 1
                    continue

                timings: List[float] = []
                for rep in range(args.repeat):
                    label = f"{profile} {task_name} run {rep + 1}/{args.repeat}"
                    try:
                        if task_name == "kem_roundtrip":
                            elapsed_ms = run_kem_roundtrip(client, profile, args.timeout_ms)
                        else:
                            spec = TASK_SPECS[task_name]
                            elapsed_ms = run_reference_task(client, profile, dataset_dir, spec, args.timeout_ms)
                        timings.append(elapsed_ms)
                        print(f"PASS {label} elapsed_ms={elapsed_ms:.2f}")
                    except Exception as exc:  # noqa: BLE001
                        failures += 1
                        print(f"FAIL {label}: {exc}", file=sys.stderr)
                        if not args.keep_going:
                            return 1
                        break
                if timings:
                    avg_ms = sum(timings) / len(timings)
                    print(f"SUMMARY {profile} {task_name} pass={len(timings)} avg_ms={avg_ms:.2f}")
    finally:
        client.close()

    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
