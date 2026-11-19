#!/bin/sh
# kem-17: two HEP-QC KEM memory defects.
#   (a) hep_qc_pke_encrypt allocates G_mask = calloc(MSG_BIT, VEC_N_SIZE_64*8)
#       (PKE_HEP_QC.c:120) and never frees it, so every kem_enc and every
#       kem_dec (FO re-encryption) leaks one block of 283,648 / 861,696 /
#       1,845,248 / 12,619,776 bytes (HEP-QC-1/3/5/7). Checked in ref and
#       x86_64 by source; each reference set run with malloc/calloc/realloc/
#       free wrapped: key generation leaks nothing (control).
#   (b) kem_dec discards ct_len_bytes, so an honest ciphertext in a heap
#       buffer one byte short, declared at that length, is read past the end
#       by hep_qc_c_kem_from_string under AddressSanitizer (HEP-QC-1);
#       control: the full-length buffer is clean.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-17: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-17: $1"; exit 1; }
drv=$here/reproduce_leak_short_ct.c
I=kem-17/Implementations
C=$I/Implementations
[ -d "$C/ref" ] || bad "implementation tree not found"

for a in ref x86_64; do
    f=$C/$a/PKE_HEP_QC.c
    v=$(sed -n '/^void hep_qc_pke_encrypt/,/^}/p' "$f")
    echo "$v" | grep -q 'G_mask)\[VEC_N_SIZE_64\] = calloc(MSG_BIT, sizeof(\*G_mask));' &&
        ! echo "$v" | grep -q 'free(' &&
        grep -Fq 'hep_qc_pke_encrypt(&c_kem_prime_t.c_pke' "$C/$a/KEM_HEP_QC.c" &&
        grep -Fq '(void)ct_len_bytes;' "$C/$a/KEM_HEP_QC.c" || bad "$a differs from the archive"
done
echo 'source: ref and x86_64 hep_qc_pke_encrypt calloc G_mask without free; kem_dec re-encrypts and discards ct_len_bytes'

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
src="$C/ref/KEM_HEP_QC.c $C/ref/PKE_HEP_QC.c $(ls $C/common/*.c | tr '\n' ' ') $I/Lib/drng/drng.c $I/Lib/auxfunc/auxfunc.c"
for n in 1 3 5 7; do
    inc="-I$C/common -I$C/common/hep-qc-$n -I$C/ref -I$C/ref/hep-qc-$n -I$I/Lib/drng -I$I/Lib/auxfunc"
    gcc -O2 -std=c99 -w -DLEAK -DREPS=3 -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free $inc \
        -o "$scratch/l$n" "$drv" $src 2>"$scratch/cc" || { cat "$scratch/cc"; bad "HEP-QC-$n build failed"; }
    out=$("$scratch/l$n") || bad "HEP-QC-$n run failed"
    g=$(echo "$out" | sed -n 's/^G_mask=\([0-9]*\) .*/\1/p')
    exp="G_mask=$g keygen: +0 blocks +0 bytes; 3 enc: +3 blocks +$((3 * g)) bytes; 3 dec (honest, 3 agree): +3 blocks +$((3 * g)) bytes; 3 dec (tampered): +3 blocks +$((3 * g)) bytes"
    [ -n "$g" ] && [ "$out" = "$exp" ] || { echo "$out"; bad "HEP-QC-$n leak differs"; }
    echo "HEP-QC-$n: $out"
done
echo 'kem-17-8 (a): every kem_enc and kem_dec leaks the G_mask block (283,648 bytes at HEP-QC-1 up to 12,619,776 at HEP-QC-7)'

echo 'int main(void){return 0;}' >"$scratch/p.c"
gcc -fsanitize=address -o "$scratch/p" "$scratch/p.c" 2>/dev/null && "$scratch/p" ||
    { echo 'SKIP kem-17 (b): AddressSanitizer unavailable'; exit 77; }
inc="-I$C/common -I$C/common/hep-qc-1 -I$C/ref -I$C/ref/hep-qc-1 -I$I/Lib/drng -I$I/Lib/auxfunc"
gcc -O1 -g -std=c99 -w -fsanitize=address -fno-omit-frame-pointer $inc -o "$scratch/a" "$drv" $src 2>"$scratch/cc" ||
    { cat "$scratch/cc"; bad "ASan build failed"; }
ASAN_OPTIONS=detect_leaks=0; export ASAN_OPTIONS
full=$("$scratch/a" full 2>&1) || { echo "$full"; bad "full-length control failed"; }
echo "$full" | grep -q '^declared=4433 rc=0 agrees=1$' || { echo "$full"; bad "full-length control differs"; }
short=$("$scratch/a" 4432 2>&1)
echo "$short" | grep -q 'ERROR: AddressSanitizer' && echo "$short" | grep -q 'in hep_qc_c_kem_from_string' &&
    echo "$short" | grep -q 'in kem_dec' && echo "$short" | grep -q 'located 0 bytes after 4432-byte region' ||
    { echo "$short" | head -20; bad "short ciphertext not read past"; }
echo "HEP-QC-1: 4433-byte ciphertext clean; 4432-byte buffer: $(echo "$short" | grep -m1 -o 'READ of size [0-9]*') at $(echo "$short" | grep -m1 -o 'parsing\.c:[0-9]*'), $(echo "$short" | grep -m1 -o 'located 0 bytes after 4432-byte region')"
echo 'ATTACK kem-17-8: every encapsulation and decapsulation leaks the G_mask block, and kem_dec reads past a one-byte-short ciphertext buffer: CONFIRMED'
exit 0
