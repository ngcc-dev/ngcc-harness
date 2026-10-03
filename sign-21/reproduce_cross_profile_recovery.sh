#!/bin/sh
set -eu

commit=d0db6e0e0a28ef7ef41fe5dd826aa17ff743bcba
archive_sha256=ab64cb503fa185e40ab067fa6e01385d289392f429ef0eb73003509775685b9a
url="https://codeload.github.com/martinfeussner/NGCC-Signature-Audit/tar.gz/$commit"
tmp=$(mktemp -d "${TMPDIR:-/tmp}/ngcc-resolved-cross.XXXXXX")
trap 'rm -rf -- "$tmp"' EXIT HUP INT TERM

if ! curl -L --fail --silent --show-error "$url" -o "$tmp/package.tar.gz"; then
    echo "SKIP sign-21-3: pinned package download failed" >&2
    exit 77
fi
printf '%s  %s\n' "$archive_sha256" "$tmp/package.tar.gz" | sha256sum -c -
tar -xzf "$tmp/package.tar.gz" -C "$tmp"
root="$tmp/NGCC-Signature-Audit-$commit/ReSolveD-alpha/reproducer"

log="$tmp/reproducer.log"
(cd "$root" && sh ./run.sh) | tee "$log"
grep -qx 'minimal_reproducer=pass' "$log"
test -s "$root/work/160/recovered-witness.bin"
test -s "$root/work/160/forgery-s.sig"
grep -q 'different_message_pair=reject' "$log"
grep -q 'different_rho_pair=reject' "$log"
grep -q 'different_key_pair=reject' "$log"

echo "OUT-OF-MODEL sign-21-3 CONFIRMED: deterministic same-key S/F transcripts recover an equivalent witness and forge"
