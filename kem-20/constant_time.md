# Constant-time review: MAMBA-Frost

The packed private matrix/vector `S`, recovered message, rejection key material, and shared secret are secret. The public key, ciphertext, salt, and dimensions are public. This traces the MAMBA-Frost-128 reference source; 192/256, optimized builds, and emitted machine code remain unreviewed.

`Frost/src/kem.c:444-503` loads the private `S` and sign-extends every coefficient. At line 493 a C `if` tests the sign bit of each private coefficient before the main matrix multiplication; if compiled as a branch, its trace directly reveals the sign pattern of `S`. The subsequent error returns around lines 503,545,552,562 are driven by reconstruction/quantization status and need separate taint and timing checks. `%`/division inside rounding helpers is not promoted without checking the runtime divisor and compiled instruction sequence.

Status: source-level concern, not yet a confirmed machine-level side channel. No report is filed until an emitted-code or controlled trace witness shows the secret-bit branch survives compilation.
