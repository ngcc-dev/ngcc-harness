No: seeding the random number generator that the four FIPS202 implementations leave unseeded is a code-only change that alters no size and no operation count.

Impact: none
Scope: Single line (small), code
Assessed: 2026-11-23

## Attacks considered

- kem-15-3: The FIPS202 variants generate predictable keys because their RNG is never seeded (Implementation, Confirmed)

## Remediation

- **kem-15-3.** The root cause is that `randombytes` in the four FIPS202 trees expands a static all-zero seed and counter, and nothing in the KEM API calls `randombytes_init`. The specification is sound on the point: key generation and encapsulation draw `z`, `ρ` and `m` uniformly at random (Algorithms 8, 11 and 12). The costed repair is the missing call: seed that generator once, before its first use, from the randomness that the caller supplies. The primary reference and optimized trees already draw from the caller-seeded `drng_algorithm` at the same three call sites, and the KAT driver of the FIPS202 trees seeds that object too. This is the cheapest credible repair because it removes the unseeded state in all four trees and leaves the scheme, its parameters and its encodings as they are. Scope: Single line, code. Impact: none. No transmitted object depends on the generator. Each draw stays one SHAKE256 call on a 40-byte input for 32 output bytes, so key generation (two draws), encapsulation (one draw) and decapsulation (no draw) do the same work as the submitted code; the seeding itself happens once per process. The report records no proposed fix for this finding. The two forum posts and the repository page linked under the proposed fixes of kem-15-1 and kem-15-2 do not mention the generator; the repository's files were not read. The repaired trees were not built or run.

## Evidence

- Baseline: Table 11 (physical p. 21) gives public keys of 615 and 1229 bytes and ciphertexts of 512 and 1024 bytes for FLIT128 and FLIT256. The FIPS202 instantiation exists at these two levels only (§4.2.3, physical p. 20). The repair changes none of these sizes.
- Specification steps: Algorithm 8 line 1 (`z ← {0,1}^λ`, physical p. 10), Algorithm 11 line 1 (`ρ ← {0,1}^λ`) and Algorithm 12 line 1 (`m ← {0,1}^λ`), both on physical p. 11.
- Source, identical in `FLIT_FIPS202_REF128`, `REF256`, `OPT128` and `OPT256` under `Implementations/Additional_Implementation/`: the zero seed and counter at `rng.c:16–17`; `randombytes` at `rng.c:32–42`, one `shake256` call over the 32-byte seed and 8-byte counter; `randombytes_init` at `rng.c:19–30`, which no KEM function calls. The draws are at `indcpa.c:33` and `kem.c:32` (key generation) and `kem.c:59` (encapsulation), each of `SEEDBYTES` = 32 bytes.
- Primary trees for comparison: `Implementations/Reference_Implementation/FLIT128/indcpa.c:34` and `kem.c:33,60` call `get_random_number(&drng_algorithm, …)`, as do the same lines of the FLIT256 and FLIT512 reference trees and of the three optimized trees. The FIPS202 KAT driver seeds `drng_algorithm` at `KAT_KEM.c:105`.
- Count: two draws in key generation, one in encapsulation, none in decapsulation, before and after the repair.
