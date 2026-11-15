# Constant-time review: LoongKEM

The secret noise vector, recovered message, rejection seed, and shared secret are secret. The public key, ciphertext, lengths, and modulus constants are public. This follows Loong128 reference C; the other reference parameter sets use a byte-identical `poly.c` and a `KEM_Loong.c` that differs only by one space in Loong256. The Optimized and Additional trees are not independently certified.

`KEM_Loong.c:187-218` decrypts by multiplying ciphertext data with the secret noise vector, then reduces and decodes. `KEM_Loong.c:264-304` re-encrypts and selects the fallback shared secret with a fixed-length XOR accumulator and byte mask. The inspected wrapper has no branch on a private coefficient or validity result. The reduction helpers it calls do: `standard_mod` and `central_mod` (`poly.c:39-51,67-75`) test the sign, range and zero-ness of secret-dependent intermediate values, and gcc 15.2.0 with the makefile's `-O3 -march=native -fomit-frame-pointer` keeps conditional jumps for the `poly.c:45,47` tests in `mat_mul` and `vector_central_modulo_q`. `poly.c` has no `/` or `%` operator; reduction modulo `Q = 8191` uses shifts and masks, and that build has no divide instruction in `poly.o` or `KEM_Loong.o`.

Status: representative-path source screen only; no report from this review.
