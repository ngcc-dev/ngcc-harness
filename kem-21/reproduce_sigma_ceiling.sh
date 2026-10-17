#!/bin/sh
# kem-21-2: the Viper-384/512 encapsulation secret r is derived from a 256-bit
# sigma, so (pk, ct) and a 2^256 sigma enumeration give the shared key.
# The victim side is the prebuilt kem-21/lib/*.so; the attacker side is built
# from an unmodified copy of the submitted reference sources in a temporary
# directory. NGCC_LONG=1 widens the reduced search from 2^16 to 2^20 candidates (about 15 minutes).
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-21-2: gcc unavailable'; exit 77; }

ref=kem-21/Implementations/Reference_Implementation
bits=16
[ "${NGCC_LONG:-0}" = 1 ] && bits=20
trials=20

scratch=$(mktemp -d) || exit 2
case "$scratch" in /tmp/*) ;; *) echo 'unexpected temporary path' >&2; exit 2 ;; esac
trap 'rm -rf "$scratch"' EXIT HUP INT TERM

# The sampler and KEM sources are shared by every reference and optimized tree.
for level in 128 192 256 512; do
    cmp -s "$ref/MAMBA-Viper-384/viper.c" "$ref/MAMBA-Viper-$level/viper.c" || { echo "NOT CONFIRMED kem-21-2: viper.c differs at $level"; exit 1; }
    cmp -s "$ref/MAMBA-Viper-384/kem.c" "$ref/MAMBA-Viper-$level/kem.c" || { echo "NOT CONFIRMED kem-21-2: kem.c differs at $level"; exit 1; }
done
for level in 128 192 256 384 512; do
    opt=kem-21/Implementations/Optimized_Implementation/MAMBA-Viper-$level
    cmp -s "$ref/MAMBA-Viper-384/kem.c" "$opt/kem.c" || { echo "NOT CONFIRMED kem-21-2: optimized kem.c differs at $level"; exit 1; }
    [ "$(grep -c 'shake256(buf, sizeof(buf), seed, 32);' "$opt/viper.c")" = 2 ] || { echo "NOT CONFIRMED kem-21-2: optimized sampler differs at $level"; exit 1; }
done
[ "$(grep -c 'shake256(buf, sizeof(buf), seed, 32);' "$ref/MAMBA-Viper-384/viper.c")" = 2 ] || { echo 'NOT CONFIRMED kem-21-2: reference sampler changed'; exit 1; }
echo 'source: viper_sample_secret absorbs 32 seed bytes in all ten trees; kem.c identical in all ten'

confirmed=0
for level in 256 384 512; do
    lib=kem-21/lib/libMAMBA-Viper-$level.so
    [ -f "$lib" ] || { echo "SKIP kem-21-2: $lib missing (build it: make -C kem-21)"; exit 77; }
    src=$scratch/src$level
    cp -rL "$ref/MAMBA-Viper-$level" "$src" || exit 2
    gcc -O2 -std=gnu11 -DVIPER_LEVEL=$level -DVIPER_USE_E8_CODEC=1 -I"$src" \
        kem-21/reproduce_sigma_ceiling.c "$src/kem.c" "$src/viper.c" "$src/viper_arith.c" \
        "$src/viper_e8.c" "$src/viper_message_codec.c" "$src/verify.c" "$src/drng.c" \
        "$src/auxfunc.c" "$src/viper_api_rng.c" "$src/viper_api_hash.c" \
        -o "$scratch/h$level" -ldl -lm 2>"$scratch/cc$level.log" || { cat "$scratch/cc$level.log"; exit 2; }
    out=$("$scratch/h$level" "$PWD/$lib" "$bits" "$trials")
    rc=$?
    printf '%s\n' "$out"
    [ "$rc" -eq 0 ] || { echo "NOT CONFIRMED kem-21-2: harness failed at Viper-$level (rc=$rc)"; exit 1; }
    case $level in
        256) printf '%s\n' "$out" | grep -q 'sigma_bits=256 target_bits=256 PASS' || { echo 'NOT CONFIRMED kem-21-2: Viper-256 control failed'; exit 1; }
             echo 'control: at Viper-256 the 2^256 sigma search equals the target, so it is not a finding there' ;;
        *)   printf '%s\n' "$out" | grep -q "sigma_bits=256 target_bits=$level PASS" && confirmed=$((confirmed + 1)) ;;
    esac
done

if [ "$confirmed" -eq 2 ]; then
    echo "CONFIRMED kem-21-2: Viper-384/512 r is a function of a 256-bit sigma; K recovered from (pk, ct, sigma[0..31]) $trials/$trials at each level, omega tail ignored, wrong prefix rejected"
    exit 0
fi
echo 'NOT CONFIRMED kem-21-2'
exit 1
