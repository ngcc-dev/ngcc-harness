#!/bin/sh
set -eu

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ref="$here/Implementations and Test_Vectors/Implementations/Reference_Implementation"
tmp=$(mktemp -d /tmp/qing-repro.XXXXXX)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

python3 "$here/repro_qing/joux_secondpreimage.py" > "$tmp/joux.log"
test "$(grep -c 'OK 2nd-preimage' "$tmp/joux.log")" -eq 10
! grep -q MISS "$tmp/joux.log"
test "$(grep -c 'LITERATURE BOUND' "$tmp/joux.log")" -eq 3
python3 "$here/repro_qing/multitarget.py" > "$tmp/multitarget.log"
test "$(grep -c 'DISTINCT slope' "$tmp/multitarget.log")" -eq 3
test "$(grep -c 'SHARED   slope' "$tmp/multitarget.log")" -eq 3
echo "ANALYSIS sign-20-3 CONFIRMED: recursive Joux preimages leave QingLuan-384/-512 below target"

keys=${QING_KEYS:-1000}
for level in 128 256 384 512; do
    src="$ref/QingLuan-$level/src"
    inc="$ref/QingLuan-$level/include"
    cc -O2 -std=c11 -I"$inc" -Dxof_squeeze=xof_squeeze_counted \
        -c "$src/rsdp.c" -o "$tmp/rsdp-$level.o"
    cc -O2 -std=c11 -I"$inc" -o "$tmp/probe-$level" \
        "$here/repro_qing/probe.c" "$here/repro_qing/count.c" "$tmp/rsdp-$level.o" \
        "$src/fq_arith.c" "$src/hash.c" "$src/utils.c" "$src/restr.c" \
        "$src/mpc.c" "$src/keygen.c" "$src/sign.c" "$src/verify.c"
    "$tmp/probe-$level" "$keys" > "$tmp/probe-$level.csv"
    python3 - "$tmp/probe-$level.csv" "$level" <<'PY'
import csv, sys
rows = list(csv.DictReader(open(sys.argv[1])))
draws = [int(row["draws_eta"]) for row in rows]
assert rows and len(set(draws)) > 1
assert all(row["draws_eta"] == row["draws_eta_repeat"] for row in rows)
print(f"QingLuan-{sys.argv[2]}: {len(rows)} keys, {len(set(draws))} repeatable draw counts")
PY
done
echo "SIDE CHANNEL sign-20-4 CONFIRMED: signing re-expands the secret with repeatable variable work"
