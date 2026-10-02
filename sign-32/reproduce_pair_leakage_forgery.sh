#!/bin/sh
set -eu

commit=91f2ddf0590a24ad39afc2ce2ee4f2627ec54726
archive_sha256=cdd6e44eaea61191ecd33b330f14d061c12c46a5ea13bb74b85d104fb29dbeb2
url="https://codeload.github.com/martinfeussner/NGCC-Signature-Audit/tar.gz/$commit"
tmp=$(mktemp -d "${TMPDIR:-/tmp}/ngcc-uvw-pairs.XXXXXX")
trap 'rm -rf -- "$tmp"' EXIT HUP INT TERM

if ! curl -L --fail --silent --show-error "$url" -o "$tmp/package.tar.gz"; then
    echo 'SKIP sign-32-3: pinned attack package is unavailable' >&2
    exit 77
fi
printf '%s  %s\n' "$archive_sha256" "$tmp/package.tar.gz" | sha256sum -c -
tar -xzf "$tmp/package.tar.gz" -C "$tmp"
root="$tmp/NGCC-Signature-Audit-$commit/UVW/reproducer"
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
submission="$here/Implementations/Reference_Implementation/UVW-128"

if [ -n "${NGCC_SAGE_PYTHON:-}" ]; then
    python=$NGCC_SAGE_PYTHON
elif [ -n "${PYTHON:-}" ]; then
    python=$PYTHON
elif python3 -c 'import numpy' >/dev/null 2>&1; then
    python=python3
elif command -v sage >/dev/null 2>&1 && sage -python -c 'import numpy' >/dev/null 2>&1; then
    python=$(sage -python -c 'import sys; print(sys.executable)')
else
    echo 'SKIP sign-32-3: Python with NumPy is unavailable; set NGCC_SAGE_PYTHON' >&2
    exit 77
fi
if ! "$python" -c 'import numpy' >/dev/null 2>&1; then
    echo 'SKIP sign-32-3: selected Python lacks NumPy; set NGCC_SAGE_PYTHON' >&2
    exit 77
fi

# Build the public recovery directly against the archived source in this
# artifact.  The upstream build helper otherwise clones a second, 360 MB
# harness checkout.  Only the temporary source copy is cleaned of DBG prints.
source="$tmp/source"
cp -R "$submission" "$source"
sed \
    -e '/size_t fe_hw = vf3_hamming_weight(fe);/d' \
    -e '/size_t d_zero = 0;/d' \
    -e '/size_t p_dup = 0;/d' \
    -e '/d_zero++/d' \
    -e '/p_dup++/d' \
    -e '/fprintf(stderr, "DBG_/d' \
    "$source/uvw.c" > "$tmp/uvw.c.clean"
mv "$tmp/uvw.c.clean" "$source/uvw.c"

cc=${CC:-cc}
cflags=${CFLAGS:--O3 -std=gnu11 -Wall -Wextra}
includes="-I$source -I$source/fq_arithmetic"
common="$source/uvw.c $source/gauss.c $source/auxfunc.c $source/drng.c \
$source/fq_arithmetic/vf3.c $source/fq_arithmetic/mf3.c \
$source/fq_arithmetic/dp.c"
# shellcheck disable=SC2086
$cc $cflags -DUSE_API_PKC $includes "$root/uvw_public_forgery.c" $common \
    -lm -o "$tmp/uvw_public_forgery"

log="$tmp/reproducer.log"
evidence="$root/evidence"
(cd "$evidence" && sha256sum --check SHA256SUMS)
env OPENBLAS_NUM_THREADS="${OPENBLAS_NUM_THREADS:-4}" \
    "$python" "$root/uvw_pair_likelihood.py" \
    "$evidence/uvw300.errors.bin" --out "$tmp/candidates.tsv"
"$tmp/uvw_public_forgery" \
    "$evidence/uvw300.pk.bin" "$tmp/candidates.tsv" 1600 \
    "$tmp/forgery.api.bin" 2>&1 | tee "$log"
cmp "$tmp/forgery.api.bin" "$evidence/uvw300.api-signature.bin"
test "$(wc -c < "$tmp/forgery.api.bin")" -eq 1244
grep -Fq 'recovered_pairs=4850/4850' "$log"
grep -Fq 'fresh_message_forgery_accepted=1 changed_message_rejected=1' "$log"

echo 'ATTACK sign-32-3 CONFIRMED: 300 public signatures recover an equivalent UVW-128 decoder and forge'
