#!/bin/sh
# hash-08 (Duet): for digest_len_bits > h the shipped code squeezes c bits per step instead of r' = h
# (spec 2.1, 2.5, Algorithm 1 lines 1/16/19/22), so its output differs from the specified XOF from bit h on
# and exposes the c-h capacity bits the specification keeps hidden.
# Run from the repository root:  sh hash-08/reproduce_xof_rate.sh
set -eu
cd "$(dirname "$0")/.." || exit 2
for t in gcc python3; do
    command -v "$t" >/dev/null 2>&1 || { echo "SKIP hash-08-2: $t unavailable"; exit 77; }
done
IMPL="hash-08/Duet/Implementations"
T=$(mktemp -d)
trap 'rm -r "$T"' EXIT
CC=${CC:-cc}
cat > "$T/main.c" <<'EOC'
#include <stdio.h>
int CryptHash(int, const unsigned char *, unsigned long long, unsigned char *);
int main(void) {
  unsigned char msg[3] = {'a', 'b', 'c'}, dig[1024];
  int nu = 2 * H, rc = CryptHash(nu, msg, 24, dig);
  if (rc) return 1;
  for (int i = 0; i < nu / 8; i++) printf("%02x", dig[i]);
  printf("\n");
  return 0;
}
EOC
cat > "$T/spec.py" <<'EOP'
# Duet written from the specification text only (S-box Table 1, M15/M30, Tables 3/4, Algorithm 1 and 2).
import sys
M = (1 << 64) - 1
SB = [0x1A,0x09,0x10,0x17,0x07,0x08,0x15,0x0E,0x19,0x1F,0x1E,0x0C,0x1B,0x01,0x04,0x0A,
      0x05,0x13,0x0D,0x0F,0x16,0x1C,0x06,0x18,0x11,0x12,0x14,0x03,0x1D,0x02,0x00,0x0B]
PI = [0x243f6a8885a308d3,0x13198a2e03707344,0xa4093822299f31d0,0x082efa98ec4e6c89,0x452821e638d01377,0xbe5466cf34e90c6c,
      0xc0ac29b7c97c50dd,0x3f84d5b5b5470917,0x9216d5d98979fb1b,0xd1310ba698dfb5ac,0x2ffd72dbd01adfb7,0xb8e1afed6a267e96,
      0xba7c9045f12c7f99,0x24a19947b3916cf7,0x0801f2e2858efc16,0x636920d871574e69,0xa458fea3f4933d7e,0x0d95748f728eb658,
      0x718bcd5882154aee,0x7b54a41dc25a59b5,0x9c30d5392af26013,0xc5d1b023286085f0,0xca417918b8db38ef,0x8e79dcb0603a180e]
E = [0xb7e151628aed2a6a,0xbf7158809cf4f3c7,0x62e7160f38b4da56,0xa784d9045190cfef,0x324e7738926cfbe5,0xf4bf8d8d8c31d763,
     0xda06c80abb1185eb,0x4f7c7b5757f59584,0x90cfd47d7c19bb42,0x158d9554f7b46bce,0xd55c4d79fd5f24d6,0x613c31c3839a2ddf,
     0x8a9a276bcfbfa1c8,0x77c56284dab79cd4,0xc2b3293d20e9e5ea,0xf02ac60acc93ed87,0x4422a52ecb238fee,0xe5ab6add835fd1a0,
     0x753d0a8f78e537d2,0xb95bb79d8dcaec64,0x2c1e9f23b829b5c2,0x780bf38737df8bb3,0x00d01334a0d0bd86,0x45cbfa73a6160ffe,
     0x393c48cbbbca060f,0x0ff8ec6d31beb5cc,0xeed7f2f0bb088017,0x163bc60df45a0ecb,0x1bcd289b06cbbfea,0x21ad08e1847f3f73,
     0x78d56ced94640d6e,0xf0d3d37be67008e1,0x86d1bf275b9b241d,0xeb64749a47dfdfb9,0x6632c3eb061b6472,0xbbf84c26144e49c2]
SUP = {15: [0,3,4,5,7,8,13], 30: [0,3,10,13,14,15,16,18,21,26,28]}
PAR = {512: (384, 576, 12, PI, (3,0,5,1,4,2)),
       768: (1088, 832, 18, E, (9,2,15,0,6,12,4,16,1,10,7,14,3,11,5,13,8)),
       1024: (768, 1152, 18, E, (8,2,11,0,5,9,3,10,1,7,4,6))}
