# Constant-time review: MAMBA-Frost

The packed private matrix/vector `S`, recovered message, rejection key material, and shared secret are secret. The public key, ciphertext, salt, and dimensions are public. This traces the MAMBA-Frost-128 reference source. The Reference and Optimized trees ship the same `kem.c` for all ten parameter sets, apart from the banner comment in four Frost-CC copies; optimized builds remain unreviewed.

`Frost/src/kem.c:444-503` loads the private `S` and sign-extends every coefficient. At line 493 a C `if` tests the sign bit of each private coefficient before the main matrix multiplication; if compiled as a branch, its trace directly reveals the sign pattern of `S`. The subsequent error returns at lines 503-505, 545-547, 552-554 and 562-564 fire only when `malloc` fails in the dither helpers (`kem.c:157-161,188-192`; `frost_expand_dither_local` always returns 0), so they do not depend on secret data. The rounding helpers (`kem.c:126-138`) use only shifts, masks and additions; `kem.c` has no runtime `/` or `%`.

Status: the secret-bit branch survives compilation in the reference build. gcc 15.2.0 with the reference Makefile's `-std=c99 -O2` compiles the line-493 test to `test $0x4,%dl; je` in `crypto_kem_dec_Frost128`. No timing or trace measurement has been made.
