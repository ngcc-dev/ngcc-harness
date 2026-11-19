#!/bin/sh
# kem-39: Weaver BCH decoder stack-array bounds and kem_dec's unchecked
# ct_len_bytes. Builds the three reference sets (sources unmodified unless
# stated) with reproduce_bch_bounds.c.
#
# (1) compute_error_locator_polynomial indexes elp->c[j+k] up to 3t-2 in a
#     (2t+1)-entry array. AddressSanitizer reports a stack-buffer-overflow
#     READ on honest decapsulation for WeaverKEM-256 and -512 (not -128). An
#     observation copy (one hook call added before the statement, -O2) shows
#     that every out-of-range access XORs 0 (honest, crafted, 400 random
#     ciphertexts, and decoder sweeps over all 2^24 WeaverKEM-256 low-layer
#     remainders or 2^22 random WeaverKEM-512 ones), and a symbolic sweep of
#     all 2^t discrepancy patterns gives 2t-1 as the largest index that can
#     change.
# (2) chien_search stores roots[count] = n-i every step, count <= t, into
#     errloc[t]. A ciphertext with u = 0 and v placing exactly t errors in a
#     BCH codeword makes decapsulation store 4 bytes past errloc[] (ASan
#     stack-buffer-overflow WRITE) at all three levels; t-1 errors do not.
#     With clang -O2 the slot past errloc[] in decode_bch_high holds the
#     spilled data pointer: WeaverKEM-128 decapsulation of such a ciphertext
#     dies with SIGSEGV, while the same build with errloc[t+1] and with gcc
#     -O2/-O3 survive (clang part skipped if clang is missing).
# (3) kem_dec discards ct_len_bytes: an honest ciphertext in a heap buffer
#     one byte short, declared at that length, is read out of bounds under
#     ASan; the full-length buffer is clean.
set -u
here=$(cd "$(dirname "$0")" && pwd) || exit 2
cd "$here/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kem-39: gcc unavailable'; exit 77; }
bad() { echo "NOT CONFIRMED kem-39: $1"; exit 1; }
drv=$here/reproduce_bch_bounds.c
impl=kem-39/Implementations
ref=$impl/Reference_Implementation
[ -d "$ref/WeaverKEM-128" ] || bad "reference tree not found"

scratch=$(mktemp -d) || exit 2
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM
echo 'int main(void){return 0;}' >"$scratch/p.c"
gcc -fsanitize=address -o "$scratch/p" "$scratch/p.c" 2>/dev/null && "$scratch/p" ||
    { echo 'SKIP kem-39: AddressSanitizer unavailable'; exit 77; }

# source: the 12 decoder copies are identical, and the 6 wrappers drop ct_len_bytes
for f in bch_low.c bch_high.c; do
    for d in "$ref"/WeaverKEM-* "$impl"/Optimized_Implementation/WeaverKEM-*; do
        cmp -s "$d/$f" "$ref/WeaverKEM-128/$f" || bad "$d/$f differs from the reference copy"
    done
done
grep -Fq 'elp->c[j+k] ^= (tmp_c&mask_d&mask_pelp&mask_deg);' "$ref/WeaverKEM-128/bch_low.c" &&
    grep -Fq 'roots[count] = n-i;' "$ref/WeaverKEM-128/bch_low.c" &&
    grep -Fq 'unsigned int errloc[BCH_T];' "$ref/WeaverKEM-128/bch_low.c" || bad "decoder differs from the archive"
for d in "$ref"/WeaverKEM-* "$impl"/Optimized_Implementation/WeaverKEM-*; do
    grep -Fq '(void)ct_len_bytes;' "$d"/KEM_WeaverKEM-*.c || bad "$d wrapper checks ct_len_bytes"
done
echo 'source: the 12 bch_low.c/bch_high.c copies are identical; all 6 kem_dec wrappers discard ct_len_bytes'

