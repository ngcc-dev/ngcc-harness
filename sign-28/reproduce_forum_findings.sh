#!/bin/sh
set -eu

COMMIT=709d5ec64239206effb2667ca853ddcef3c060b4
POC_SHA256=4e666aedf96dd8a4642ee23024c64c6cb4e691cd08547eb504e98b805163a0b9
ARCHIVE_SHA256=c4ae5b27a188612cd286a786179ce5461e6aca0b8599ba13b8d77b210a940adb
ARCHIVE_URL='https://www.niccs.org.cn/niccs/Proposal/Public-Key%20Cryptographic%20Algorithms/Round%201%20candidates/SYDO.zip'
MODE=${1:-fast}
TMP=$(mktemp -d /tmp/ngcc-sydo.XXXXXX)
trap 'rm -rf -- "$TMP"' EXIT HUP INT TERM

if [ "$MODE" = full ] && { [ -z "${CC_OPT:-}" ] || [ -z "${CXX_OPT:-}" ]; }; then
    for pair in gcc:g++ clang-21:clang++-21 clang-20:clang++-20 clang-19:clang++-19 clang-18:clang++-18 clang-17:clang++-17 clang:clang++; do
        old_ifs=$IFS
        IFS=:
        set -- $pair
        IFS=$old_ifs
        if command -v "$1" >/dev/null 2>&1 && command -v "$2" >/dev/null 2>&1 &&
           printf 'int main(void){return 0;}\n' | "$1" -x c -std=c23 -march=native -c -o /dev/null - >/dev/null 2>&1 &&
           printf '#include <cstddef>\ntemplate<class T> void f(){static_assert(false);}\nint main(){}\n' |
               "$2" -x c++ -std=c++23 -march=native -c -o /dev/null - >/dev/null 2>&1; then
            CC_OPT=${CC_OPT:-$1}
            CXX_OPT=${CXX_OPT:-$2}
            break
        fi
    done
fi
if [ "$MODE" = full ]; then
    : "${CC_OPT:?a native C23 compiler is required for the optimized implementation}"
    : "${CXX_OPT:?a matching C++23 compiler is required for the optimized implementation}"
else
    CC_OPT=${CC_OPT:-cc}
    CXX_OPT=${CXX_OPT:-c++}
fi

if ! curl -fsSL "$ARCHIVE_URL" -o "$TMP/SYDO.zip"; then
    echo 'SKIP sign-28-1/sign-28-2: archived submission is unavailable' >&2
    exit 77
fi
printf '%s  %s\n' "$ARCHIVE_SHA256" "$TMP/SYDO.zip" | sha256sum -c -
unzip -q "$TMP/SYDO.zip" -d "$TMP/submission"

if ! curl -fsSL "https://codeload.github.com/acprk/ngcc-round1-cryptanalysis/tar.gz/$COMMIT" \
    -o "$TMP/source.tar.gz"; then
    echo 'SKIP sign-28-1/sign-28-2: pinned external artifact is unavailable' >&2
    exit 77
fi
printf '%s  %s\n' "$POC_SHA256" "$TMP/source.tar.gz" | sha256sum -c -
tar -xzf "$TMP/source.tar.gz" -C "$TMP"
POC="$TMP/ngcc-round1-cryptanalysis-$COMMIT/sydo-grinding-and-padding"
RUN_LOG="$TMP/run.log"

# Do not allow run_all.sh to fall back to the package's committed result files.
mv "$POC/results" "$POC/published-results"
mkdir "$POC/results"

REF="$TMP/submission" WORK="$TMP/work" SETS="${SETS:-160s 160f 256s 256f 512s 512f}" \
    CC_REF="${CC_REF:-gcc}" CC_OPT="$CC_OPT" CXX_OPT="$CXX_OPT" \
    "$POC/run_all.sh" "$MODE" >"$RUN_LOG" 2>&1
cat "$RUN_LOG"

if [ "$MODE" = full ]; then
    grep -Fq 'ref: verify(sig.bin)=0 verify(sig_mut.bin)=0' "$POC/results/padding_crossverify.log"
    grep -Fq 'opt: verify(sig.bin)=0 verify(sig_mut.bin)=-1' "$POC/results/padding_crossverify.log"
    grep -Eq 'ref: RANGE .* accepts=1120 ' "$RUN_LOG"
    grep -Eq 'opt: RANGE .* accepts=0 ' "$RUN_LOG"
    echo 'ATTACK sign-28-2 SYDO CONFIRMED reference accepts nonzero BAVC padding while optimized rejects'
else
    for set_name in ${SETS:-160s 160f 256s 256f 512s 512f}; do
        test -x "$TMP/work/ref_$set_name/layout"
        layout=$(grep -F "sydo_$set_name " "$POC/results/layouts.txt")
        zero_bits=$(printf '%s\n' "$layout" | sed -n 's/.*zero_bits_param=\([0-9][0-9]*\).*/\1/p')
        enforced=$(printf '%s\n' "$layout" | sed -n 's/.*enforced_zero_bits=\([0-9][0-9]*\).*/\1/p')
        test -n "$zero_bits" && test -n "$enforced"
        test "$enforced" -eq "$((zero_bits - 2))"
    done
    grep -Fq 'satisfy spec grinding condition:' "$POC/results/kat_grind.log"
    grep -Fq 'cheat on [3, 77, 150, 201]' "$POC/results/qs_cheat.log"
    echo 'ATTACK sign-28-1 SYDO CONFIRMED implementations enforce two fewer grinding bits'
fi
