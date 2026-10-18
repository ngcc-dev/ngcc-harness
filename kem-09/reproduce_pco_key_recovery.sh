#!/bin/sh
# kem-09 CheetahKEM: full secret-key recovery by a plaintext-checking oracle.
# Escalates kem-09-4 (rejection key bound to the re-encryption) combined with
# kem-09-1 (partial selection mask).  The attack binary is built in a temporary
# directory from pco_cheetah.c and queries only the prebuilt kem-09/lib/*.so
# through the uniform ABI (dlopen: ngcc_seed, kem_keygen, kem_enc, kem_dec).
#
# Run from the repository root:  sh kem-09/reproduce_pco_key_recovery.sh
# Default: recover the smallest set (Cheetah128), reporting a sequential
# run and the parallel fork-based run.  NGCC_LONG=1 recovers all four sets.
set -e
cd "$(dirname "$0")/.." || exit 2
SELF="kem-09"
LIBDIR="$SELF/lib"
SRC="$SELF/pco_cheetah.c"
HDR="$SELF/cheetah_ntt_tables.h"

command -v gcc >/dev/null 2>&1 || { echo "SKIP kem-09-4: gcc not found"; exit 77; }
[ -f "$LIBDIR/libCheetah128.so" ] || { echo "SKIP kem-09-4: $LIBDIR/libCheetah128.so missing (run make -C $SELF)"; exit 77; }

WORKERS=$(nproc 2>/dev/null || echo 4)
[ "$WORKERS" -gt 24 ] && WORKERS=24

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT INT TERM
cp "$SRC" "$TMP/pco_cheetah.c"
cp "$HDR" "$TMP/cheetah_ntt_tables.h"
gcc -O2 -I"$TMP" -o "$TMP/pco" "$TMP/pco_cheetah.c" -ldl || { echo "SKIP kem-09-4: build failed"; exit 77; }

if [ "${NGCC_LONG:-0}" = "1" ]; then
    SETS="Cheetah128 Cheetah256 Cheetah384 Cheetah512"
else
    SETS="Cheetah128"
fi

rc=0
echo "kem-09 CheetahKEM plaintext-checking-oracle key recovery (workers=$WORKERS; NGCC_LONG=1 for all four sets)"
# smallest set: report both variants
echo "-- sequential variant (single process) --"
"$TMP/pco" "$LIBDIR/libCheetah128.so" Cheetah128 sequential 1 || rc=1
echo "-- parallel static variant --"
for s in $SETS; do
    "$TMP/pco" "$LIBDIR/lib$s.so" "$s" static "$WORKERS" || rc=1
done

if [ "$rc" = "0" ]; then
    echo "CONFIRMED kem-09-4: plaintext-checking oracle recovers the complete CheetahKEM decryption secret"
    exit 0
fi
echo "NOT CONFIRMED kem-09-4: see per-set lines above"
exit 1
