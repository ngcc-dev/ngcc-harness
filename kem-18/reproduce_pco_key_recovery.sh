#!/bin/sh
# kem-18 LoongKEM: full secret-key recovery by a plaintext-checking oracle.
# Escalates kem-18-4 (rejection key bound to the re-encryption) combined with
# kem-18-1 (partial selection mask).  The attack binary is built in a temporary
# directory from pco_loong.c and queries only the prebuilt kem-18/lib/*.so
# through the uniform ABI (dlopen: ngcc_seed, kem_keygen, kem_enc, kem_dec).
#
# Run from the repository root:  sh kem-18/reproduce_pco_key_recovery.sh
# Default: recover the smallest set (Loong128), reporting a sequential
# run and the parallel fork-based run.  NGCC_LONG=1 recovers all four sets.
set -e
cd "$(dirname "$0")/.." || exit 2
SELF="kem-18"
LIBDIR="$SELF/lib"
SRC="$SELF/pco_loong.c"

command -v gcc >/dev/null 2>&1 || { echo "SKIP kem-18-4: gcc not found"; exit 77; }
[ -f "$LIBDIR/libLoong128.so" ] || { echo "SKIP kem-18-4: $LIBDIR/libLoong128.so missing (run make -C $SELF)"; exit 77; }

WORKERS=$(nproc 2>/dev/null || echo 4)
[ "$WORKERS" -gt 24 ] && WORKERS=24

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT INT TERM
cp "$SRC" "$TMP/pco_loong.c"
gcc -O2 -o "$TMP/pco" "$TMP/pco_loong.c" -ldl || { echo "SKIP kem-18-4: build failed"; exit 77; }

if [ "${NGCC_LONG:-0}" = "1" ]; then
    SETS="Loong128 Loong256 Loong384 Loong512"
else
    SETS="Loong128"
fi

rc=0
echo "kem-18 LoongKEM plaintext-checking-oracle key recovery (workers=$WORKERS; NGCC_LONG=1 for all four sets)"
# smallest set: report both variants
echo "-- sequential variant (single process) --"
"$TMP/pco" "$LIBDIR/libLoong128.so" Loong128 sequential 1 || rc=1
echo "-- parallel static variant --"
for s in $SETS; do
    "$TMP/pco" "$LIBDIR/lib$s.so" "$s" static "$WORKERS" || rc=1
done

if [ "$rc" = "0" ]; then
    echo "CONFIRMED kem-18-4: plaintext-checking oracle recovers the complete LoongKEM decryption secret"
    exit 0
fi
echo "NOT CONFIRMED kem-18-4: see per-set lines above"
exit 1
