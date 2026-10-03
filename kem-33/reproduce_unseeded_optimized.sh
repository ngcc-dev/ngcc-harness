#!/bin/sh
set -eu

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
commit=f9479dcb18daaa484bf3f6df36a6ec7ee209c141
archive_sha=82d27cbef98274d1b542bbbaa74e621de8e98dba175f0d28035abba811c878df
url="https://github.com/acprk/ngcc-round1-cryptanalysis/archive/$commit.tar.gz"
tmp=$(mktemp -d /tmp/qube-unseeded.XXXXXX)
cleanup() {
    rc=$?
    trap - EXIT HUP INT TERM
    if [ "$rc" -ne 0 ] && [ -f "$tmp/run.log" ]; then
        sed -n '1,240p' "$tmp/run.log" >&2
        find "$tmp" -name '*.build.log' -type f -exec sh -c \
            'for f do echo "== $f" >&2; tail -n 80 "$f" >&2; done' sh {} +
    fi
    rm -rf "$tmp"
    exit "$rc"
}
trap cleanup EXIT HUP INT TERM

if ! command -v curl >/dev/null 2>&1; then exit 77; fi
if ! curl -fsSL "$url" -o "$tmp/artifact.tar.gz"; then exit 77; fi
printf '%s  %s\n' "$archive_sha" "$tmp/artifact.tar.gz" | sha256sum -c - >/dev/null
tar -xzf "$tmp/artifact.tar.gz" -C "$tmp"
artifact="$tmp/ngcc-round1-cryptanalysis-$commit/qube-opt-unseeded-prng"

# GCC 15 otherwise rejects an undeclared POSIX clock_gettime used by the package.
(cd "$artifact" && CFLAGS=-D_GNU_SOURCE JOBS="${JOBS:-4}" \
    REF="$here/Implementations" ./run_all.sh) > "$tmp/run.log" 2>&1

test "$(grep -c 'all 10 records: PK identical, CT identical, SS identical' "$tmp/run.log")" -eq 4
test "$(sed -n '/== \[2\]/,/== \[3\]/p' "$tmp/run.log" | grep -c 'PK differ, CT differ, SS differ')" -eq 4
test "$(sed -n '/== \[3\]/,/== \[4\]/p' "$tmp/run.log" | grep -c 'PK differ, CT differ, SS differ')" -eq 2
test "$(grep -c 'attacker pk == Alice pk: YES .*attacker sk == Alice sk: YES .*attacker ss == Bob ss: YES' "$tmp/run.log")" -eq 4
test "$(grep -c 'attacker ct == Dave ct: YES .*attacker ss == Dave ss: YES .*Carol decaps == Dave ss: YES' "$tmp/run.log")" -eq 4

echo "ATTACK kem-33-3 CONFIRMED: full key and sender-session recovery reproduced on all four optimized sets"
