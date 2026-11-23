No: the only Critical finding is confined to the written Algorithm 6, and correcting its coefficient update to the one the submitted code already uses changes no size and no operation count.

Impact: none
Scope: Function (small), specification
Assessed: 2026-11-23

## Attacks considered

- sign-17-1: The specified fixed-support challenge permits public-key forgeries in 2^39, 2^45, and 2^90 (Design, Confirmed)

## Remediation

- **sign-17-1.** Replace lines 16 and 17 of Algorithm 6 (physical p. 9), which swap `idx[i]` and `idx[j]` and then write the sign at `idx[j]`, with the update `c[i] = c[j]; c[j] = sign` of the submitted `poly_challenge`. Both recorded proposals contain this update: the original post proposes the submitted implementation's procedure, and the team says that specification v2.0 replaces this step and that the "lattice parameters and the byte sizes of keys and signatures are unchanged" (as reported, not evaluated). The submitted reference and AVX2 code already contains the repair, so it is a specification change only: the stated sizes are those of the code, and the sampler's work is already in the baseline. With every position reachable, the challenge count of the finding's model is binomial(n, τ)·2^τ = 2^234.02, 2^260.84 and 2^525.19 for the three sets, against 2^39, 2^45 and 2^90 as written. Scope: Function, specification (one step of one specified algorithm). Impact: none. The recorded proposals go further and restate the whole of Algorithm 6 after the code, with its parsing, rejection and refill rules, which the written algorithm also describes differently (a 16-bit multiply-and-shift index against the code's masked 9- or 10-bit index); that is a rewrite of one sub-algorithm of the specification with the same impact. Specification v2.0 itself was not read.

## Evidence

- Baseline, Table 1 (physical p. 3): (|pk|, |sig|) = (2560, 4349), (3392, 5540), (6720, 12021) bytes, with n = 512, 512, 1024 and τ = 39, 45, 90.
- Size control from the submitted constants (`params.h:24–43` in each reference tree): public key 64 + k·n·13/8 = 64 + 3·832 = 2560, 64 + 4·832 = 3392, 64 + 4·1664 = 6720; signature c̃ + ℓ·z + hint = 32 + 3·1408 + 93 = 4349, 64 + 4·1344 + 100 = 5540, 128 + 4·2944 + 117 = 12021. The stated sizes are the sizes of the submitted code, and none depends on the sampler.
- Source: `c->coeffs[i] = c->coeffs[b]; c->coeffs[b] = 1 - 2*(signs & 1); signs >>= 1` at `OPSsig-128/poly.c:264–266`, `OPSsig-256/poly.c:265–267` and `OPSsig-512/poly.c:266–268,283–285`; the AVX2 trees have the same update (`OPSsig-128-avx2/poly.c:602–604`).
- Challenge count: log2 binomial(512, 39) = 195.02, log2 binomial(512, 45) = 215.84, log2 binomial(1024, 90) = 435.19; adding τ sign bits gives 234.02, 260.84 and 525.19.
- Speed: Table 3 (physical p. 27) times the Level-1 reference code, which runs the corrected sampler; `poly_challenge` is about 0.03 ms of a 2.6 ms signature there. Nothing was measured.
- Proposed fixes: the two entries under sign-17-1 in `report.md`, and the team's 2026-10-09 response linked there.
