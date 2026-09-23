# Constant-time review: CheetahKEM

The secret noise vector in `sk`, recovered message, rejection seed, and shared secret are secret. `pk`, `ct`, and lengths are public. This review follows Cheetah128 reference source; other parameter directories are not independently certified.

`KEM_Cheetah.c:174-211` decrypts by multiplying the public ciphertext component with the secret noise vector and extracting the plaintext. `KEM_Cheetah.c:265-294` re-encrypts and selects the fallback shared secret with a fixed-length XOR accumulator and mask, without branching on failure. `mod.c:30,42,62` uses `% Q` and `ntt.c:20` likewise reduces by constant `Q`; source syntax alone does not establish a variable-latency hardware divide because `Q` is compile-time constant. `poly.c:45` rejects samples while constructing a public matrix from public seed, so that branch is not a secret leak in decapsulation.

Status: no confirmed secret-dependent branch or lookup in the traced KEM path; machine-code division and all variants remain to be checked. No report from this review.
