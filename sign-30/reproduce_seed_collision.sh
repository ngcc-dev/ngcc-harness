#!/bin/sh
set -eu

commit=6ebb3b4ab00c68d8e72b1c08ba839ed7056dc166
archive_sha256=bc71c8e04ad22d441feb7de2f9d5c18435fdfc58a3db0648786af5f447a7ea00
url="https://codeload.github.com/martinfeussner/NGCC-Signature-Audit/tar.gz/$commit"
tmp=$(mktemp -d "${TMPDIR:-/tmp}/ngcc-trine-seed-collision.XXXXXX")
trap 'rm -rf -- "$tmp"' EXIT HUP INT TERM

if ! curl -L --fail --silent --show-error "$url" -o "$tmp/package.tar.gz"; then
    echo 'SKIP sign-30-2: pinned attack package is unavailable' >&2
    exit 77
fi
printf '%s  %s\n' "$archive_sha256" "$tmp/package.tar.gz" | sha256sum -c -
tar -xzf "$tmp/package.tar.gz" -C "$tmp"
root="$tmp/NGCC-Signature-Audit-$commit/TRINE/reproducer"
(cd "$root" && bash ./run_all.sh) | tee "$tmp/reproducer.log"
grep -Fq 'balanced_and_shortsig_fresh_forgery=PASS' "$tmp/reproducer.log"
grep -Fq 'validation=PASS' "$tmp/reproducer.log"
echo 'ATTACK sign-30-2 CONFIRMED: specified TRINE-128 round seeds permit below-target fresh-message forgeries'
