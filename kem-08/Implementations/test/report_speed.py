#!/usr/bin/env python3
import re
import sys
from pathlib import Path

ORDER = [
    "gen_matrix",
    "poly_getnoise_eta1",
    "poly_getnoise_eta2",
    "poly_ntt",
    "poly_invntt_tomont",
    "polyvec_basemul_acc_montgomery",
    "poly_tomsg",
    "poly_frommsg",
    "poly_tobytes",
    "poly_frombytes",
    "polyvec_compress",
    "polyvec_decompress",
    "encode_bw32",
    "decode_bw32",
    "indcpa_keypair_derand",
    "indcpa_enc",
    "indcpa_dec",
    "crypto_kem_keypair",
    "crypto_kem_enc",
    "crypto_kem_dec",
]

MEDIAN_RE = re.compile(r"^median:\s+(\d+)\s+cycles/ticks$")
AVERAGE_RE = re.compile(r"^average:\s+(\d+)\s+cycles/ticks$")


def parse_log(path: Path):
    rows = {}
    current = None
    median = None

    for raw in path.read_text().splitlines():
        line = raw.strip()
        if not line:
            continue
        if line.startswith("instance:") or line.startswith("implementation:"):
            continue
        m = MEDIAN_RE.match(line)
        if m:
            median = int(m.group(1))
            continue
        m = AVERAGE_RE.match(line)
        if m and current is not None and median is not None:
            rows[current] = {
                "median": median,
                "average": int(m.group(1)),
            }
            current = None
            median = None
            continue
        current = line
        median = None

    return rows


def speedup(ref, opt):
    if ref is None or opt is None or opt == 0:
        return "-"
    return f"{ref / opt:.2f}x"


def main(argv):
    if len(argv) != 3:
        raise SystemExit(f"usage: {argv[0]} <log_dir> <report_path>")

    log_dir = Path(argv[1])
    report_path = Path(argv[2])

    data = {}
    for impl in ("reference", "optimized"):
        for path in sorted(log_dir.glob(f"*.{impl}.log")):
            instance = path.name[: -(len(impl) + 5)]
            data.setdefault(instance, {})[impl] = parse_log(path)

    lines = ["# BW-KEM API Speed Report", ""]
    lines.append("按 `median cycles/ticks` 计算加速比，`speedup = reference / optimized`。")
    lines.append("")

    for instance in sorted(data):
        lines.append(f"## {instance}")
        lines.append("")
        lines.append("| Case | Reference Median | Optimized Median | Speedup | Reference Avg | Optimized Avg |")
        lines.append("| --- | ---: | ---: | ---: | ---: | ---: |")

        ref_rows = data[instance].get("reference", {})
        opt_rows = data[instance].get("optimized", {})
        seen = []
        for case in ORDER:
            if case in ref_rows or case in opt_rows:
                seen.append(case)
        for case in sorted(set(ref_rows) | set(opt_rows)):
            if case not in seen:
                seen.append(case)

        for case in seen:
            ref = ref_rows.get(case)
            opt = opt_rows.get(case)
            lines.append(
                "| {case} | {rmed} | {omed} | {sp} | {ravg} | {oavg} |".format(
                    case=case,
                    rmed=ref["median"] if ref else "-",
                    omed=opt["median"] if opt else "-",
                    sp=speedup(ref["median"] if ref else None, opt["median"] if opt else None),
                    ravg=ref["average"] if ref else "-",
                    oavg=opt["average"] if opt else "-",
                )
            )

        lines.append("")

    report_path.write_text("\n".join(lines) + "\n")


if __name__ == "__main__":
    main(sys.argv)
