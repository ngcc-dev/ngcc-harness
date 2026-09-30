#!/bin/sh
set -eu

COMMIT=729616512528d6e8aa06e85707e68cd4cf09a62a
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TMP=$(mktemp -d /tmp/ngcc-chinith-em.XXXXXX)
trap 'rm -rf -- "$TMP"' EXIT HUP INT TERM

curl -fsSL "https://github.com/acprk/ngcc-round1-cryptanalysis/archive/$COMMIT.tar.gz" |
    tar -xz -C "$TMP"
AUDIT="$TMP/ngcc-round1-cryptanalysis-$COMMIT/chinith-em-misalignment"
OUT="$TMP/run.log"

REF="$HERE/Implementations/Reference_Implementation" JOBS="${JOBS:-4}" \
    "$AUDIT/harness/run_all.sh" | tee "$OUT"

grep -Fq '5 [a0check] signer a0 == verifier a0 : YES' "$OUT"
test "$(grep -Fc '5 [a0check] signer a0 == verifier a0 : NO' "$OUT")" -eq 2
grep -Fq '[valchk] n=576 nonzero values: norm=0 io0=0 io1=0' "$OUT"
grep -Fq 'uBlock-256/256 KAT: PASS' "$OUT"

REF="$HERE/Implementations/Reference_Implementation" "$AUDIT/build.sh"
PK1=$(tr -dc '0-9a-fA-F' < "$AUDIT/fixedpoint/pk1_rand.txt" | tail -c 64)
X=$(printf '01%.0s' $(seq 1 32))
for byte in 00 ff; do
    PK2=$(printf "$byte%.0s" $(seq 1 32))
    DEMO=$(python3 "$AUDIT/fixedpoint/spec_witness.py" demo "$PK1" "$X" "$PK2")
    printf '%s\n' "$DEMO"
    printf '%s\n' "$DEMO" | grep -Fq 'failing constraint indices j: [0]'
    printf '%s\n' "$DEMO" | grep -Fq 'All j=1..11 pass for ARBITRARY pk2.'
done

printf '%s\n' 'ATTACK sign-05-4 uBlockith-EM CONFIRMED relation-and-completeness'
printf '%s\n' 'LIMITATION sign-05-4 the 2^135.5 full enumeration was not run'
printf '%s\n' 'ATTACK sign-05-5 Vistrutith CONFIRMED honest-proof-mismatch'
