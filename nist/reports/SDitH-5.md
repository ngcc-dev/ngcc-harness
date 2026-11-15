# SDitH-5 — Low: Five further places where the code and KATs depart from the literal specification

Severity: Low
Status: Confirmed
Layer: Design
Affected: Specification text for all 12 parameter sets; the submitted code and KATs are internally consistent
Discovery: Moderate
Exploitation: Literal-specification interoperability failures; no forgery or key-recovery consequence shown
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-11-15

## Inconsistencies

Each item compares the v3.0 specification text (printed pages) with the
submitted reference code. [SDitH-2](/nist/reports/SDitH-2.html) is a separate verifier hash
omission.

1. **Witness encoding.** Algorithm 1 lines 10-11 (p.11) drop the last
   coordinate of each chunk, but the mux circuit (§3.2.6, p.34) reads an
   all-zero digit as position 0. A secret key re-encoded as Algorithm 1 writes
   it is rejected (0 of 24 keys accepted).
2. **Gray map.** Algorithms 11 and 12 line 4 (pp.29-30) print
   `Delta_inv = psi(istar)`, while Algorithm 12 lines 14 and 21 need
   `psi(phi_Gray(istar))`. A build without the Gray map rejects its own
   signatures (0 of 3).
3. **ExpandH.** The specification (pp.17-18) describes one sequential PRG
   stream; the code places column i at blocks 4i to 4i+3. Only column 0 of the
   tested columns matches the sequential stream; all 12 match the code layout.
4. **Challenge and grinding layout.** ExpandEvaluationChallenge (p.19) places
   bits differently from the code: with the specification's layout the
   grinding condition holds in 32 of 400 honest signatures, about chance.
5. **Signature field order.** Algorithm 3 line 9 (p.13) orders the fields
   differently from `map_signature`: with `hpiop` read from the position
   Algorithm 3 gives, grinding holds in 20 of 400 signatures, about chance.

Each check has a control that uses the code's own convention and succeeds in
every case.

## Classification and limits

Low / Confirmed / Design: specification/code mismatches that break
interoperability for an implementation written from the text. No security
consequence is shown. Items 2 to 5 were run on cat1_fast only; the sources
are identical across the 12 sets. The cipher-grinding half of item 4 is a
source check. The design survey's DS-SDitH-3 counts eleven inconsistencies
and names three; the overlap with the other eight could not be checked.

## Reproduction

```sh
python3 SDitH/reproduce_sdith_5.py
```

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py SDitH-5
```
