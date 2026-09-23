# Constant-time review: CTL

The private NTRU polynomials `f,g,F,G,w`, rejection seed `rr`, decrypted polynomial `s`, and shared secret are secret. Ciphertext, public key, dimensions, and `Q` are public. This traces the CTL-257-512 reference C path; the 769/3329 and optimized x86 paths are not certified by it.

In `crypto_kem/Implementations/CTL-257-512/api.c:716-789`, decapsulation decrypts with the private polynomials, re-encrypts, OR-accumulates all discrepancies, derives both good/bad key material, and selects with a mask. The code explicitly avoids branching on the comparison result (`api.c:767-789`). The visible `if (!mq_poly_inv_ntt(...))` and coefficient-bound branch in `kem257.c:605-622` occur in private-key reconstruction/key-generation support, not the cited decapsulation comparison. No secret-derived table index or variable-divisor operation was established in the representative decapsulation wrapper.

Status: wrapper-level source screen only. The transitive NTRU arithmetic, alternate parameter sets, and emitted machine code still require targeted analysis; no vulnerability report from this limited trace.
