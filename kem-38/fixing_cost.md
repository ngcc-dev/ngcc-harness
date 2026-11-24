Yes: the public key reveals the hidden Reed–Solomon structure in polynomial time (kem-38-8) and no repair that keeps the UVW design is known, so the impact on sizes and speed is not bounded at any of the three parameter sets.

Impact: not bounded
Scope: None known (large), specification and code
Assessed: 2026-11-24

## Attacks considered

- kem-38-1: UVW-512 derives its encryption pair from only 256 bits (Implementation, Confirmed)
- kem-38-4: Unused c1 bits make ciphertexts malleable without changing the key (Implementation, Confirmed)
- kem-38-8: Cubic-jet syzygies recover UVW's secret structure from the public key (Design, Confirmed)

## Remediation

- **kem-38-1.** UVW-KEM-512 hashes the message to a 32-byte seed and expands `(r, e)` from that seed, in encapsulation (`KEM_AlgorithmInstance.c:314–319`) and again in decapsulation (`:410–418`). The specification defines `H1` as a hash straight into `(r, e)` (Algorithm 6, physical p. 15) and has no 256-bit intermediate. The costed repair is a 64-byte `H1` seed at both sites. The message already has 512 bits (Table 1, m = 512; `params.h:15`), so the plaintext space does not grow. Scope: Function, code. Impact: none. The seed is never transmitted, and the change is 32 more bytes of hash output per call. The generator that expands the seed is the contest's ICCS SM3 DRBG (`drng.c`, `init_random_number` and `get_random_number`), not an expander of the scheme's own. By the standing NGCC convention that DRBG is a placeholder for a perfect DRBG: its 55-byte (440-bit) state (`drng.h:15`) is not a weakness, while seeding it with less input than the level is. That seeding is the root cause here and is what the repair changes; the generator's state is not costed. Using the external RBG as a deterministic expander at all, in place of the SM3-based XOF that the specification names for its hashes (§1.3, physical p. 15), is kem-38-7 (Low) and is not costed here.
- **kem-38-4.** `decompress_gf_array` ignores the 4 unused bits at the end of the packed `c1` of UVW-KEM-128 and UVW-KEM-512, and both the comparison and `H4` use the decoded values (`KEM_AlgorithmInstance.c:436,475–476,483` in UVW-KEM-128; `:433,480–481,488` in UVW-KEM-512). Algorithm 9 (physical p. 15) already requires the received `(c1, c2, d)` to equal the re-encryption. The costed repair rejects a ciphertext whose unused bits are not zero; comparing the received bytes with the packed re-encryption is an alternative of the same scope. Scope: Single line, code. The specification stays as it is: it gives no byte-padding rule, and this repair needs none. Impact: none. The 4 bits are already inside the stated ciphertext sizes, and the check reads one byte.
- **kem-38-8.** The public code hides a Reed–Solomon component behind twin columns. Cubic-jet syzygies of shortened public codes expose the pairing, the scalings and the Reed–Solomon description from the public key alone. At the two measured levels this refutes the Generalized (U+V, U+W) Code Distinguishing assumption (physical p. 16) on which Theorem 3 rests (physical p. 19). The report gives the method as polynomial in n, so larger parameters are not a repair. The report records no proposed fix for this finding, and no repair that keeps the design is known. Scope: None known, specification and code. Impact: not bounded. Recovery is demonstrated at UVW-KEM-128 and the distinguishing step at UVW-KEM-256; the report states that UVW-KEM-512 has the same structure.

## Evidence

- Baseline, Table 1 (physical p. 16): `(q, n, k, k1, k2, w, m)` = (433, 860, 430, 215, 215, 116, 256), (857, 1708, 854, 427, 427, 232, 256), (1709, 3412, 1706, 853, 853, 463, 512). Table 2 (physical p. 31): public key 208,013 / 911,645 / 4,001,850 bytes and ciphertext 1,032 / 2,199 / 4,820 bytes.
- Control, with b = ⌈log2 q⌉ = 9 / 10 / 11 bits per element (`params.h:9`):
  - Public key ⌈k·(n−k)·b/8⌉: 430·430·9 = 1,664,100 bits, 208,013 bytes; 854·854·10 = 7,293,160 bits, 911,645 bytes; 1706·1706·11 = 32,014,796 bits, 4,001,850 bytes.
  - Ciphertext ⌈n·b/8⌉ + 2·m/8: 968 + 64 = 1,032; 2,135 + 64 = 2,199; 4,692 + 128 = 4,820.
  - Unused bits in `c1`: 860·9 = 7,740 = 8·968 − 4; 1708·10 = 17,080 = 8·2,135; 3412·11 = 37,532 = 8·4,692 − 4. UVW-KEM-256 has none, as the report says.
- The two implementation repairs change no stated size: the `H1` seed of kem-38-1 is internal, and kem-38-4 only constrains bits that are already sent.
- Work: Table 3 (physical p. 32) gives UVW512 medians of 11,644,865 cycles for encapsulation and 20,290,008,773 for decapsulation. The kem-38-1 repair adds 32 bytes of hash output to each.
- The `kem-38-1` check of `security/design_parameter_audit.py` (`:982–991`) tests the specification text for the 512-bit claim and the type of `H1`, and the UVW-KEM-512 source for the 32-byte seed. All five conditions hold on the extracted text and the source. The script was not re-run when this note was checked, because it loads every built library before it selects the report ID.
- Generator: `drng.c` and `drng.h` carry the ICCS header and are the contest's DRBG. Encapsulation seeds a local `DRNG_ctx` with the 32-byte `H1` output (`KEM_AlgorithmInstance.c:318–319`) and draws `r` and `e` through `get_random_bytes_from`, which calls `get_random_number` (`:1510–1513`); decapsulation does the same at `:417–418`. `H1` itself is the contest `pseudoXOF`, through `kem_hash` (`:1681`, `:1716`).
- Source paths are under `Implementations/Reference_Implementation/UVW-KEM-<level>/`, files `src/KEM_AlgorithmInstance.c`, `include/params.h` and `include/drng.h`. The optimized UVW-KEM-512 tree has the same 32-byte seed (`Implementations/Optimized_Implementation/UVW-KEM-512/src/KEM_AlgorithmInstance.c:314`).
- Not checked: nothing was built or run for this note. The papers linked from kem-38-8 were not read for repair proposals. No parameter estimate was made for kem-38-8, because the attack is not a parameter-model attack.
