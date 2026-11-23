#!/usr/bin/env python3
"""Hash-work count for the kex-03 fixing-cost note (stdlib only, no scheme code).

Counts the bytes hashed and the SM3 compression calls of the CreTAKE-S2K
initiator's Der step before and after the repairs of kex-03-2 (signature
bytes in the KDF) and kex-03-4 (bit length of the G input).  All lengths are
transcribed from the submitted headers named below; nothing is executed.

Sources (under Implementations/Reference_Implementation/):
  Common/primitives/sig/BiT-*/params.h          BIT_PUBLICKEYBYTES, BIT_SECRETKEYBYTES, BIT_SIGNBYTES
  Common/primitives/kem/POLARLAC-*/params.h     PKE_PUBLIC_KEY_BYTES, PKE_CIPHERTEXT_BYTES, SS_KEY_BYTES
  Common/primitives/kem/ZEN_*/params.h          ZEN_INDCPA_PUBLICKEY_LEN_BYTES, ..._CIPHERTEXT_LEN_BYTES
  CreTAKE*/CreTAKE-S2K-*/KEX_AlgorithmInstance.c:27-48 (KDF), :124-125 and :201-204 (G), :188-216 (Der step)
  auxfunc.c:122-161 (sm3_bit), :187-222 (hmac), :224-279 (pseudohash_512), :482-530 (pseudoXOF)
"""

def sm3(nbytes):
    """SM3 compression calls for an nbytes message (auxfunc.c sm3_bit)."""
    return nbytes // 64 + (1 if nbytes % 64 <= 55 else 2)

def pseudohash512(m):
    """pseudohash_512: HMAC over 0x0200||msg, SM3 over msg||0x0200, SM3 over 64 bytes."""
    return sm3(64 + 2 + m) + sm3(64 + 32) + sm3(m + 2) + sm3(64)

