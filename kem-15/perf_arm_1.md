# kem-15 FLIT — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-15` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560844812374016.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: FLIT
- Implementation versions measured: reference
- Parameter sets: `FLIT128_REF`, `FLIT256_REF`, `FLIT512_REF`

## 2. Assessment environment

| item | value |
|---|---|
| processor | Qualcomm Oryon (CPU 2, one core) |
| machine | ASUS Vivobook S 15 |
| clock | fixed 2.71 GHz (governor performance, minimum = maximum), boost off, SMT none |
| memory | 30562 MiB |
| OS / kernel | Ubuntu 26.04.1 LTS / 7.0.0-34-generic |
| compiler / build tool | gcc (Ubuntu 15.2.0-16ubuntu1) 15.2.0 / cmake version 4.2.3 |
| campaign start / end (UTC) | 2026-09-28T15:05:46 / 2026-09-29T08:43:52 |

## 3. Functional testing (KAT)

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-15/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `FLIT128_REF` | guide | PASS |
| `FLIT256_REF` | guide | PASS |
| `FLIT512_REF` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `FLIT128_REF` | keygen | 107.1 k | 39.7 µs | 2.52e+04 | 39.7 µs | 70485 (5 × 14097) |
| `FLIT128_REF` | enc | 100.3 k | 37.2 µs | 2.69e+04 | 37.2 µs | 80810 (5 × 16162) |
| `FLIT128_REF` | dec | 122.4 k | 45.4 µs | 2.2e+04 | 45.4 µs | 66395 (5 × 13279) |
| `FLIT256_REF` | keygen | 260.5 k | 96.6 µs | 1.03e+04 | 96.5 µs | 33380 (5 × 6676) |
| `FLIT256_REF` | enc | 203.9 k | 75.6 µs | 1.32e+04 | 75.6 µs | 40270 (5 × 8054) |
| `FLIT256_REF` | dec | 272.4 k | 101 µs | 9.89e+03 | 101 µs | 30535 (5 × 6107) |
| `FLIT512_REF` | keygen | 1.07 M | 396 µs | 2.52e+03 | 396 µs | 8300 (5 × 1660) |
| `FLIT512_REF` | enc | 766.5 k | 284 µs | 3.52e+03 | 284 µs | 11040 (5 × 2208) |
| `FLIT512_REF` | dec | 928.3 k | 344 µs | 2.9e+03 | 344 µs | 9090 (5 × 1818) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `FLIT128_REF` | keygen | 29904 | 3428 KiB | 3496 KiB |
| `FLIT128_REF` | enc | 29904 | 1424 KiB | 1488 KiB |
| `FLIT128_REF` | dec | 29904 | 1420 KiB | 1484 KiB |
| `FLIT256_REF` | keygen | 30096 | 1420 KiB | 1492 KiB |
| `FLIT256_REF` | enc | 30096 | 1432 KiB | 1496 KiB |
| `FLIT256_REF` | dec | 30096 | 1436 KiB | 1500 KiB |
| `FLIT512_REF` | keygen | 27472 | 1428 KiB | 1516 KiB |
| `FLIT512_REF` | enc | 27472 | 1448 KiB | 1520 KiB |
| `FLIT512_REF` | dec | 27472 | 1452 KiB | 1524 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `FLIT128_REF` | 615 | 1351 | 512 | 32 |
| `FLIT256_REF` | 1229 | 2605 | 1024 | 32 |
| `FLIT512_REF` | 3072 | 6336 | 2304 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `FLIT128_REF` | keygen | 38% | 7.9% | drng 2, pseudoXOF 2.98, sm3hash 1 |
| `FLIT128_REF` | enc | 60% | 4.2% | drng 1, pseudoXOF 3, pseudohash 1, sm3hash 3 |
| `FLIT128_REF` | dec | 38% | 0.0% | pseudoXOF 3, pseudohash 1, sm3hash 1 |
| `FLIT256_REF` | keygen | 36% | 3.3% | drng 2, pseudoXOF 3.01, sm3hash 1 |
| `FLIT256_REF` | enc | 50% | 2.1% | drng 1, pseudoXOF 3, pseudohash 1, sm3hash 3 |
| `FLIT256_REF` | dec | 28% | 0.0% | pseudoXOF 3, pseudohash 1, sm3hash 1 |
| `FLIT512_REF` | keygen | 37% | 1.0% | drng 2, pseudoXOF 3.03, pseudohash 1 |
| `FLIT512_REF` | enc | 61% | 0.8% | drng 1, pseudoXOF 3, pseudohash 4 |
| `FLIT512_REF` | dec | 34% | 0.0% | pseudoXOF 3, pseudohash 2 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `FLIT128_REF` | KAT log (sha256 `bb801e99741d0972…`) | `kat/kem-15/FLIT128_REF.log` |
| `FLIT128_REF` | timing dec | `records/kem-15/FLIT128_REF__dec.json` |
| `FLIT128_REF` | timing enc | `records/kem-15/FLIT128_REF__enc.json` |
| `FLIT128_REF` | timing keygen | `records/kem-15/FLIT128_REF__keygen.json` |
| `FLIT128_REF` | hash profile dec | `profile/kem-15/FLIT128_REF__dec.json` |
| `FLIT128_REF` | hash profile enc | `profile/kem-15/FLIT128_REF__enc.json` |
| `FLIT128_REF` | hash profile keygen | `profile/kem-15/FLIT128_REF__keygen.json` |
| `FLIT256_REF` | KAT log (sha256 `3bffd869ee3e5ae8…`) | `kat/kem-15/FLIT256_REF.log` |
| `FLIT256_REF` | timing dec | `records/kem-15/FLIT256_REF__dec.json` |
| `FLIT256_REF` | timing enc | `records/kem-15/FLIT256_REF__enc.json` |
| `FLIT256_REF` | timing keygen | `records/kem-15/FLIT256_REF__keygen.json` |
| `FLIT256_REF` | hash profile dec | `profile/kem-15/FLIT256_REF__dec.json` |
| `FLIT256_REF` | hash profile enc | `profile/kem-15/FLIT256_REF__enc.json` |
| `FLIT256_REF` | hash profile keygen | `profile/kem-15/FLIT256_REF__keygen.json` |
| `FLIT512_REF` | KAT log (sha256 `aaec32954d4dc5ce…`) | `kat/kem-15/FLIT512_REF.log` |
| `FLIT512_REF` | timing dec | `records/kem-15/FLIT512_REF__dec.json` |
| `FLIT512_REF` | timing enc | `records/kem-15/FLIT512_REF__enc.json` |
| `FLIT512_REF` | timing keygen | `records/kem-15/FLIT512_REF__keygen.json` |
| `FLIT512_REF` | hash profile dec | `profile/kem-15/FLIT512_REF__dec.json` |
| `FLIT512_REF` | hash profile enc | `profile/kem-15/FLIT512_REF__enc.json` |
| `FLIT512_REF` | hash profile keygen | `profile/kem-15/FLIT512_REF__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

