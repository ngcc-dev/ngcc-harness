#!/bin/sh
set -eu

COMMIT=0366a8aaf2914d08a5bb8e672e68a87e40a8ef15
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
PYTHON=${PYTHON:-python3}
TMP=$(mktemp -d /tmp/ngcc-loong256.XXXXXX)
trap 'rm -rf -- "$TMP"' EXIT HUP INT TERM

"$PYTHON" -c 'import fpylll' 2>/dev/null || {
    echo 'set PYTHON to an interpreter that provides fpylll' >&2
    exit 2
}

curl -fsSL "https://github.com/ifeelok92/ngcc-analysis/archive/$COMMIT.tar.gz" |
    tar -xz -C "$TMP"
POC="$TMP/ngcc-analysis-$COMMIT/instances/loong256"
SRC="$HERE/Implementations/Reference_Implementation/Loong256"

# On Sage/Python builds where fpylll returns Sage Integer objects, the PoC's
# diagnostic RMS calculation cannot divide by a Python int. Convert the sum to
# float first; this runs only after BKZ and does not change candidate recovery.
sed -i 's@rms = (sum(x \* x for x in res) / len(res)) \*\* 0.5@rms = (float(sum(x * x for x in res)) / len(res)) ** 0.5@' \
    "$POC/attack_loong256_kem.py"
grep -Fq 'rms = (float(sum(x * x for x in res)) / len(res)) ** 0.5' \
    "$POC/attack_loong256_kem.py"

cc -O2 -I"$SRC" -o "$TMP/dump" "$POC/dump_loong256_kem.c" \
    "$SRC/KEM_Loong.c" "$SRC/poly.c" "$SRC/auxfunc.c" "$SRC/drng.c"
cc -O2 -I"$SRC" -o "$TMP/verify" "$POC/verify_loong256_candidates.c" \
    "$SRC/poly.c" "$SRC/auxfunc.c" "$SRC/drng.c"

"$TMP/dump" 67 > "$TMP/control.txt"
CONTROL_SS=$(awk '$1 == "SS" {print $2}' "$TMP/control.txt")
grep -v '^SS ' "$TMP/control.txt" > "$TMP/public.txt"

PYTHONUNBUFFERED=1 "$PYTHON" "$POC/attack_loong256_kem.py" \
    "$TMP/public.txt" --verifier "$TMP/verify" --m 240 --block 40 |
    tee "$TMP/attack.log"

grep -Fq 'r2_candidates=1' "$TMP/attack.log"
grep -Fq 'message_candidates=65536' "$TMP/attack.log"
grep -Fq 'FOUND checked=' "$TMP/attack.log"
RECOVERED_SS=$(awk '$1 == "SS" {print $2}' "$TMP/attack.log" | tail -n 1)
test -n "$CONTROL_SS" && test "$RECOVERED_SS" = "$CONTROL_SS"
printf '%s\n' 'ATTACK kem-18-3 Loong256 CONFIRMED public-only-shared-secret-recovery'
