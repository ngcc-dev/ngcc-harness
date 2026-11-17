#!/bin/sh
# hash-28 (XRH-1): reference CryptHash() returns a message-independent constant digest for every
# msg_len_bits in [2^64 - r, 2^64 - 1]: the block-count expression (msg_len_bits + rate) / rate wraps to 0,
# so nothing is absorbed (not even the padding) and the message buffer is never read.
# Run from the repository root:  sh hash-28/reproduce_lenwrap.sh
set -e
cd "$(dirname "$0")/.." || exit 2
for t in cc gcc; do
    command -v "$t" >/dev/null 2>&1 || { echo "SKIP hash-28-1: $t unavailable"; exit 77; }
done
ROOT=$(pwd)
REF="$ROOT/hash-28/XRH-1/Implementations/Reference_Implementation"
T=$(mktemp -d)
trap 'rm -r "$T"' EXIT
cat > "$T/w.c" <<'CEOF'
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
int CryptHash(int, const unsigned char *, unsigned long long, unsigned char *);
int main(void) {
    /* a PROT_NONE page as "message": any read of the message would fault */
    unsigned char *na = mmap(NULL, (size_t)sysconf(_SC_PAGESIZE), PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    unsigned char d[4][DIG / 8], e[DIG / 8];
    unsigned long long top = ~0ULL, len[4];
    int i, same = 1, zero = 1;
    len[0] = top; len[1] = top - 1; len[2] = top - RATE / 2; len[3] = top - RATE + 1;
    for (i = 0; i < 4; i++) CryptHash(DIG, na, len[i], d[i]);
    CryptHash(DIG, (const unsigned char *)"", 0, e);
    for (i = 1; i < 4; i++) if (memcmp(d[0], d[i], DIG / 8)) same = 0;
    for (i = 0; i < (RATE < DIG ? RATE : DIG) / 8; i++) if (d[0][i]) zero = 0;
    printf("  n=%d r=%d: digest(2^64-1) = ", DIG, RATE);
    for (i = 0; i < 16; i++) printf("%02x", d[0][i]);
    printf("...  identical for 4 lengths in [2^64-r, 2^64-1]: %s; first min(r,n) bits zero: %s; differs from H(empty): %s\n",
           same ? "yes" : "no", zero ? "yes" : "no", memcmp(d[0], e, DIG / 8) ? "yes" : "no");
    return !(same && zero);
}
CEOF
ok=1
for n in 512 768 1024; do
    r=$((1280 - n - 64))
    cc -O2 -std=gnu99 -DDIG=$n -DRATE=$r -I"$REF/XRH-1-$n" "$T/w.c" "$REF/XRH-1-$n/CryptHash_AlgorithmInstance.c" -o "$T/w$n"
    "$T/w$n" || ok=0
done
if [ $ok = 1 ]; then
    echo "ATTACK hash-28-1 (reference CryptHash: constant digest, message unread, for msg_len_bits >= 2^64 - r) CONFIRMED"
else
    echo "ATTACK hash-28-1 NOT reproduced"; exit 1
fi
