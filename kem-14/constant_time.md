# Constant-time review: DTRU

The NTRU private polynomial `fhat`, recovered message, rejection `z`, and shared secret are secret. Ciphertext, public key, lengths, and modulus are public. This traces the DTRU-Light reference implementation; 768/1536 and optimized code are not certified here.

`KEM_AlgorithmInstance.c:100-150` decrypts under `sk`, re-encrypts, OR-accumulates ciphertext differences, and selects the fallback secret with a bit mask. `dtru.c:52-70` performs private-polynomial multiplication before decoding. The KEM returns the failure bit at `KEM_AlgorithmInstance.c:150`; that is an explicit validity result, not by itself evidence that a private coefficient is exposed. The examined PKE body has no source-level branch on the secret polynomial. Arithmetic in `poly.c`, `ntt.c`, and `reduce.c` needs emitted-code review before ruling out divide/remainder timing.

Status: no additional confirmed secret-dependent lookup or branch in the representative PKE/KEM chain; no report from this limited review. This note does not assess the separate length-handling finding in `report.md`.
