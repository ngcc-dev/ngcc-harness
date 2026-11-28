#!/bin/sh
# sign-14-6: one full-size guess of the Lynxer-160 key recovery.
#   Builds reproduce_lynx160_dump.c with the unmodified Lynxer-160f reference
#   sources and the degree-3 XL solver reproduce_lynx160_xl.c (OpenMP, BMI2),
#   then runs reproduce_lynx160_key_recovery.py for the seeds given in
#   NGCC_LYNX_SEEDS (default 1). Per seed: the model must reproduce the
#   submitted code, the correct guess must solve to the secret key, and a
#   wrong guess must be rejected. About 40 seconds per seed on 16 cores and
#   250 MB of memory.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-14-6: gcc unavailable'; exit 77; }
command -v python3 >/dev/null 2>&1 || { echo 'SKIP sign-14-6: python3 unavailable'; exit 77; }
grep -qw bmi2 /proc/cpuinfo 2>/dev/null || { echo 'SKIP sign-14-6: CPU lacks BMI2'; exit 77; }
d=sign-14/Implementations/Reference_Implementation/Lynxer-160f
[ -d "$d" ] || { echo 'NOT CONFIRMED sign-14-6: tree not found'; exit 1; }
scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
(cd "$d" && gcc -O2 -std=gnu11 -w -DXOF_PSEUDO -I. -o "$scratch/dump" "$here/reproduce_lynx160_dump.c" \
    lynx_matrices.c owf.c fields.c instances.c utils.c auxfunc.c drng.c) >"$scratch/cc" 2>&1 ||
    { tail -20 "$scratch/cc"; echo 'NOT CONFIRMED sign-14-6: dump build failed'; exit 1; }
gcc -O3 -march=native -fopenmp -w -o "$scratch/xl" "$here/reproduce_lynx160_xl.c" -lm >"$scratch/cc" 2>&1 ||
    { tail -20 "$scratch/cc"; echo 'SKIP sign-14-6: XL solver build failed (needs OpenMP)'; exit 77; }
PYTHONDONTWRITEBYTECODE=1 python3 -B "$here/reproduce_lynx160_key_recovery.py" "$scratch/dump" "$scratch/xl" "${NGCC_LYNX_SEEDS:-1}"
