#!/bin/sh
# kex-03-1, K2K extension: the generic K2K double-key encapsulation absorbs only
# 64 random bits, so weak forward secrecy costs 2^64. Checks all five affected
# twokem.c call sites, then runs a reduced 2^16 search against unmodified
# K2K-ZEN128 and K2K-ZEN512 objects built in a scratch copy.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP kex-03-1: gcc unavailable'; exit 77; }
command -v make >/dev/null 2>&1 || { echo 'SKIP kex-03-1: make unavailable'; exit 77; }

ref=kex-03/Implementations/Reference_Implementation
for site in CreTAKE128/CreTAKE-K2K-PLAC128:PKE_MESSAGE_BYTES CreTAKE256/CreTAKE-K2K-PLAC256:PKE_MESSAGE_BYTES \
            CreTAKE128/CreTAKE-K2K-ZEN128:MSG_LEN_BYTES CreTAKE256/CreTAKE-K2K-ZEN256:MSG_LEN_BYTES \
            CreTAKE512/CreTAKE-K2K-ZEN512:MSG_LEN_BYTES; do
    dir=$ref/${site%%:*}; msg=${site#*:}
    grep -Fq 'get_random_number(&drng_algorithm, buf, SEED_BYTES * 8ULL)' "$dir/twokem.c" || exit 1
    line=$(grep -nF "pseudoXOF(($msg + SEED_BYTES) * 8, buf, SEED_BYTES, buf2);" "$dir/twokem.c" | cut -d: -f1)
    [ -n "$line" ] || { echo "NOT CONFIRMED kex-03-1: call site missing in $dir/twokem.c"; exit 1; }
    echo "source: ${site%%:*}/twokem.c:$line draws SEED_BYTES*8 bits but absorbs SEED_BYTES (=64) bits"
done

scratch=$(mktemp -d) || exit 2
case "$scratch" in /tmp/*) ;; *) echo 'unexpected temporary path' >&2; exit 2 ;; esac
trap 'rm -rf -- "$scratch"' EXIT HUP INT TERM

run() {
    level=$1; zen=$2; lib=$3
    inst=CreTAKE$level/CreTAKE-K2K-ZEN$level
    mkdir -p "$scratch/$level/CreTAKE$level" "$scratch/$level/Common/primitives/kem"
    cp -rL "$ref/$inst" "$scratch/$level/CreTAKE$level/" || exit 2
    cp -rL "$ref/Common/primitives/kem/$zen" "$scratch/$level/Common/primitives/kem/" || exit 2
    d=$scratch/$level/$inst
    make -s -C "$d" KAT_KEX >"$scratch/make-$level.log" 2>&1 || { cat "$scratch/make-$level.log"; exit 2; }
    gcc -O2 -w -no-pie -I"$d" -I"$scratch/$level/Common/primitives/kem/$zen" kex-03/reproduce_k2k_wpfs.c \
        "$d/KEX_AlgorithmInstance.o" "$d/twokem.o" "$d/auxfunc.o" "$d/drng.o" "$d/secure_bzero.o" \
        "$scratch/$level/Common/primitives/kem/$zen/$lib" -Wl,--wrap=get_random_number \
        -o "$scratch/wpfs-$level" || exit 2
    out=$("$scratch/wpfs-$level" 2>&1); rc=$?
    printf '%s\n' "$out" | sed "s/^/K2K-ZEN$level: /"
    [ "$rc" -eq 0 ] &&
    printf '%s\n' "$out" | grep -Fq 'm2 identical=yes, responder key identical=yes' &&
    printf '%s\n' "$out" | grep -Fq 'honest session agrees: yes' &&
    printf '%s\n' "$out" | grep -Fq 'recovered key == session key: yes' &&
    printf '%s\n' "$out" | grep -Fq 'control (wrong hint bit): matches=0' || {
        echo "NOT CONFIRMED kex-03-1: K2K-ZEN$level witness failed"; exit 1; }
}
run 128 ZEN_128 libzen128.a
run 512 ZEN_512 libzen512.a
echo 'CONFIRMED kex-03-1: K2K double-key encapsulation depends on 64 random bits; reduced 2^16 wPFS search recovers K2K-ZEN128 and K2K-ZEN512 session keys, wrong-hint control matches 0'
