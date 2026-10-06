# kem-17 Hybrid Equivalent Punctured and Quasi-Cyclic — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-17` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560853532332032.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: Hybrid Equivalent Punctured and Quasi-Cyclic
- Implementation versions measured: reference
- Parameter sets: `hep-qc-1`, `hep-qc-3`, `hep-qc-5`, `hep-qc-7`

## 2. Assessment environment

| item | value |
|---|---|
| processor | 12th Gen Intel(R) Core(TM) i7-12700 (CPU 2, one core) |
| clock | max 2.10 GHz, governor performance, turbo off, SMT off |
| memory | 31788 MiB |
| OS / kernel | Debian GNU/Linux 13 (trixie) / 6.12.107+deb13-amd64 |
| compiler / build tool | gcc (Debian 14.2.0-19) 14.2.0 / cmake version 3.31.6 |
| campaign start / end (UTC) | 2026-09-25T10:02:21 / 2026-09-28T10:51:21 |

## 3. Functional testing (KAT)

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-17/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `hep-qc-1` | guide | PASS |
| `hep-qc-3` | guide | PASS |
| `hep-qc-5` | guide | PASS |
| `hep-qc-7` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `hep-qc-1` | keygen | 232.07 M | 111 ms | 8.99 | 111 ms | 100 (5 × 20) |
| `hep-qc-1` | enc | 15.14 M | 7.34 ms | 136 | 7.34 ms | 675 (5 × 135) |
| `hep-qc-1` | dec | 246.18 M | 118 ms | 8.5 | 118 ms | 100 (5 × 20) |
| `hep-qc-3` | keygen | 504.91 M | 244 ms | 4.11 | 242 ms | 100 (5 × 20) |
| `hep-qc-3` | enc | 45.13 M | 21.9 ms | 45.8 | 21.8 ms | 230 (5 × 46) |
| `hep-qc-3` | dec | 482.18 M | 232 ms | 4.31 | 231 ms | 100 (5 × 20) |
| `hep-qc-5` | keygen | 883.58 M | 426 ms | 2.35 | 425 ms | 100 (5 × 20) |
| `hep-qc-5` | enc | 103.32 M | 50 ms | 20 | 50 ms | 100 (5 × 20) |
| `hep-qc-5` | dec | 814.49 M | 391 ms | 2.56 | 390 ms | 100 (5 × 20) |
| `hep-qc-7` | keygen | 4.22 G | 2.04 s | 0.49 | 2.01 s | 100 (5 × 20) |
| `hep-qc-7` | enc | 684.81 M | 331 ms | 3.03 | 329 ms | 100 (5 × 20) |
| `hep-qc-7` | dec | 3.39 G | 1.63 s | 0.614 | 1.63 s | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `hep-qc-1` | keygen | 327429 | 1732 KiB | 3788 KiB |
| `hep-qc-1` | enc | 327429 | 2972 KiB | 41272 KiB |
| `hep-qc-1` | dec | 327429 | 3520 KiB | 9680 KiB |
| `hep-qc-3` | keygen | 907557 | 1736 KiB | 7912 KiB |
| `hep-qc-3` | enc | 907557 | 5372 KiB | 46676 KiB |
| `hep-qc-3` | dec | 907557 | 7108 KiB | 25696 KiB |
| `hep-qc-5` | keygen | 1897637 | 1728 KiB | 14868 KiB |
| `hep-qc-5` | enc | 1897637 | 9388 KiB | 47344 KiB |
| `hep-qc-5` | dec | 1897637 | 13088 KiB | 51000 KiB |
| `hep-qc-7` | keygen | 12731229 | 1828 KiB | 89708 KiB |
| `hep-qc-7` | enc | 12731229 | 52688 KiB | 311616 KiB |
| `hep-qc-7` | dec | 12731229 | 77712 KiB | 336604 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `hep-qc-1` | 285889 | 285969 | 4433 | 32 |
| `hep-qc-3` | 866210 | 866298 | 8978 | 32 |
| `hep-qc-5` | 1852485 | 1852581 | 14421 | 32 |
| `hep-qc-7` | 12644449 | 12644577 | 49297 | 32 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — never calls the DRNG: zero-initialised PRNG (known kem-17-1/-3)

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `hep-qc-1` | keygen | 87% | 0.0% | pseudoXOF 3.65e+04, pseudohash 1 |
| `hep-qc-1` | enc | 43% | 0.0% | pseudoXOF 14, pseudohash 1, sm3hash 1 |
| `hep-qc-1` | dec | 89% | 0.0% | pseudoXOF 3.83e+04, pseudohash 1, sm3hash 4 |
| `hep-qc-3` | keygen | 81% | 0.0% | pseudoXOF 7.31e+04, pseudohash 1 |
| `hep-qc-3` | enc | 42% | 0.0% | pseudoXOF 14, pseudohash 1, sm3hash 1 |
| `hep-qc-3` | dec | 88% | 0.0% | pseudoXOF 7.3e+04, pseudohash 1, sm3hash 4 |
| `hep-qc-5` | keygen | 74% | 0.0% | pseudoXOF 1.17e+05, pseudohash 1 |
| `hep-qc-5` | enc | 39% | 0.0% | pseudoXOF 14, pseudohash 1, sm3hash 1 |
| `hep-qc-5` | dec | 84% | 0.0% | pseudoXOF 1.16e+05, pseudohash 1, sm3hash 4 |
| `hep-qc-7` | keygen | 55% | 0.0% | pseudoXOF 4e+05, pseudohash 1 |
| `hep-qc-7` | enc | 39% | 0.0% | pseudoXOF 14, pseudohash 1, sm3hash 1 |
| `hep-qc-7` | dec | 73% | 0.0% | pseudoXOF 3.97e+05, pseudohash 1, sm3hash 4 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `hep-qc-1` | KAT log (sha256 `26da064f901ded5e…`) | `kat/kem-17/hep-qc-1.log` |
| `hep-qc-1` | timing dec | `records/kem-17/hep-qc-1__dec.json` |
| `hep-qc-1` | timing enc | `records/kem-17/hep-qc-1__enc.json` |
| `hep-qc-1` | timing keygen | `records/kem-17/hep-qc-1__keygen.json` |
| `hep-qc-1` | hash profile dec | `profile/kem-17/hep-qc-1__dec.json` |
| `hep-qc-1` | hash profile enc | `profile/kem-17/hep-qc-1__enc.json` |
| `hep-qc-1` | hash profile keygen | `profile/kem-17/hep-qc-1__keygen.json` |
| `hep-qc-3` | KAT log (sha256 `793a6b193df5b0a1…`) | `kat/kem-17/hep-qc-3.log` |
| `hep-qc-3` | timing dec | `records/kem-17/hep-qc-3__dec.json` |
| `hep-qc-3` | timing enc | `records/kem-17/hep-qc-3__enc.json` |
| `hep-qc-3` | timing keygen | `records/kem-17/hep-qc-3__keygen.json` |
| `hep-qc-3` | hash profile dec | `profile/kem-17/hep-qc-3__dec.json` |
| `hep-qc-3` | hash profile enc | `profile/kem-17/hep-qc-3__enc.json` |
| `hep-qc-3` | hash profile keygen | `profile/kem-17/hep-qc-3__keygen.json` |
| `hep-qc-5` | KAT log (sha256 `0df458797a12b3ce…`) | `kat/kem-17/hep-qc-5.log` |
| `hep-qc-5` | timing dec | `records/kem-17/hep-qc-5__dec.json` |
| `hep-qc-5` | timing enc | `records/kem-17/hep-qc-5__enc.json` |
| `hep-qc-5` | timing keygen | `records/kem-17/hep-qc-5__keygen.json` |
| `hep-qc-5` | hash profile dec | `profile/kem-17/hep-qc-5__dec.json` |
| `hep-qc-5` | hash profile enc | `profile/kem-17/hep-qc-5__enc.json` |
| `hep-qc-5` | hash profile keygen | `profile/kem-17/hep-qc-5__keygen.json` |
| `hep-qc-7` | KAT log (sha256 `076fda694e04f356…`) | `kat/kem-17/hep-qc-7.log` |
| `hep-qc-7` | timing dec | `records/kem-17/hep-qc-7__dec.json` |
| `hep-qc-7` | timing enc | `records/kem-17/hep-qc-7__enc.json` |
| `hep-qc-7` | timing keygen | `records/kem-17/hep-qc-7__keygen.json` |
| `hep-qc-7` | hash profile dec | `profile/kem-17/hep-qc-7__dec.json` |
| `hep-qc-7` | hash profile enc | `profile/kem-17/hep-qc-7__enc.json` |
| `hep-qc-7` | hash profile keygen | `profile/kem-17/hep-qc-7__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

