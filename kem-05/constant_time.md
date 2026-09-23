# Constant-time review: BIKE-MLThre

The sparse parity-check vectors and fallback `sigma` in `sk`, decoded error `e`, recovered message `m`, and shared secret are secret. The ciphertext and fixed parameter lengths are public. This follows the submitted `Implementations/reference` path only; optimized code and emitted assembly remain unreviewed.

`kem.c:247-260` derives a syndrome using the private key and enters `BGF_decoder`. `decode_ml.c:305-348` branches on syndrome bits (`if (s[bit])` and `if (prev_s[bit])`), while the threshold/state computation and black/gray-flip updates are driven by that secret-derived syndrome. `kem.c:276-289` also branches on the decoded-error comparison to select either `sigma` or `m_prime` for the shared-secret hash. These are variable-control-flow candidates; the comparison branch is not itself proof of key recovery. No secret-indexed table or variable-divisor instruction was established in the representative trace.

Status: source-level CT concern; no controlled timing/cache measurement or complete extraction has been reproduced. The path should be hardened and retested before any constant-time claim. No separate report is filed here because the demonstrated observation is not yet narrowed beyond the decoded validity path.
