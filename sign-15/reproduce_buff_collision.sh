#!/bin/sh
# sign-15-15: ATLAS challenge collisions break the claimed BUFF binding below
# each set's level.
#
# For each unmodified reference set, builds a chosen key (rho, t1 = 0) and a
# zero-response signature, checks that verification reduces to the challenge
# equality (one message accepts, another rejects), and prints the challenge
# image and generic collision cost. A full-size collision is not searched.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-15-15: gcc unavailable'; exit 77; }
command -v python3 >/dev/null 2>&1 || { echo 'SKIP sign-15-15: python3 unavailable'; exit 77; }

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
ok=0
for l in 128 192 256 512; do
    src=sign-15/Implementation/Reference_Implementation/lwrdsa$l
    gcc -O1 -std=gnu11 -D_GNU_SOURCE -DNGCC_BUILD_SIG '-DNGCC_ID="sign-15"' \
        '-DNGCC_ALG="ATLAS"' '-DNGCC_VARIANT="ref"' -I "$src" -I api \
        -o "$scratch/b$l" sign-15/reproduce_buff_collision.c \
        $(ls "$src"/*.c | grep -v KAT_SIG.c) api/link_shim.c -lm 2>"$scratch/cc$l" ||
        { cat "$scratch/cc$l"; echo "NOT CONFIRMED sign-15-15: ATLAS-$l build failed"; exit 1; }
    out=$("$scratch/b$l") && ok=$((ok + 1))
    echo "ATLAS-$l: $out"
done
python3 - <<'PY'
from math import comb, log2
rows = [("ATLAS-128", 128, 31, 128), ("ATLAS-192 (kappa=64 as submitted)", 128, 64, 192),
        ("ATLAS-256", 256, 60, 256), ("ATLAS-512 (316 reachable positions as implemented)", 316, 60, 512)]
for name, n, k, lam in rows:
    image = log2(comb(n, k)) + k
    print(f"{name}: challenge image 2^{image:.2f}, collision about 2^{image / 2:.2f}, level {lam}")
PY
[ "$ok" -eq 4 ] || { echo "NOT CONFIRMED sign-15-15: $ok/4 sets reduced to the challenge equality"; exit 1; }
echo 'CONFIRMED sign-15-15: zero-response signatures under a t1 = 0 key verify exactly when the challenge matches; challenge collisions cost about half the image exponent'
exit 0
