#!/bin/sh
# kem-01-6: kem_dec ignores ct_len_bytes and parses the ciphertext at its
# fixed length, so a heap buffer of the declared length is overread.
#
# Builds the three reference sets with AddressSanitizer. A full-length heap
# ciphertext must decapsulate cleanly; one a byte short, in a heap buffer of
# exactly that length, must trigger a heap-buffer-overflow read in
# poly_decompress reached from kem_dec. The secret key sits 64 bytes into its
# heap block so the separately filed pre-key read (kem-01-2) does not fire.
#
# Then builds the three AVX2 trees with recoverable AddressSanitizer. There a
# full-length ciphertext in an exactly sized heap buffer is already overread
# by the vector loads in poly_decompress, reached only from kem_dec (key
# generation and encapsulation on exactly sized buffers stay clean). The
# script reports how many bytes past the end the faulting load reaches. The
# same ciphertext in a buffer 16 bytes larger is clean. Skipped (exit 77
# for that part only) when the CPU lacks AVX2, BMI2, POPCNT or AES-NI.
set -u
here=$(cd "$(dirname "$0")" && pwd)
[ -d kem-01/Implementations ] || cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-01-6/7: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-01-6/7: $1"; exit 1; }
ASAN_OPTIONS=detect_leaks=0; export ASAN_OPTIONS

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
echo 'int main(void){return 0;}' >"$scratch/p.c"
gcc -fsanitize=address -o "$scratch/p" "$scratch/p.c" 2>/dev/null && "$scratch/p" ||
    { echo 'SKIP kem-01-6/7: AddressSanitizer unavailable'; exit 77; }
for p in 1 2 3; do
    n=$(echo I II III | cut -d' ' -f$p)
    d=kem-01/Implementations/Reference_Implementation/Aigis-Enc+-$n
    grep -Fq 'return mkem_dec(sk, ct, ss);' "$d/kat_test/KEM_AlgorithmInstance.c" || bad "Aigis-Enc+-$n differs from the archive"
    (cd "$d" && gcc -O1 -g -fsanitize=address -fno-omit-frame-pointer -w -I. -Ikat_test -DPARAMS=$p -DUSE_NICCS_API \
        -o "$scratch/t" "$here/reproduce_short_ct.c" cbd.c fips202.c poly.c reduce.c precomp.c ntt.c polyvec.c \
        owcpa.c kem.c verify.c hashkdf.c gen_a.c pack.c kat_test/auxfunc.c kat_test/KEM_AlgorithmInstance.c \
        kat_test/drng.c kat_test/rng.c) 2>"$scratch/cc" || { cat "$scratch/cc"; bad "Aigis-Enc+-$n build failed"; }
    "$scratch/t" full >"$scratch/full" 2>&1 && grep -q 'rc=0 agrees=1' "$scratch/full" ||
        { cat "$scratch/full"; bad "Aigis-Enc+-$n control failed"; }
    "$scratch/t" short >"$scratch/short" 2>&1 && bad "Aigis-Enc+-$n short ciphertext not detected"
    grep -q 'heap-buffer-overflow' "$scratch/short" && grep -q 'in poly_decompress' "$scratch/short" &&
        grep -q 'in kem_dec' "$scratch/short" || { cat "$scratch/short"; bad "Aigis-Enc+-$n report differs"; }
    echo "Aigis-Enc+-$n: full ciphertext clean; $(grep -o 'located 0 bytes after [0-9]*-byte region' "$scratch/short")"
done
echo 'ATTACK kem-01-6: at all three reference levels a ciphertext one byte short, in a heap buffer of the declared length, is read out of bounds by decapsulation: CONFIRMED'

for f in avx2 bmi2 popcnt aes; do
    grep -qw $f /proc/cpuinfo 2>/dev/null ||
        { echo "SKIP kem-01-7 AVX2 part: CPU lacks $f; the reference result above stands"; exit 0; }
done
ASAN_OPTIONS=detect_leaks=0:halt_on_error=0; export ASAN_OPTIONS
for p in 1 2 3; do
    n=$(echo I II III | cut -d' ' -f$p)
    d=kem-01/Implementations/Optimized_Implementation/avx2/Aigis-Enc+-$n
    grep -Fq 'poly_decompress(v, c + CT_POLYVEC_COMPRESSED_BYTES);' "$d/pack.c" || bad "AVX2 Aigis-Enc+-$n differs from the archive"
    (cd "$d" && gcc -O1 -g -mavx2 -mbmi2 -mpopcnt -maes -Wno-incompatible-pointer-types -fsanitize=address \
        -fsanitize-recover=address -fno-omit-frame-pointer -w -I. -DPARAMS=$p -DUSE_NICCS_API -o "$scratch/v" \
        "$here/reproduce_short_ct.c" cbd.c fips202.c poly.c precomp.c ntt.c polyvec.c owcpa.c kem.c verify.c \
        hashkdf.c pack.c genmatrix.c auxfunc.c KEM_AlgorithmInstance.c drng.c rng.c) 2>"$scratch/cc" ||
        { cat "$scratch/cc"; bad "AVX2 Aigis-Enc+-$n build failed"; }
    "$scratch/v" padded >"$scratch/pad" 2>&1
    grep -q 'ERROR: AddressSanitizer' "$scratch/pad" && { cat "$scratch/pad"; bad "AVX2 Aigis-Enc+-$n padded buffer not clean"; }
    grep -q 'padded: declared=[0-9]* rc=0 agrees=1' "$scratch/pad" || bad "AVX2 Aigis-Enc+-$n padded control failed"
    "$scratch/v" full >"$scratch/full" 2>&1
    grep -q 'full: declared=[0-9]* rc=0 agrees=1' "$scratch/full" || { cat "$scratch/full"; bad "AVX2 Aigis-Enc+-$n full run failed"; }
    [ "$(grep -c 'ERROR: AddressSanitizer: heap-buffer-overflow' "$scratch/full")" = 1 ] &&
        grep -q 'in poly_decompress' "$scratch/full" && grep -q 'in kem_dec' "$scratch/full" &&
        ! grep -Eq 'in (kem_enc|kem_keygen|mkem_enc|mkem_keygen) ' "$scratch/full" ||
        { cat "$scratch/full"; bad "AVX2 Aigis-Enc+-$n report differs"; }
    size=$(sed -n 's/^READ of size \([0-9]*\) at \(0x[0-9a-f]*\).*/\1/p' "$scratch/full")
    addr=$(sed -n 's/^READ of size \([0-9]*\) at \(0x[0-9a-f]*\).*/\2/p' "$scratch/full")
    region=$(sed -n 's/.* \([0-9]*\)-byte region \[\(0x[0-9a-f]*\),\(0x[0-9a-f]*\))$/\1 \3/p' "$scratch/full")
    len=${region% *}; end=${region#* }
    past=$((addr + size - end))
    [ "$past" -gt 0 ] || bad "AVX2 Aigis-Enc+-$n overread not past the end"
    echo "AVX2 Aigis-Enc+-$n: exact $len-byte ciphertext: $size-byte load in poly_decompress from kem_dec reads $past bytes past the end; +16 padding clean"
done
echo 'ATTACK kem-01-7: at all three AVX2 levels decapsulation reads past the end of a full-length, exactly sized heap ciphertext: CONFIRMED'
exit 0
