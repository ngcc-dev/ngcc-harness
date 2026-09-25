#!/usr/bin/env python3
"""Show that a malformed NIIKE-lv128 peer key terminates the honest party's process."""
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from niike_witness_common import derive, keygen, load, swap_pq   # noqa: E402


def child(variant):
    lib = load()
    pk_a, sk_a = keygen(lib, os.urandom(64))
    pk_b, _ = keygen(lib, os.urandom(64))
    peer = pk_b if variant == "honest" else swap_pq(pk_b)
    rc, _ = derive(lib, sk_a, peer)
    print(f"rc={rc}")


def main():
    if len(sys.argv) == 3 and sys.argv[1] == "--child":
        child(sys.argv[2]); return
    for variant in ("honest", "swapped"):
        res = subprocess.run([sys.executable, __file__, "--child", variant],
                             capture_output=True, text=True, timeout=30)
        tail = (res.stderr.strip().splitlines() or [res.stdout.strip()])[-1]
        label = "CONTROL honest peer key" if variant == "honest" else "CONFIRMED P/Q-swapped peer key"
        print(f"{label}: exit {res.returncode}: {tail[-110:]}")
        assert (res.returncode == 0 and "rc=0" in res.stdout) if variant == "honest" \
            else res.returncode < 0, f"{variant}: unexpected result"


if __name__ == "__main__":
    main()
