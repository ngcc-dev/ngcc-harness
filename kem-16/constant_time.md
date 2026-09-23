# Constant-time review: HARE

The private HQC seed/secret vector, recovered message, fallback `sigma`, and shared secret are secret. Ciphertext, public key, salt, and loop dimensions are public. This traces the shared `hare_core/ref` decoder used by the reference builds; x86/SVE backends and emitted code are not certified.

`common/kem.c:166-251` decrypts under the private seed, re-encrypts, compares the ciphertext, and selects the fallback through byte masks. The Reed–Solomon decoder is explicitly written with fixed public loop bounds and masked Berlekamp–Massey/erasure selection (`ref/reed_solomon.c:205-267`); `gf_pow_alpha` at `ref/reed_solomon.c:33-37` has a loop and table index, but its call arguments at lines 115,136,159,351 are public loop positions, not decoded secret symbols. `ref/gf.c:156-195` uses a fixed multiplication/inversion chain. The KEM API returns a decapsulation-validity status at `common/kem.c:252`; no separate secret-coefficient leakage follows from that alone.

Status: no source-confirmed secret-indexed lookup or branch in this representative decoder; no report from this review. Whole-program compiler/CPU CT analysis remains open.
