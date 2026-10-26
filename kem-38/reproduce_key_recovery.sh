#!/bin/sh
# UVW-KEM-128 public-key structure recovery and attacker decapsulation.
# The sender builds the frozen reference key generation and encapsulation.
# The attacker reads only the public key, recovers the twin pairing, twin
# ratios and Reed-Solomon description, then decapsulates the ciphertexts.
# The secret key, monomial map and sender secrets are read only for scoring.
# Controls: a second key pair's ciphertext, a flipped c2 bit and a flipped c1 bit.
# About 1.4 GB and one minute on 28 cores; much longer on fewer.
set -eu
cd "$(dirname "$0")/.."
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-38-8: gcc unavailable'; exit 77; }
command -v python3 >/dev/null 2>&1 || { echo 'SKIP kem-38-8: python3 unavailable'; exit 77; }
echo 'int main(void){return 0;}' > "${TMPDIR:-/tmp}/ngcc_omp_$$.c"
if ! gcc -fopenmp "${TMPDIR:-/tmp}/ngcc_omp_$$.c" -o "${TMPDIR:-/tmp}/ngcc_omp_$$" 2>/dev/null; then
    rm -f "${TMPDIR:-/tmp}/ngcc_omp_$$.c"; echo 'SKIP kem-38-8: OpenMP unavailable'; exit 77
fi
rm -f "${TMPDIR:-/tmp}/ngcc_omp_$$.c" "${TMPDIR:-/tmp}/ngcc_omp_$$"
tmp=$(mktemp -d)
trap 'rm -r "$tmp"' EXIT HUP INT TERM
src=kem-38/Implementations/Reference_Implementation/UVW-KEM-128
lib="$src/src/drng.c $src/src/auxfunc.c $src/src/gf_math.c $src/src/list_decoding.c $src/src/uvw_constants.c"
gcc -std=gnu11 -O2 -w -I "$src/include" -I "$src/src" -o "$tmp/sender" kem-38/uvw_sender.c $lib -lm
gcc -std=gnu11 -O2 -w -I "$src/include" -I "$src/src" -o "$tmp/decaps" kem-38/uvw_attack_decaps.c $lib -lm
gcc -std=gnu11 -O3 -march=native -fopenmp -w -o "$tmp/recover" kem-38/uvw_key_recovery.c

ncts=3
mkdir "$tmp/victim"
"$tmp/sender" 1 "$ncts" "$tmp/victim"
OMP_WAIT_POLICY=passive "$tmp/recover" "$tmp/victim/pk.bin" 53 1 8 "$tmp/victim/truth.txt" "$tmp/struct.txt" \
    | grep -E '^(PAIRING|DIFFERENCE|SS:|TOTAL|SCORE pairing)'
ok=0
t=0
while [ "$t" -lt "$ncts" ]; do
    line=$("$tmp/decaps" "$tmp/victim/pk.bin" "$tmp/struct.txt" "$tmp/victim/ct$t.bin" "$tmp/victim/ss$t.bin" || true)
    echo "ct$t: $line" | cut -c1-170
    case "$line" in *"d_check=ok FO_reencrypt(r,e)=ok K==sender_ss: YES"*) ok=$((ok + 1)) ;; esac
    t=$((t + 1))
done
# Control: a second key pair's ciphertext must not decode with this structure.
mkdir "$tmp/other"
"$tmp/sender" 2 1 "$tmp/other"
ctl=$("$tmp/decaps" "$tmp/victim/pk.bin" "$tmp/struct.txt" "$tmp/other/ct0.bin" "$tmp/other/ss0.bin" || true)
echo "control (other key's ciphertext): $ctl" | cut -c1-120
case "$ctl" in *"K==sender_ss: YES"*) echo 'NOT REPRODUCED kem-38-8: control decapsulated'; exit 1 ;; esac
# Controls: one flipped bit of c2 (byte 973) must fail the FO checks, and one
# flipped bit of c1 (byte 1) must fail to decode. The ciphertext is c1 (968
# bytes), c2 (32) and d (32).
flip() { python3 -c 'import sys; b=bytearray(open(sys.argv[1],"rb").read()); b[int(sys.argv[2])]^=int(sys.argv[3]); open(sys.argv[4],"wb").write(b)' "$@"; }
flip "$tmp/victim/ct0.bin" 973 16 "$tmp/c2flip.bin"
flip "$tmp/victim/ct0.bin" 1 128 "$tmp/c1flip.bin"
for c in c2flip c1flip; do
    ctl=$("$tmp/decaps" "$tmp/victim/pk.bin" "$tmp/struct.txt" "$tmp/$c.bin" "$tmp/victim/ss0.bin" || true)
    echo "control ($c): $ctl" | cut -c1-140
    case "$ctl" in *"d_check=ok FO_reencrypt(r,e)=ok"*) echo "NOT REPRODUCED kem-38-8: $c control accepted"; exit 1 ;; esac
done
if [ "$ok" -eq "$ncts" ]; then
    echo "CONFIRMED kem-38-8 UVW-KEM-128: public-key structure recovery decapsulated $ok/$ncts fresh ciphertexts to the sender's shared secret"
else
    echo "NOT REPRODUCED kem-38-8: $ok/$ncts ciphertexts decapsulated"
    exit 1
fi