src='drng.c auxfunc.c symmetric-iccs.c kem_cca.c indcpa.c polyvec.c poly.c ntt.c cbd.c reduce.c verify.c msgenc.c bch_high.c bch_low.c poly_invq.c'
build() { # dir mode out compiler flags...
    bd=$1 bm=$2 bo=$3 bc=$4; shift 4
    (cd "$bd" && $bc "$@" -w -DWEAVER_MODE=$bm -I. -o "$bo" "$drv" $src KEM_WeaverKEM-*.c -lm) \
        >"$scratch/cc" 2>&1 || { cat "$scratch/cc"; bad "build of $bo failed"; }
}
ASAN_OPTIONS=detect_leaks=0:halt_on_error=0; export ASAN_OPTIONS
for s in 128:1:2:10 256:3:4:24 512:5:7:49; do
    n=${s%%:*}; r=${s#*:}; m=${r%%:*}; r=${r#*:}; t=${r%%:*}; eb=${r#*:}
    d="$ref/WeaverKEM-$n"
    build "$d" $m "$scratch/a$n" gcc -O1 -g -fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer

    # (1) honest decapsulation
    "$scratch/a$n" honest 3 >"$scratch/h" 2>&1
    grep -q '^honest agree=3/3$' "$scratch/h" || { cat "$scratch/h"; bad "WeaverKEM-$n honest run failed"; }
    reads=$(grep -A2 'ERROR: AddressSanitizer: stack-buffer-overflow' "$scratch/h" | grep -c 'in compute_error_locator_polynomial')
    grep -A2 'ERROR: AddressSanitizer: stack-buffer-overflow' "$scratch/h" | grep -q '^WRITE' &&
        bad "WeaverKEM-$n honest run has an out-of-bounds write report"
    sites=$(grep -A2 'ERROR: AddressSanitizer' "$scratch/h" | grep -o 'bch_[a-z]*\.c:[0-9]*' | sort -u | tr '\n' ' ')
    if [ $n = 128 ]; then [ "$reads" = 0 ] || bad "WeaverKEM-128 honest run reports elp overflow"
    else [ "$reads" -ge 1 ] || { cat "$scratch/h"; bad "WeaverKEM-$n honest run: no elp report"; }; fi
    echo "WeaverKEM-$n honest: 3/3 agree; ASan elp stack-buffer-overflow READ reports: $reads ${sites:+at $sites}"

    # (2) crafted ciphertexts: t errors vs t-1 errors in the low codeword (and high for 128/512)
    for lay in crafted crafted-high; do
        [ $n = 256 ] && [ $lay = crafted-high ] && continue   # -256 never runs the high decoder (kem-39-2)
        for e in $((t - 1)) $t; do
            "$scratch/a$n" $lay $e >"$scratch/c" 2>&1
            grep -q "^$lay errors=$e rc=0$" "$scratch/c" || { cat "$scratch/c"; bad "WeaverKEM-$n $lay $e run failed"; }
            w=$(grep -A3 '^WRITE of size 4' "$scratch/c" | grep -c 'in chien_search')
            if [ $e = $t ]; then
                [ "$w" -ge 1 ] && grep -q 'stack-buffer-overflow' "$scratch/c" || { cat "$scratch/c"; bad "WeaverKEM-$n $lay $t errors: no errloc write"; }
                echo "WeaverKEM-$n $lay, $t errors: ASan stack-buffer-overflow WRITE of size 4 at $(grep -A3 '^WRITE of size 4' "$scratch/c" | grep -m1 -o 'bch_[a-z]*\.c:[0-9]*')"
            else
                [ "$w" = 0 ] || bad "WeaverKEM-$n $lay $e errors also writes past errloc"
            fi
        done
    done

    # (3) short ciphertext
    "$scratch/a$n" short >"$scratch/s" 2>&1
    grep -q 'heap-buffer-overflow' "$scratch/s" && grep -q '0 bytes after' "$scratch/s" &&
        grep -q 'in kem_dec' "$scratch/s" || { cat "$scratch/s"; bad "WeaverKEM-$n short ciphertext not read past"; }
    cl=$(sed -n 's/^declared=\([0-9]*\) .*/\1/p' "$scratch/s")
    "$scratch/a$n" short $((cl + 1)) >"$scratch/f" 2>&1
    grep -q 'heap-buffer-overflow' "$scratch/f" && { cat "$scratch/f"; bad "WeaverKEM-$n full-length control not clean"; }
    grep -q "declared=$((cl + 1)) rc=0 agrees=1" "$scratch/f" || bad "WeaverKEM-$n full-length control failed"
    echo "WeaverKEM-$n short: $(grep -m1 -o 'READ of size [0-9]*' "$scratch/s") $(grep -m1 -o 'located 0 bytes after [0-9]*-byte region' "$scratch/s"); $((cl + 1))-byte control has no heap overflow"

    # observation copy: hooks only record index/value before the original statements
    cp -R "$d" "$scratch/o$n" || bad "copy failed"
    for f in bch_low bch_high; do
        tag=0; [ $f = bch_high ] && tag=1
        sed -i -e '1i void obs_elp(int, unsigned, int); void obs_root(int, unsigned, unsigned, unsigned);' \
            -e 's/^\( *\)elp->c\[j+k\] ^= (tmp_c&mask_d&mask_pelp&mask_deg);/\1obs_elp(j+k, (tmp_c\&mask_d\&mask_pelp\&mask_deg), t); &/' \
            -e "s/^\( *\)roots\[count\] = n-i;/\1obs_root($tag, count, n-i, t); &/" "$scratch/o$n/$f.c"
        [ "$(grep -c 'obs_' "$scratch/o$n/$f.c")" = 4 ] || bad "instrumentation of $f.c failed"
    done
    build "$scratch/o$n" $m "$scratch/o$n/x" gcc -O2 -DOBSERVE
    "$scratch/o$n/x" observe >"$scratch/obs" || bad "WeaverKEM-$n observation run failed"
    sed 's/^/    /' "$scratch/obs"
    grep -q '^honest: agree=200/200 elp_oob_accesses=[0-9]* nonzero_oob_xors=0 roots\[t\]_stores=0$' "$scratch/obs" &&
        grep -q "nonzero_oob_xors=0; decoder sweep" "$scratch/obs" &&
        grep -q "final_value_low=$eb\.\.$eb (ecc_bits=$eb)" "$scratch/obs" &&
        grep -q " e=$((t - 1)):roots\[t\]=0 e=$t:roots\[t\]=1$" "$scratch/obs" || bad "WeaverKEM-$n observation differs"
    "$scratch/o$n/x" enum >"$scratch/en"
    grep -q "^t=$t valid=0..$((2 * t)) max_index_touched=$((3 * t - 2)) max_index_changed=$((2 * t - 1))$" "$scratch/en" ||
        { cat "$scratch/en"; bad "WeaverKEM-$n symbolic sweep differs"; }
    echo "    symbolic: $(cat "$scratch/en")"
done
echo 'kem-39-9 (1): WeaverKEM-256/-512 decapsulation reads elp->c past its stack array on every honest call and writes the same value back; no ciphertext changes those bytes'

# clang -O2: what errloc[t] overwrites in WeaverKEM-128's decode_bch_high
d="$ref/WeaverKEM-128"
cp -R "$d" "$scratch/big" && sed -i 's/unsigned int errloc\[BCH_T\];/unsigned int errloc[BCH_T+1];/' "$scratch/big/bch_high.c" ||
    bad "oracle copy failed"
for o in -O2 -O3; do
    build "$d" 1 "$scratch/g$o" gcc $o
    "$scratch/g$o" crafted-high 2 >/dev/null 2>&1 || bad "gcc $o WeaverKEM-128 control failed"
done
echo 'gcc -O2 and -O3: WeaverKEM-128 decapsulates the 2-error high-codeword ciphertext without fault'
cl=
for c in clang clang-21 clang-20 clang-19 clang-18 /usr/bin/clang; do
    command -v $c >/dev/null 2>&1 && $c -O2 -o "$scratch/p" "$scratch/p.c" 2>/dev/null && "$scratch/p" 2>/dev/null && { cl=$c; break; }
done
if [ -n "$cl" ]; then
    build "$d" 1 "$scratch/k" $cl -O2
    build "$scratch/big" 1 "$scratch/kb" $cl -O2
    "$scratch/k" crafted-high 1 >/dev/null 2>&1 || bad "clang -O2 1-error control failed"
    "$scratch/kb" crafted-high 2 >/dev/null 2>&1 || bad "clang -O2 errloc[t+1] control failed"
    "$scratch/k" crafted-high 2 >/dev/null 2>&1; rc=$?
    [ $rc -ge 128 ] || bad "clang -O2 WeaverKEM-128 survived the 2-error ciphertext (rc=$rc)"
    echo "$($cl --version | head -n 1) -O2: WeaverKEM-128 kem_dec on the 2-error high-codeword ciphertext dies with signal $((rc - 128)); 1 error and the errloc[BCH_T+1] build return 0"
else
    echo 'SKIP clang part: clang unavailable; the ASan write above stands'
fi
echo 'ATTACK kem-39-8: a ciphertext with exactly t errors in a BCH codeword makes chien_search store 4 bytes past errloc[] at all three levels: CONFIRMED'
echo 'ATTACK kem-39-9: decapsulation reads elp->c past its stack array on every honest WeaverKEM-256/-512 call, and kem_dec reads past a one-byte-short ciphertext buffer: CONFIRMED'
exit 0
