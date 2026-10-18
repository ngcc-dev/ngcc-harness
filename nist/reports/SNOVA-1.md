# SNOVA-1 — Critical: biased vinegar sampling in six On-Ramp Round 3 alternative parameter sets

Severity: Critical
Status: Confirmed
Layer: Design
Affected: Six odd-characteristic alternative v3.0 sets; the nine recommended q=16 sets are unaffected by this bias
Discovery: Non-trivial
Exploitation: Published end-to-end signing-key recovery for alternatives A–D; root-cause and exact-distribution witness reproduced locally; E–F remain estimates
Remediation: Moderate
Credit: Original cryptanalysis by Ward Beullens and Basil Hess, who disclosed that its first version was independently found by the gpt-5.6-sol AI system; independent local verification by Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-10-18

The status applies to the exact sampler bias and the published A–D attacks.
The SNOVA team announced a v3.1 repair on September 28; that repair is not
pinned or verified here, and the pinned NIST submission remains v3.0.
Critical applies to the affected alternatives because ordinary signatures
disclose enough information for signing-key recovery. Obtaining the required
signatures is a material attack cost. No side channel, fault injection,
malformed signature, or weak RNG is required. These query counts are below
NIST's allowed chosen-message bound; see [NIST rules](https://github.com/ngcc-dev/ngcc-harness/blob/main/nist/doc/NIST-rules.md).

## Published result and version evidence

