No: seeding the library's generator repairs kem-05-1 in code and a stated limit of 2^64 encapsulations per public key repairs kem-05-2 in the specification, and neither changes a transmitted object or an operation count in any parameter set.

Impact: none
Scope: Single line (small), specification and code
Assessed: 2026-11-23

## Attacks considered

- kem-05-1: The default library deterministically generates a public secret key (Implementation, Confirmed)
- kem-05-2: Same-key multi-instance decoding misses BIKE-MLThre-128 and -256 targets (Design, Confirmed)

## Remediation

- **kem-05-1.** Root cause: the default library draws its key-generation and encapsulation seeds from an AES-CTR generator whose state starts at zero and which no library path seeds. The costed repair is the missing seeding call, made once from the platform generator before the first draw; the KAT wrapper already contains such a call. Scope: Single line, code. The specification only says to sample (h0, h1), σ and m at random (§1.3, physical pp. 4–5) and stays as it is. Impact: none. The code skips no specified work: once seeded, `get_seeds` draws the same number of bytes as before, and seeding is done once per process. The report records no proposed fix. Not checked: the optimized tree, and the bundled generator beyond its seeding.
- **kem-05-2.** Root cause: every encapsulation under one public key adds r decoding targets to one same-code search. The costed repair is a stated limit of 2^64 encapsulations per public key. At that limit the finding's estimator gives 2^129.396 for BIKE-MLThre-128 and 2^259.995 for BIKE-MLThre-256, both at or above target. Static keys (§3.4, physical pp. 30–31) and the security notion are kept. Scope: Single line, specification. Impact: none. The report records no proposed fix. Alternative without a limit: at 2^80 ciphertexts the estimates are 2^124.258 and 2^253.731, and at unchanged r the smallest error weights that reach the targets are t = 139 (from 134) and t = 267 (from 264). A heavier error raises the decoding failure rate. The specification gives no failure-rate formula, so the block length that would hold 2^-128 and 2^-256 was not computed. If r grows in proportion to t, the public key and ciphertext grow about 4% at 128 and about 1% at 256 (judgment). Speed at those values was not estimated. These values are costing aids, not proposed parameters.

## Evidence

- Baseline: Table 1 (physical p. 3) gives (r, w, t) = (12323, 142, 134), (40973, 274, 264) and (150001, 546, 524). Table 9 (physical p. 33) gives public keys of 12,323 / 40,973 / 150,001 bits and ciphertexts of 12,579 / 41,229 / 150,513 bits. The formula |c| = r + μ (physical p. 33) with μ = 256, 256, 512 reproduces the ciphertext column. Neither repair changes r or μ.
- kem-05-1, source under `kem-05/Implementations/reference/`: `Makefile:5,46` builds the library with `-DNIST_RAND=1` and `FromNIST/rng.c`; `FromNIST/rng.c:14` is the zero-initialised global state; `kem.h:23–26` draws both seeds with `randombytes`; `kem.c:114` and `kem.c:169` are the key-generation and encapsulation calls; `kat/KEM_AlgorithmInstance.c:11–21` is the only path in that tree that seeds it.
- kem-05-2, estimator: the pinned `dsDOOM` loaded by `kem-05/reproduce_multi_instance.py`, with the witness's inputs (length 2r, dimension r − 1, weight t, r·2^s targets). It needs numpy and scipy; the default `python3` here lacks scipy, so an interpreter that has both was used. Command, for the first figure: `python3 -B -c "import sys; sys.path.insert(0,'kem-05'); import reproduce_multi_instance as w; print(w.load_estimator()(24646, 12322, 134, 12323 << 64)[:2])"`.

  | set | 2^64 ciphertexts: time, memory | 2^80 ciphertexts: time | t below target at 2^80 | smallest t at target at 2^80 |
  |---|---|---|---|---|
  | BIKE-MLThre-128 | 2^129.396, 2^91.188 | 2^124.258 | 138: 2^127.415 | 139: 2^128.337 |
  | BIKE-MLThre-256 | 2^259.995, 2^94.648 | 2^253.731 | 266: 2^255.659 | 267: 2^256.623 |
  | BIKE-MLThre-512 | 2^521.577, 2^98.390 | 2^513.965 | none | 524 (submitted) |

- Alternative, scaling: r/(w·t) is 0.648, 0.566 and 0.524 for the three sets of Table 1, so r proportional to t at fixed w is assumed. Then 139/134 = +3.7% and 267/264 = +1.1% in r, which is the public key, and the same within 0.1% for the ciphertext r + 256. At r scaled by the same factors, 12783 and 41439 (not adjusted to an admissible block length), the estimator gives 2^128.776 and 2^256.759 at 2^80 ciphertexts.
