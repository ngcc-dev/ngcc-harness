#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
root=$PWD
tmp=$(mktemp -d)
trap 'rm -rf -- "$tmp"' EXIT
for level in 128 256 512; do
    mkdir "$tmp/$level"
    cp -a "Implementations/Reference_Implementation/Sigurd-$level/." "$tmp/$level/"
    cp reproduce_zero_zeta.c "$tmp/$level/"
    python3 - "$tmp/$level/sig_core.c" <<'PY'
import pathlib, sys
p=pathlib.Path(sys.argv[1])
s=p.read_text()
old='*zeta_raw = load_u16_be(stream + (word_index * 2u));'
assert s.count(old)==1
p.write_text(s.replace(old, '*zeta_raw = (uint16_t)(ZETA_FORCE_ZERO ? 0u : load_u16_be(stream + (word_index * 2u)));'))
PY
    (
        cd "$tmp/$level"
        for forced in 1 0; do
            gcc -O2 -DZETA_FORCE_ZERO=$forced reproduce_zero_zeta.c SIG_AlgorithmInstance.c auxfunc.c drng.c rsencode.c rsencode_common.c -o "witness_$forced" -lm
            "./witness_$forced" 5 "public_$forced.txt" "truth_$forced.txt"
            python3 "$root/solve_zero_zeta.py" "public_$forced.txt" > "recovered_$forced.txt" || true
            if [ "$forced" = 1 ]; then
                tail -n 1 "recovered_$forced.txt" | cmp -s - "truth_$forced.txt" || { echo "forced recovery failed at $level"; exit 1; }
                echo "Sigurd-$level forced-zero: exact witness recovery"
            else
                if tail -n 1 "recovered_$forced.txt" | cmp -s - "truth_$forced.txt"; then
                    echo "nonzero control incorrectly recovered witness at $level"; exit 1
                fi
                echo "Sigurd-$level nonzero control: no witness recovery"
            fi
            head -n 1 "recovered_$forced.txt"
        done
    )
done
echo "CONFIRMED sign-24-2: forced-zero witness recovery at all three levels; nonzero controls reject"
