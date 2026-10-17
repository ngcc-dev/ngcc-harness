#!/bin/sh
# sign-02-3 BiT-128: 128-bit masking-seed collisions recover an equivalent key.
#
# Builds the submitted BiT-128 reference tree twice in a temporary copy: once
# with seed_y reduced to NGCC_SEED_BITS bits (the only change; default 16) and
# once unmodified (control). Signs distinct messages, finds colliding seeds
# from public verifier commitments, recovers s0, completes an equivalent key
# from pk and forges a fresh message. Default 16 bits / 8192 signatures; NGCC_LONG=1 runs 24 bits / 65536.
set -u
cd "$(dirname "$0")/.." || exit 2
id=sign-02-3
command -v gcc >/dev/null 2>&1 || { echo "SKIP $id: gcc unavailable"; exit 77; }

ref=sign-02/Implementations/Reference_Implementation/BiT-128
[ -f "$ref/sign.c" ] || { echo "NOT CONFIRMED $id: $ref missing"; exit 1; }
grep -q '^#define BIT_SEEDBYTES       16$' "$ref/params.h" ||
    { echo "NOT CONFIRMED $id: BIT_SEEDBYTES is not 16"; exit 1; }
grep -q 'bit_xof256_3(temp_seed_y, BIT_SEEDBYTES' sign-02/Implementations/Optimized_Implementation/BiT-128-avx2/sign.c ||
    { echo "NOT CONFIRMED $id: AVX2 seed_y derivation changed"; exit 1; }

if [ "${NGCC_LONG:-0}" = 1 ]; then bits=${NGCC_SEED_BITS:-24}; nsig=${NGCC_SIGNATURES:-65536}
else bits=${NGCC_SEED_BITS:-16}; nsig=${NGCC_SIGNATURES:-8192}; fi

tmp=$(mktemp -d) || exit 2
trap 'rm -rf -- "$tmp"' EXIT HUP INT TERM
cp -rL "$ref" "$tmp/src" || exit 2
cp sign-02/reproduce_seed_collision.c "$tmp/driver.c" || exit 2

# Insert the reduced-width truncation directly after seed_y is squeezed.
awk '
    { print }
    /bit_xof256_3\(temp_seed_y, BIT_SEEDBYTES,/ { armed = 1 }
    armed && /message_hash, BIT_MESSAGEBYTES\);/ {
        print "#ifdef NGCC_SEED_BITS"
        print "    { /* reduced-width model only: keep NGCC_SEED_BITS bits of seed_y */"
        print "        unsigned kb = NGCC_SEED_BITS / 8, rb = NGCC_SEED_BITS % 8;"
        print "        if (rb) temp_seed_y[kb++] &= (unsigned char)((1u << rb) - 1);"
        print "        memset(temp_seed_y + kb, 0, BIT_SEEDBYTES - kb); }"
        print "#endif"
        armed = 0; n++
    }
    END { if (n != 1) exit 1 }
' "$ref/sign.c" >"$tmp/src/sign.c" || { echo "NOT CONFIRMED $id: patch anchor not found"; exit 1; }

srcs=""
for f in SIG_AlgorithmInstance.c auxfunc.c drng.c fips202.c ntt.c poly.c polyvec.c sample.c sign.c symmetric.c packing.c; do
    srcs="$srcs $tmp/src/$f"
done
cflags="-O2 -std=gnu11 -fwrapv -fno-strict-aliasing -D_GNU_SOURCE -w -I$tmp/src"
# shellcheck disable=SC2086
gcc $cflags -DNGCC_SEED_BITS="$bits" -o "$tmp/reduced" "$tmp/driver.c" $srcs -lm || exit 2
# shellcheck disable=SC2086
gcc $cflags -o "$tmp/full" "$tmp/driver.c" $srcs -lm || exit 2

echo "== reduced-width seed_y: $bits bits, $nsig signatures =="
"$tmp/reduced" "$nsig" >"$tmp/reduced.out" 2>&1; rc1=$?
cat "$tmp/reduced.out"
echo "== control: shipped 128-bit seed_y, same key and DRBG stream, $nsig signatures =="
"$tmp/full" "$nsig" >"$tmp/full.out" 2>&1; rc2=$?
cat "$tmp/full.out"
[ "$rc1" -eq 0 ] && [ "$rc2" -eq 0 ] || { echo "NOT CONFIRMED $id: driver failed ($rc1/$rc2)"; exit 1; }

red=$(grep '^RESULT matches=' "$tmp/reduced.out")
ful=$(grep '^RESULT matches=' "$tmp/full.out")
get() { printf '%s\n' "$1" | tr ' ' '\n' | sed -n "s/^$2=//p"; }
m=$(get "$red" matches); c=$(get "$red" completed); fv=$(get "$red" forgery)
cv=$(get "$red" changed); eqk=$(get "$red" s0_equal); fm=$(get "$ful" matches)
nm=$(grep '^RESULT nonmatching_pair' "$tmp/reduced.out" | sed 's/.*completion=//')

# Measured usable fraction: completed keys over the expected seed collisions.
python3 - "$nsig" "$bits" "$c" <<'EOF' || true
import math, sys
q, w, c = int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3])
exp = q * (q - 1) / 2 / 2**w
print(f"measured: {c} usable pairs / {exp:.1f} expected seed collisions = {c/exp:.3f}")
f = 0.21875   # usable fraction measured over 512 collisions (112 usable)
lam = lambda Q: f * Q * Q / 2**129
print(f"128-bit model with usable fraction {f}: P[success at 2^64] = {1-math.exp(-lam(2**64)):.3f}; "
      f"50% at 2^{64 + 0.5*math.log2(2*math.log(2)/f):.2f}; one expected usable pair at "
      f"2^{64.5 + 0.5*math.log2(1/f):.2f}; expected usable pairs at 2^80 = 2^{math.log2(lam(2**80)):.2f}")
EOF

if [ "${c:-0}" -ge 1 ] && [ "$fv" = 0 ] && [ "$cv" != 0 ] && [ "$eqk" = 1 ] &&
   [ "${fm:-1}" = 0 ] && [ "${nm:-0}" != 0 ]; then
    echo "CONFIRMED $id: $c/$m commitment matches at $bits-bit seed_y gave equivalent BiT-128 keys; fresh-message forgery accepted, changed message rejected, non-matching pair rejected, full-width control 0 matches"
    exit 0
fi
echo "NOT CONFIRMED $id: matches=$m completed=$c forgery=$fv changed=$cv s0_equal=$eqk full_matches=$fm nonmatching=$nm"
exit 1
