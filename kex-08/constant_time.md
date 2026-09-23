# NIIKE constant-time review

Scope: NIIKE-lv128 representative reference, plus comparison of the lv256/lv512 key-generation helpers. The long-term secret is the integer vector `secretkey_t`; shared isogeny/j-invariant before output is secret. Public keys, peer encodings and fixed torsion/strategy parameters are public.

- `ngccapi/KEX_AlgorithmInstance.c:59-80,133-159` applies a secret vector to make public keys or shared values. `protocols/bundleprotocols.c:47-59` selects secret-dependent isogeny directions through `compare_gt` and `bundleswap_*` masks while keeping public loop bounds. Strategy checks at lines 60-70 depend on fixed traversal counters, not on `s[i]`.
- `NIIKE-lv128/protocols/bundleprotocols_internal.c:53-62` rejects random candidate key-vector entries above public bounds. This is variable key-generation work; rejected candidates are not the returned key. It was not promoted as a leakage of the retained secret vector. The lv512 helper (`NIIKE-lv512/protocols/bundleprotocols_internal.c:53-79`) has a byte-identical optimized copy and a separate fixed-two-key defect, `kex-08-2`.
- `protocols/bundleprotocols.c:193-213` has a debug-only curve-equality assertion/abort. The check is an invariant of the internal two-branch computation, not a demonstrated key-recovery oracle. Length-driven polynomial-recursion branches in `ec/bundle_poly-mul.c:26-45,127-166` use public/fixed polynomial lengths.

No new constant-time report was confirmed here. The raw shared-key distinguisher already tracked as `kex-08-1` is a design/output-distribution issue, not a timing leak. The lv512 fixed two-key cycle is independently tracked as `kex-08-2`; it is a direct key-generation defect, not a timing leak. This review is not a complete machine-code or malicious-peer-key analysis.
