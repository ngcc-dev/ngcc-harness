#!/bin/sh
set -eu

commit=709d5ec64239206effb2667ca853ddcef3c060b4
archive_sha256=4e666aedf96dd8a4642ee23024c64c6cb4e691cd08547eb504e98b805163a0b9
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
scratch=$(mktemp -d "${TMPDIR:-/tmp}/ngcc-scloud.XXXXXX")
trap 'rm -rf "$scratch"' EXIT HUP INT TERM

if ! curl -L --fail --silent --show-error \
    "https://codeload.github.com/acprk/ngcc-round1-cryptanalysis/tar.gz/$commit" \
    -o "$scratch/source.tar.gz"; then
    echo 'SKIP kem-35-1: pinned external artifact is unavailable' >&2
    exit 77
fi
printf '%s  %s\n' "$archive_sha256" "$scratch/source.tar.gz" | sha256sum -c -
tar -xzf "$scratch/source.tar.gz" -C "$scratch"
artifact="$scratch/ngcc-round1-cryptanalysis-$commit/scloudplus-reencryption-timing"

log="$scratch/run.log"
if ! (
    cd "$artifact"
    REF="$here/Implementations and Test_Vectors/Implementations" \
        CC="${CC:-gcc}" ./run_all.sh
) >"$log" 2>&1; then
    cat "$log"
    exit 1
fi
cat "$log"

grep -Eq 'XOF squeezes: \{[0-9]+: [1-9][0-9]*, [0-9]+: [1-9][0-9]*\}' "$log"
grep -Fq 'ROBUST observable: class gap' "$log"
grep -Fq 'Welch t = ' "$log"
grep -Fq 'Done. The squeeze count is a deterministic function of' "$log"
echo 'ATTACK kem-35-1 Scloud+ RE-ENCRYPTION TIMING: CONFIRMED'