def rotl(x, n):
    n %= 64
    return ((x << n) | (x >> (64 - n))) & M if n else x
def perm(X, rc):
    nw = len(X)
    for c in rc:
        Y = [0] * nw
        for j in range(nw // 5):                      # SC: 5-bit S-box, x0 = MSB
            x = X[5*j:5*j+5]
            for u in range(32):
                m = M
                for b in range(5):
                    m &= x[b] if (u >> (4 - b)) & 1 else (~x[b] & M)
                for b in range(5):
                    if (SB[u] >> (4 - b)) & 1: Y[5*j+b] |= m
        Z = [0] * nw
        for t in range(nw):                           # MS: right-circulant
            for k in SUP[nw]: Z[t] ^= Y[(t + k) % nw]
        Z[0] ^= c                                     # AC
        X = [rotl(Z[t], t) ^ rotl(Z[t], t + 21) ^ rotl(Z[t], t + 43) for t in range(nw)]   # ML
    return X
def duet(h, msg, nu):
    r, c, nr, RC, sig = PAR[h]
    bits = ''.join(format(b, '08b') for b in msg) + '1'
    bits += '0' * (-len(bits) % r)
    wr, wc = r // 64, c // 64
    A, B, C = [0] * wr, [0] * wr, [0] * wc
    def F(s, R, C):
        o = perm(R + C, RC[s*nr:(s+1)*nr])
        return o[:wr], [a ^ b for a, b in zip(o[wr:], C)]
    for o in range(0, len(bits), r):
        Mi = [int(bits[o+64*i:o+64*i+64], 2) for i in range(wr)]
        A, C = F(0, [a ^ m for a, m in zip(A, Mi)], C)
        B, C = F(1, [B[i] ^ Mi[sig[i]] for i in range(wr)], C)
    cap0 = ''.join(format(w, '064b') for w in C)      # finalized capacity C_M (c bits)
    Z = cap0[:h]                                      # r' = h  (Algorithm 1, line 1 and 16)
    while len(Z) < nu:
        A, C = F(0, A, C); Z += ''.join(format(w, '064b') for w in C)[:h]
        if len(Z) < nu:
            B, C = F(1, B, C); Z += ''.join(format(w, '064b') for w in C)[:h]
    return Z[:nu], cap0
h = int(sys.argv[1]); impl = bin(int(sys.argv[2], 16))[2:].zfill(4 * len(sys.argv[2]))
c = PAR[h][1]
spec, cap = duet(h, b'abc', 2 * h)
first = next((i for i in range(2 * h) if spec[i] != impl[i]), -1)
print("Duet-%d nu=%d: first %d bits equal: %s; first differing bit index: %d" % (h, 2*h, h, spec[:h] == impl[:h], first))
print("  implementation bits %d..%d equal the capacity tail C_M[%d..%d] (hidden by r' = h): %s" % (h, c-1, h, c-1, impl[h:c] == cap[h:c]))
sys.exit(0 if (spec[:h] == impl[:h] and spec != impl and impl[h:c] == cap[h:c]) else 1)
EOP
ok=0; n=0
for h in 512 768 1024; do
  for tree in "Reference_Implementation/Duet-$h/CryptHash_Duet-$h.c" \
              "Optimized_Implementation/Duet-$h/CryptHash_Duet-$h-c-avx2.c"; do
    case "$tree" in *avx2*) fl="-mavx2";; *) fl="";; esac
    if "$CC" -std=c99 -O1 $fl -DH=$h -I "$IMPL/$(dirname "$tree")" "$T/main.c" "$IMPL/$tree" -o "$T/t" 2>/dev/null; then
      out=$("$T/t") || continue
      n=$((n+1)); echo "[$tree]"
      if PYTHONDONTWRITEBYTECODE=1 python3 -I "$T/spec.py" $h "$out"; then ok=$((ok+1)); fi
    fi
  done
done
if [ "$n" -gt 0 ] && [ "$ok" -eq "$n" ]; then
  echo "ATTACK hash-08-2 Duet XOF output (digest_len_bits > h) deviates from Algorithm 1 from bit h and discloses the c-h hidden capacity bits in $ok/$n builds CONFIRMED"
else
  echo "NOT REPRODUCED ($ok/$n)"; exit 1
fi
