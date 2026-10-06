#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
archive="$scratch/Tins.zip"
if [ -n "${TINS_ARCHIVE:-}" ]; then
    cp -- "$TINS_ARCHIVE" "$archive"
else
    if ! curl -fsSL --max-time 40 \
        'https://www.niccs.org.cn/niccs/Proposal/Public-Key%20Cryptographic%20Algorithms/Round%201%20candidates/Tins.zip' \
        -o "$archive"; then
        echo 'SKIP sign-29-3: official archive unavailable'
        exit 77
    fi
fi
printf '%s  %s\n' 84affc1f7cbdeec48a7cb6df3349b5708644672f2ac12140c529e74f6d57ca21 "$archive" | sha256sum -c - >/dev/null
unzip -qq "$archive" 'tins/Implementations/Reference_Implementation/Tins128/*' -d "$scratch"
src="$scratch/tins/Implementations/Reference_Implementation/Tins128"
compiler=${CC:-/usr/bin/clang}
if [ ! -x "$compiler" ]; then compiler=cc; fi
"$compiler" -O1 -g -fsanitize=address -fno-omit-frame-pointer -fno-stack-protector -std=c99 \
    -I"$src" "$root/reproduce_path_overflow.c" \
    "$src/SIG_TINS128.c" "$src/bavc_commit.c" "$src/ff_arith.c" \
    "$src/auxfunc.c" "$src/drng.c" -o "$scratch/check"
ASAN_OPTIONS=detect_leaks=0 "$scratch/check" >"$scratch/control.out" 2>"$scratch/control.err"
if ASAN_OPTIONS=detect_leaks=0:exitcode=91 OVERLONG=1 "$scratch/check" \
    >"$scratch/attack.out" 2>"$scratch/attack.err"; then
    echo 'NOT CONFIRMED sign-29-3: overlong input did not trigger sanitizer' >&2
    exit 1
fi
grep -q 'AddressSanitizer: stack-buffer-overflow' "$scratch/attack.err"
grep -q 'SIG_TINS128.c:365' "$scratch/attack.err"
echo 'CONFIRMED sign-29-3: oversized signature overwrites the Tins128 verifier stack path'
