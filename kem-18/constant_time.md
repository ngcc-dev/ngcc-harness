# Constant-time review: LoongKEM

The secret noise vector, recovered message, rejection seed, and shared secret are secret. The public key, ciphertext, lengths, and modulus constants are public. This follows Loong128 reference C; higher parameter sets and emitted code are not independently certified.

`KEM_Loong.c:187-224` decrypts by multiplying ciphertext data with the secret noise vector, then reduces and decodes. `KEM_Loong.c:264-304` re-encrypts and selects the fallback shared secret with a fixed-length XOR accumulator and byte mask. The inspected wrapper has no branch on a private coefficient or validity result. Modulo/rounding helpers require machine-code review to determine whether any variable-time divide remains; source syntax with compile-time modulus alone does not establish leakage.

Status: representative-path source screen only; no report from this review.
