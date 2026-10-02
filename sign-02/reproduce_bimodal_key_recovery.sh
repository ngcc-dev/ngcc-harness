#!/bin/sh
set -eu

commit=91f2ddf0590a24ad39afc2ce2ee4f2627ec54726
archive_sha256=cdd6e44eaea61191ecd33b330f14d061c12c46a5ea13bb74b85d104fb29dbeb2
url="https://codeload.github.com/martinfeussner/NGCC-Signature-Audit/tar.gz/$commit"
tmp=$(mktemp -d "${TMPDIR:-/tmp}/ngcc-bit-bimodal.XXXXXX")
trap 'rm -rf -- "$tmp"' EXIT HUP INT TERM

if ! curl -L --fail --silent --show-error "$url" -o "$tmp/package.tar.gz"; then
    echo 'SKIP sign-02-2: pinned attack package is unavailable' >&2
    exit 77
fi
printf '%s  %s\n' "$archive_sha256" "$tmp/package.tar.gz" | sha256sum -c -
tar -xzf "$tmp/package.tar.gz" -C "$tmp"
root="$tmp/NGCC-Signature-Audit-$commit/BiT/reproducer"
log="$tmp/reproducer.log"
bit_url='https://www.niccs.org.cn/niccs/Proposal/Public-Key%20Cryptographic%20Algorithms/Round%201%20candidates/BiT.zip'
bit_sha256=698fbe834279a4100a66c20f3e0b634c738e1150936acf74b55efc6db6975e8d

if ! curl -L --fail --silent --show-error "$bit_url" -o "$tmp/BiT.zip"; then
    echo 'SKIP sign-02-2: archived BiT submission is unavailable' >&2
    exit 77
fi
printf '%s  %s\n' "$bit_sha256" "$tmp/BiT.zip" | sha256sum -c -

(cd "$root" && BIT_ARCHIVE="$tmp/BiT.zip" \
    sh ./run.sh "${BIT_SIGNATURES:-200000}" "${BIT_KEY_ID:-0}") 2>&1 | tee "$log"
grep -Fq 'equivalent-key=RECOVERED' "$log"
grep -Fq 'fresh-message-forgery=ACCEPTED' "$log"
grep -Fq 'changed-message=REJECTED' "$log"

echo 'ATTACK sign-02-2 CONFIRMED: public transcript recovers an equivalent BiT-128 key and forges'
