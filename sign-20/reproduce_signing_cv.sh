#!/bin/sh
# sign-20-5 (signing path): one 256-bit SM3 chaining value determines every
# QingLuan-512 round seed of a signature, and with the signature it yields the
# secret exponent vector.
set -u
cd "$(dirname "$0")/.." || exit 2
command -v gcc >/dev/null 2>&1 || { echo 'SKIP sign-20-5: gcc unavailable'; exit 77; }
id=sign-20-5

src="sign-20/Implementations and Test_Vectors/Implementations/Reference_Implementation/QingLuan-512"
grep -q '#define PARAM_HASH_BYTES     128' "$src/include/params.h" &&
grep -q 'sm3hash(256, tmp, (unsigned long long)(PARAM_HASH_BYTES + 4) \* 8, ctx->cache);' "$src/api_pkc/hash_adapter.c" &&
grep -q 'xof_squeeze(&xof, seed_leaves, (size_t)PARAM_TAU \* PARAM_SEED_BYTES);' "$src/src/sign.c" &&
grep -q 'v_exp\[i\] = restr_exp_sub(eta_sk\[i\], eta_prime\[i\]);' "$src/src/mpc.c" ||
    { echo "NOT CONFIRMED $id: QingLuan-512 source does not match the expected XOF and signing path"; exit 1; }
echo "source: SeedLeaves = SM3(K || BE32(i)) with a 128-byte K; unopened rounds publish v = eta - eta'"

scratch=$(mktemp -d) || exit 2
trap 'rm -rf "$scratch"' EXIT HUP INT TERM
cp -rL "$src" "$scratch/ql" || exit 2
cp sign-20/reproduce_signing_cv.c "$scratch/" || exit 2

(
    cd "$scratch/ql" || exit 2
    objs=
    for f in src/fq_arith.c src/rsdp.c src/restr.c src/mpc.c src/keygen.c src/sign.c src/verify.c \
             api_pkc/drng.c api_pkc/auxfunc.c api_pkc/utils_adapter.c api_pkc/hash_adapter.c \
             ../reproduce_signing_cv.c; do
        o=$scratch/$(basename "$f" .c).o
        gcc -O2 -w -DQINGLUAN_512 -Iapi_pkc -Iinclude -c "$f" -o "$o" || exit 1
        objs="$objs $o"
    done
    # shellcheck disable=SC2086
    gcc -o "$scratch/wit" $objs -Wl,--wrap=randombytes || exit 1
) || { echo "NOT CONFIRMED $id: build failed"; exit 1; }

out=$(timeout 120 "$scratch/wit") || { echo "NOT CONFIRMED $id: witness run failed"; exit 1; }
printf '%s\n' "$out"
fail=0
printf '%s\n' "$out" | grep -q 'verify = 0, tau = 1018, opened = 842' || fail=1
printf '%s\n' "$out" | grep -q 'XOF key: 128 bytes = 2 SM3 blocks' || fail=1
printf '%s\n' "$out" | grep -q '^from CV: opened seeds matched 842/842; eta from unopened round: equal to the secret' || fail=1
printf '%s\n' "$out" | grep -q '^control (one CV bit flipped): opened seeds matched 0/842; eta different' || fail=1

if [ "$fail" -eq 0 ]; then
    echo "CONFIRMED $id: a QingLuan-512 signature plus its 256-bit post-key SM3 state regenerates all round seeds and the secret exponent vector; a one-bit-wrong state matches no opened seed"
    exit 0
fi
echo "NOT CONFIRMED $id: a check or control failed"
exit 1
