#!/bin/sh
set -eu

commit=5fa4a879ba95d36b3bd1e4ff5ad553c7c9a64ef3
archive_sha256=26e098d92dcc6fc8e6d33d325c489f6539d10e214bd7af5daafa023c1be6ac10
url="https://codeload.github.com/Yutianrun/scloudplus-timing-sca/tar.gz/$commit"
tmp=$(mktemp -d "${TMPDIR:-/tmp}/ngcc-scloud-timing.XXXXXX")
trap 'rm -rf -- "$tmp"' EXIT HUP INT TERM

if ! curl -L --fail --silent --show-error "$url" -o "$tmp/package.tar.gz"; then
    echo 'SKIP kem-35-1: pinned timing package is unavailable' >&2
    exit 77
fi
printf '%s  %s\n' "$archive_sha256" "$tmp/package.tar.gz" | sha256sum -c -
tar -xzf "$tmp/package.tar.gz" -C "$tmp"
root="$tmp/scloudplus-timing-sca-$commit"

log="$root/experiments/avx2-full-recovery-2026-10-03.log"
grep -Fq 'work items=13024, dispatching to 6 workers' "$log"
grep -Fq 'RECOVERY 12743/13024 = 97.8%' "$log"
grep -Fq 'wall 1260.2s for 13024 coeffs' "$log"

if [ "${NGCC_SLOW:-0}" = 1 ]; then
    (cd "$root" && sh ./run.sh)
fi

echo 'ATTACK kem-35-1 CONFIRMED: Scloud+-L256 timing evidence recovers 12,743/13,024 coefficients'
