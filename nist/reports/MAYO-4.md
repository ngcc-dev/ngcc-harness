# MAYO-4 — Low: Round 3 linear term turns an all-zero-vinegar fault into valid signatures in the oil space

Severity: Low
Status: Confirmed
Layer: Side-channel
Affected: MAYO1, MAYO2, MAYO3 and MAYO5; a property of the On-Ramp Round 3 Algorithm 7 shared by all submitted trees (reference build run)
Discovery: Moderate
Exploitation: Software-simulated fault: valid signatures from a vinegar-zeroing fault give the secret oil space from one to three signatures; no physical fault was performed
Remediation: Moderate
Credit: Vinegar fault model and the partial-zero case: Thomas Aulbach, Fabio Campos and Juliane Krämer; vinegar-seed faults: Sönke Jendral and Elena Dubrova; Round 3 all-zero evaluation: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-11-15

## Prior work

Vinegar faults on MAYO are known. Aulbach, Campos and Krämer
([SoK: On the Physical Security of UOV-based Signature Schemes](https://eprint.iacr.org/2024/1818),
PQCrypto 2025, §3.3) note that a fault leaving "one or more (but not all)"
vinegar vectors zero still gives a solvable system, so the corresponding oil
vectors appear in the signature. Jendral and Dubrova
([MAYO Key Recovery by Fixing Vinegar Seeds](https://eprint.iacr.org/2024/1550),
CIC 2025) make the vinegar seed predictable by voltage glitching and recover
the key from a faulty signature; Sayari et al. and Aulbach et al. describe
loop-abort and skipped-sampling variants. These attacks target earlier MAYO
versions without the linear term.

This report covers the remaining case: all vinegar vectors zero. Before
Round 3 that attempt gives a zero system and fails, so the signer retries; in
Round 3 it succeeds.

## Mechanism

Round 3 adds the linear term `L(Lambda, s)` to the verification equation
(Algorithm 7, physical p.15). If every vinegar vector of a signing attempt is
zero, all quadratic contributions vanish and the attempt's linear system is
`sum_i E_i0 (Lambda_v O + Lambda_o) x_i = t`. That system is full rank about
as often as an honest one, so the attempt succeeds, and each signature block
`s_i` then lies in the secret oil space. The signature verifies, so verifying
after signing does not detect the fault. In the Round 2 design, without the
linear term, the same system is zero and the attempt fails.

The vinegar is derived at
`mayo.c:496-497` (`MAYO/submission/MAYO/Reference_Implementation/src/mayo.c#L496`) and folded into the system through
`m_calculate_M_and_VPV` (`arithmetic.h:262` (`MAYO/submission/MAYO/Reference_Implementation/src/arithmetic.h#L262`)).

## Evidence

The reproducer builds MAYO1, 2, 3 and 5 from the reference sources, with a
temporary copy of `mayo.c` that adds one switchable `memset` zeroing the
vinegar after the SHAKE call. The fault is simulated in software.

| Set | Faulty signer succeeded | Verified by unmodified open | Signatures for exact recovery of O |
|---|---:|---:|---|
| MAYO1 | 276/300 | 276/276 | 1 in every run |
| MAYO2 | 300/300 | 300/300 | 3 (k = 5 < o = 13) |
| MAYO3 | 300/300 | 300/300 | 1 in 296 of 298 runs, otherwise 2 |
| MAYO5 | 300/300 | 300/300 | 1 in 256 of 278 runs, otherwise 2 |

Every signature block lies in the oil space of the key, and row reduction of
the blocks reproduces O exactly. Controls: with `Lambda` also zeroed on the
signer side (Round 2 behaviour) every faulty call fails; unfaulted signatures
verify and are not in the oil space; with both switches off the patched build
is byte-identical to the unmodified one.

## Classification and limits

Low / Confirmed / Side-channel (fault model). It extends the known
vinegar-fault attacks to the all-zero case that earlier MAYO versions handled
safely. Fault attacks are outside EUF-CMA; the specification says the salt "brings some protection" against
fault attacks (physical p.31). No physical fault was attempted, and partial
faults (some vinegar vectors zero, or known non-zero vinegar) were not
re-tested here; they are covered by the prior work above.

A local check that rejects an attempt whose vinegar is zero addresses this
exact fault; general fault resistance needs the usual countermeasures, such as
re-deriving the vinegar or checking that a signature does not lie in the oil
space.

## Reproduction

```sh
python3 MAYO/reproduce_mayo_4.py
```

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py MAYO-4
```
