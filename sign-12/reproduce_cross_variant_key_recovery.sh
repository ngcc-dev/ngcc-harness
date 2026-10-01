#!/bin/sh
set -eu

commit=d0db6e0e0a28ef7ef41fe5dd826aa17ff743bcba
archive_sha256=ab64cb503fa185e40ab067fa6e01385d289392f429ef0eb73003509775685b9a
url="https://codeload.github.com/martinfeussner/NGCC-Signature-Audit/tar.gz/$commit"
tmp=$(mktemp -d "${TMPDIR:-/tmp}/ngcc-galas-cross.XXXXXX")
trap 'rm -rf -- "$tmp"' EXIT HUP INT TERM

if ! curl -L --fail --silent --show-error "$url" -o "$tmp/package.tar.gz"; then
    echo "SKIP sign-12-3: pinned package download failed" >&2
    exit 77
fi
printf '%s  %s\n' "$archive_sha256" "$tmp/package.tar.gz" | sha256sum -c -
tar -xzf "$tmp/package.tar.gz" -C "$tmp"
root="$tmp/NGCC-Signature-Audit-$commit/Galas/reproducer"

log="$tmp/reproducer.log"
sh "$root/run.sh" "$tmp/out" | tee "$log"
grep -qx 'minimal_reproducer=pass' "$log"
test -s "$tmp/out/sk-original.bin"
cmp "$tmp/out/sk-original.bin" "$tmp/out/sk-recovered.bin"
test -s "$tmp/out/forgery-160f.bin"

echo "ATTACK sign-12-3 CONFIRMED: same-key S/F signatures recover the exact Galas key and forge"