Beullens and Hess disclosed the attack on September 22, 2026, clarified the alternative-only scope on September 23, and the SNOVA team announced corrected specification, reference and Sage code on September 28. The team explicitly kept the NIST submission unchanged. [Primary forum thread](https://groups.google.com/a/list.nist.gov/g/pqc-forum/c/cb-JmKUKRSY).

In their September 22 post, Beullens and Hess state that the first version was
found independently by the `gpt-5.6-sol` AI system, after which they verified it,
made small improvements, and wrote up the attack. The paper separately
discloses substantial AI assistance in proposing and deriving key parts and
generating experimental code. The human authors take responsibility for the
paper's contents.

The [paper](https://eprint.iacr.org/2026/2154) models the leakage as q-ary LPN in dimension o·l and demonstrates four recoveries of the secret transformation T, which together with the public key enables signing; this need not recover the original secret seed. Its timings below measure recovery after collecting signatures, not total attack time. Set D's signature generation alone took 40 hours. The measurements used a 16-core/32-thread Xeon Gold 6346 and parallel row recovery. [Original paper attachment, Tables 3 and 6, pp.6 and 10](https://groups.google.com/a/list.nist.gov/group/pqc-forum/attach/126f9eb93966b/LPN_attack.pdf?part=0.1).

| Paper set | (v,o,q,l,r,m1,m2) | Claimed category | Signatures | Published recovery wall time | Peak RAM |
|---|---|---:|---:|---|---:|
| A | (27,4,19,4,6,5,96) | 1 | 9 million | 21.8 s | 13.6 GiB |
| B | (27,5,13,4,4,5,80) | 1 | 16.5 million | 34.4 s | 26.7 GiB |
| C | (39,5,11,4,6,7,120) | 3 | 9 million | 22.3 s | 6.45 GiB |
| D | (38,5,19,4,6,7,120) | 3 | 180 million | 16 min 11 s | 108.5 GiB |
| E | (40,5,19,5,6,6,150) | 5 | not demonstrated | estimated attack only | — |
| F | (47,6,19,4,6,9,144) | 5 | not demonstrated | estimated attack only | — |

The six tuples and categories are [specification Table 6, physical p.30](https://csrc.nist.gov/csrc/media/Projects/pqc-dig-sig/documents/SNOVA_specifications.pdf), also extracted into [alternative-parameters.csv](https://github.com/ngcc-dev/ngcc-harness/blob/main/nist/SNOVA/alternative-parameters.csv). The paper's naive estimates and the specification's security estimates use different operation units; do not equate small-field multiply-adds with Boolean gates. The publication reports ordinary-message key recovery, which enables new-message signing and thus concerns EUF-CMA rather than only strong unforgeability.

## Local source evidence

snova_ref.c (`SNOVA/submission/SNOVA_Round3/Reference_Implementation/SNOVA_I_S/snova_ref.c#L298`) implements `expand_gf`. It loads a little-endian integer from a fixed byte block, emits successive base-q digits with `val % SNOVA_q` and `val /= SNOVA_q`, and returns the leftover quotient. The signer (`SNOVA/submission/SNOVA_Round3/Reference_Implementation/SNOVA_I_S/snova_ref.c#L876`) fills a byte buffer from SHAKE256, passes it to this function for the vinegar variables, and discards its return value. This is the path prescribed by specification Algorithm 14 → Algorithm 2, physical pp.23 and 16. In contrast, `expand_T12` already uses bytewise rejection for secret coefficient generation over odd fields.

For b bytes and g field elements, let R=256^b and Q=q^g. A uniform byte block maps to its residue modulo Q. When Q does not divide R, some residue tuples have more preimages. The most significant base-q digit has exact counts

```text
width = q^(g-1)
cycles, remainder = divmod(R, q*width)
count[d] = cycles*width + clamp(remainder - d*width, 0, width)
```

These are integer counts over the complete input space, not a random-sampling estimate. The local witness obtains the following heavy-set probabilities; the right-hand column is an exact uniform control, rounded for presentation.

| q | (g,b) | Heavy set | Submitted sampler | Uniform sampler |
|---|---|---|---:|---:|
| 11 | (16,7) | 0..5 | 0.695652 | 0.545455 |
| 13 | (15,7) | 0..4 | 0.546421 | 0.384615 |
| 19 | (15,8) | 0..3 | 0.346514 | 0.210526 |
| 16 | (2,1) | all digits uniform | exact uniform | exact uniform |

The q=16 control succeeds because 16²=256. Uniform rejection controls also succeed: rejecting byte-block integers at or above `floor(R/Q)*Q` gives every tuple the same number of accepted preimages; bytewise rejection before reduction modulo q gives equal counts for each field element. These controls are demonstrations outside the submission tree. Public coefficient expansion and secret vinegar sampling have different security roles; a justification for biased public data does not justify this secret-dependent signing distribution.

## Reproduction

Run from the public harness root:

```sh
python3 SNOVA/reproduce_snova_1.py
```

The witness pins the SHA-256 of the local submitted C file and header, extracts the unchanged `expand_gf` body, compiles it with the submitted packing definitions, verifies 2,189 boundary/random cases across four fields, calculates exact marginal counts, and checks uniform/rejection controls. It also confirms that all nine recommended sets (18 SHAKE/AES reference builds) select q=16. Exit 0 is PASS, 77 means the C compiler is unavailable, and any failed assertion or compiler failure is nonzero. [Recorded local result](https://github.com/ngcc-dev/ngcc-harness/blob/main/nist/security/reproductions/SNOVA-1.json).

The same cheap command also runs a deliberately small q=13, dimension-2 LPN model: it uses the exact C sampler's top digit as noise, independently sampled public x, and a known two-element secret. Exhaustive hypothesis testing recovers that secret from 3,000 synthetic samples (best score 0.537667 versus 0.411333 for the best wrong key). Repeating with uniform block-rejection noise removes the signal: the best score is 0.403667 and the original key scores 0.368. This is a controlled demonstration of the leakage-to-LPN mechanism, not signatures emitted by a complete SNOVA signer. The independence model is explicit.

This reproduces the sampler flaw, its scope, and a toy recovery mechanism, not a full-size SNOVA key recovery. The optional pinned [author-artifact runner](https://github.com/ngcc-dev/ngcc-harness/blob/main/nist/security/reproduce_snova_upstream.py) and [artifact manifest](https://github.com/ngcc-dev/ngcc-harness/blob/main/nist/security/artifacts/SNOVA-LPN.json) exercise upstream attack code separately:

```sh
python3 security/reproduce_snova_upstream.py verify
python3 security/reproduce_snova_upstream.py sample --samples 64
```

The first command passes deterministic synthetic LPN planted-row recovery regressions over q=19,13,11. The second generates and verifies 64 message/signature pairs and extracts 64 LPN records from the pinned upstream implementation; it does not recover a real-size signing key. [Recorded executions and limitations](https://github.com/ngcc-dev/ngcc-harness/blob/main/nist/security/artifacts/SNOVA-LPN.md) distinguish those two checks. The upstream archive's `nist/generic/snova_ref.c` is byte-identical to the NIST file. Its separate `src/snova_ref.c` differs, although the `expand_gf` function bodies agree after whitespace normalization. No submitted code has been patched.

The original forum attachment SHA-256 is `4b44c54fec54334d3593609458d211c87ea0ba08b78c0948da1b73630d9fb23d`. Source retrieval attempts, including blocked ePrint downloads and successful primary alternatives, are recorded in [security/harvests.jsonl](https://github.com/ngcc-dev/ngcc-harness/blob/main/nist/security/harvests.jsonl). Preserve the pinned v3.0 package for reproducibility; apply uniform sampling when developing a corrected odd-characteristic implementation rather than changing the historical vectors silently.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py SNOVA-1
```
