#!/bin/sh
set -eu

commit=91f2ddf0590a24ad39afc2ce2ee4f2627ec54726
archive_sha=cdd6e44eaea61191ecd33b330f14d061c12c46a5ea13bb74b85d104fb29dbeb2
url="https://github.com/martinfeussner/NGCC-Signature-Audit/archive/$commit.tar.gz"
tmp=$(mktemp -d /tmp/qing-rollback.XXXXXX)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

if ! command -v curl >/dev/null 2>&1; then exit 77; fi
if ! curl -fsSL "$url" -o "$tmp/audit.tar.gz"; then exit 77; fi
printf '%s  %s\n' "$archive_sha" "$tmp/audit.tar.gz" | sha256sum -c - >/dev/null
tar -xzf "$tmp/audit.tar.gz" -C "$tmp"
repro="$tmp/NGCC-Signature-Audit-$commit/Qing-Luan/reproducer"
(cd "$repro" && ./build_and_run.sh) > "$tmp/run.log"

for level in 128 256 384 512; do
    grep -q "PASS profile=QingLuan-$level .*fresh-message-forgery=accepted public-relation=verified" "$tmp/run.log"
    grep -q "PASS level=$level .*fresh_forgery=1 .*fresh_state_no_extract=1" "$tmp/run.log"
done
grep -q 'PASS QingLuan-512: one 256-bit SM3 chaining state' "$tmp/run.log"
echo "ATTACK sign-20-2 CONFIRMED: rollback recovers an equivalent witness and fresh-message forgery"
echo "ATTACK sign-20-5 CONFIRMED: level-512 XOF stream reconstructed from one 256-bit post-key state"
