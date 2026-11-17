#!/bin/sh
# hash-08 (Duet): Duet-768 reference sigma_permutation() reads block[17] of a 17-word stack array.
# Run from the repository root:  sh hash-08/reproduce_sigma_oob.sh
set -eu
cd "$(dirname "$0")/.." || exit 2
for t in gcc; do
    command -v "$t" >/dev/null 2>&1 || { echo "SKIP hash-08-1: $t unavailable"; exit 77; }
done
SRC="hash-08/Duet/Implementations/Reference_Implementation/Duet-768"
T=$(mktemp -d)
trap 'rm -r "$T"' EXIT
cat > "$T/main.c" <<'EOC'
#include <stdio.h>
#include "CryptHash_Duet-768.h"
int main(void) {
  unsigned char msg[3] = {'a', 'b', 'c'}, dig[96];
  int rc = CryptHash(768, msg, 24, dig);
  printf("rc=%d digest[0..3]=%02x%02x%02x%02x\n", rc, dig[0], dig[1], dig[2], dig[3]);
  return 0;
}
EOC
CC=${CC:-cc}
grep -n 'w17 = block\[17\]' "$SRC/CryptHash_Duet-768.c"
grep -n 'WORDS_r = r / WORD_BITS\|^  r = ' "$SRC/CryptHash_Duet-768.c"
"$CC" -std=c99 -O0 -g -fsanitize=address -fno-omit-frame-pointer -I "$SRC" \
    "$T/main.c" "$SRC/CryptHash_Duet-768.c" -o "$T/t" 2>/dev/null
if "$T/t" > "$T/out.txt" 2> "$T/err.txt"; then :; fi
grep -m1 'ERROR: AddressSanitizer' "$T/err.txt" || true
grep -m1 'READ of size' "$T/err.txt" || true
grep -m1 'in sigma_permutation' "$T/err.txt" || true
if grep -q 'stack-buffer-overflow' "$T/err.txt" && grep -q 'sigma_permutation' "$T/err.txt"; then
  echo "ATTACK hash-08-1 Duet-768 reference: 8-byte stack-buffer-overflow READ in sigma_permutation (block[17], array has 17 words) on every absorbed block CONFIRMED"
else
  echo "NOT REPRODUCED (needs a compiler with -fsanitize=address; the dead load is removed at -O1 and above)"
  exit 1
fi