def pseudoxof(out, m):
    """pseudoXOF: one SM3 over msg||counter per 32 output bytes."""
    return -(-out // 32) * sm3(m + 4)

# BiT: (public key, secret key, signature) bytes, from the params.h formulas.
def bit_sizes(n, seed, ch, tr, b1bits, b0bits, z0bits, z1bits, hbytes):
    pk = seed + 3 * (n * b1bits // 8)
    sk = seed + 3 * (n * b1bits // 8) + seed + tr + 6 * n // 4 + 3 * (n * b0bits // 8)
    sig = ch + n * z0bits // 8 + 3 * (n * z1bits // 8) + hbytes
    return pk, sk, sig

BIT = {
    # BiT-128: BIT_POLY_B1_BYTES is the literal 344 (= 256 * 10.75 / 8), so pk and sk are set directly.
    128: (16 + 3 * 344, 16 + 3 * 344 + 16 + 32 + 6 * 256 // 4 + 3 * (256 * 4 // 8),
          32 + 256 * 4 // 8 + 3 * (256 * 11 // 8) + 3 * ((256 * 3 + 7) // 8)),
    256: bit_sizes(512, 32, 64, 64, 11, 6, 5, 13, 3 * ((512 * 3 + 7) // 8)),
    512: bit_sizes(1024, 64, 64, 64, 13, 6, 5, 14, 3 * ((1024 + 4) // 5)),
}

# wKEM: (public key, CPA ciphertext, KEM ciphertext, shared-key bytes, minimum SM3 calls in wGen,
#        XOF output bytes drawn in wGen)
def plac(n, pkseed, sel, mlen, tern_out):
    pk = pkseed + 2 * (n + sel)
    ct = 2 * n + n * 4 // 8
    wgen = (pseudoxof(pkseed + 64, 64)                    # pke.c:114 seed expansion
            + 4 * pseudoxof(n, pkseed + 2 + 5)            # sample.c poly_generate_uniformQ, 2x2 polys
            + 4 * pseudoxof(tern_out, 64 + 5))            # sample.c poly_generate_tenary, s and e, no resampling
    return pk, ct, ct, mlen, wgen, (pkseed + 64) + 4 * n + 4 * tern_out

def zen(pk, ct, mlen, f_out, g_out):
    wgen = pseudoxof(f_out, 65) + pseudoxof(g_out, 65)    # poly.c poly_generate_f/g (gf), no resampling
    return pk, ct, ct, mlen, wgen, f_out + g_out

KEM = {
    ("PLAC", 128): plac(256, 16, 1, 16, 32 * 4),
    ("PLAC", 256): plac(512, 32, 2, 32, 64 * 3),
    ("PLAC", 512): plac(1024, 64, 2, 64, 128 * 7),
    ("ZEN", 128): zen(615, 512, 16, 512 * 2 // 8, 512 * 5 // 8),
    ("ZEN", 256): zen(1229, 1024, 32, 1024 * 3 // 8, 1024 * 4 // 8),
    ("ZEN", 512): zen(2458, 2048, 64, 2048 * 5 // 8, 2048 * 5 // 8),
}

print("Controls against the specification (Table 2-4, physical pp. 27-28):")
for lvl in (128, 256, 512):
    print("  BiT-%d pk %d sig %d (sk %d)" % (lvl, BIT[lvl][0], BIT[lvl][2], BIT[lvl][1]))
for key in sorted(KEM):
    print("  %s-%d pk %d ct %d" % (key[0], key[1], KEM[key][0], KEM[key][1]))

print()
print("S2K initiator Der step = G(r||sk_i), wGen, wDecaps, KDF  (no signature operation)")
print("%-22s %13s %15s %21s %8s %22s %7s" % ("instance", "G bytes", "KDF bytes", "SM3 calls (G+wGen+KDF)",
                                             "change", "bytes, one-pass hash", "change"))
for (fam, lvl) in sorted(KEM, key=lambda t: (t[1], t[0])):
    bpk, bsk, bsig = BIT[lvl]
    pk, cpa_ct, kem_ct, klen, wgen, wgen_out = KEM[(fam, lvl)]
    g_before = (64 + bsk) // 8        # byte count passed as a bit count
    g_after = 64 + bsk
    kdf_before = pk + bpk + pk + cpa_ct + kem_ct + 2 * klen
    kdf_after = kdf_before + bsig
    out = 2 * klen                    # K_LEN
    before = pseudohash512(g_before) + wgen + pseudoxof(out, kdf_before)
    after = pseudohash512(g_after) + wgen + pseudoxof(out, kdf_after)
    # With an ideal hash that absorbs each input once: bytes absorbed by G and the KDF
    # plus bytes squeezed in wGen.
    ideal_before = g_before + kdf_before + wgen_out
    ideal_after = g_after + kdf_after + wgen_out
    print("%-22s %5d -> %5d %6d -> %6d %9d -> %5d %+10.0f%% %10d -> %6d %+6.0f%%" % (
        "S2K-BiT%d-%s%d" % (lvl, fam, lvl), g_before, g_after, kdf_before, kdf_after,
        before, after, 100.0 * (after - before) / before,
        ideal_before, ideal_after, 100.0 * (ideal_after - ideal_before) / ideal_before))

print()
print("wGen SM3 calls with each sampler run once, and the change in the step if every wGen call is made twice:")
for (fam, lvl) in sorted(KEM, key=lambda t: (t[1], t[0])):
    bpk, bsk, bsig = BIT[lvl]
    pk, cpa_ct, kem_ct, klen, wgen, wgen_out = KEM[(fam, lvl)]
    kdf_before = pk + bpk + pk + cpa_ct + kem_ct + 2 * klen
    fixed_before = pseudohash512((64 + bsk) // 8) + pseudoxof(2 * klen, kdf_before)
    fixed_after = pseudohash512(64 + bsk) + pseudoxof(2 * klen, kdf_before + bsig)
    print("  S2K-BiT%d-%s%d: wGen %d; %d -> %d (%+.0f%%)" % (
        lvl, fam, lvl, wgen, fixed_before + 2 * wgen, fixed_after + 2 * wgen,
        100.0 * (fixed_after - fixed_before) / (fixed_before + 2 * wgen)))

print()
print("KDF input bytes, all signature-bearing frameworks (kex-03-2 adds each transmitted signature):")
for (fam, lvl) in sorted(KEM, key=lambda t: (t[1], t[0])):
    bpk, bsk, bsig = BIT[lvl]
    pk, cpa_ct, kem_ct, klen, wgen, wgen_out = KEM[(fam, lvl)]
    k2s = pk + pk + bpk + cpa_ct + kem_ct + 2 * klen
    s2k = pk + bpk + pk + cpa_ct + kem_ct + 2 * klen
    s2s = pk + bpk + bpk + cpa_ct + klen
    print("  %s%d: K2S %d -> %d (%+.0f%%), S2K %d -> %d (%+.0f%%), S2S %d -> %d (%+.0f%%)" % (
        fam, lvl, k2s, k2s + bsig, 100.0 * bsig / k2s, s2k, s2k + bsig, 100.0 * bsig / s2k,
        s2s, s2s + 2 * bsig, 200.0 * bsig / s2s))

print()
for mlen in (16, 32, 64):               # MSG_LEN_BYTES at 128, 256, 512; SEED_BYTES = 64
    print("Responder expansion (kex-03-1), %d-byte message: SM3 calls %d -> %d" % (
        mlen, pseudoxof(mlen + 64, 8), pseudoxof(mlen + 64, 64)))
print("K2K-PLAC512 seed expansion (kex-03-7): SM3 calls %d -> %d per 2Encaps or 2Decaps" % (
    pseudoxof(3 * 64, 16), pseudoxof(3 * 64, 128)))
