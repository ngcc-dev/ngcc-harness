# kem-16 HARE — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-16` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560844942397440.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: HARE
- Implementation versions measured: reference
- Parameter sets: `HARE-128-kr`, `HARE-256-kr`, `HARE-384-kr`, `HARE-512-kr`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-16/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `HARE-128-kr` | guide | PASS |
| `HARE-256-kr` | guide | PASS |
| `HARE-384-kr` | guide | PASS |
| `HARE-512-kr` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `HARE-128-kr` | keygen | 4.13 M | 1.53 ms | 652 | 1.53 ms | 1905 (5 × 381) |
| `HARE-128-kr` | enc | 9.54 M | 3.54 ms | 283 | 3.54 ms | 875 (5 × 175) |
| `HARE-128-kr` | dec | 14.16 M | 5.25 ms | 190 | 5.25 ms | 600 (5 × 120) |
| `HARE-256-kr` | keygen | 15.89 M | 5.9 ms | 170 | 5.89 ms | 530 (5 × 106) |
| `HARE-256-kr` | enc | 34.95 M | 13 ms | 77.1 | 13 ms | 245 (5 × 49) |
| `HARE-256-kr` | dec | 53.12 M | 19.7 ms | 50.7 | 19.7 ms | 165 (5 × 33) |
| `HARE-384-kr` | keygen | 47.23 M | 17.5 ms | 57.1 | 17.5 ms | 180 (5 × 36) |
| `HARE-384-kr` | enc | 100.50 M | 37.3 ms | 26.8 | 37.3 ms | 100 (5 × 20) |
| `HARE-384-kr` | dec | 150.39 M | 55.8 ms | 17.9 | 55.8 ms | 100 (5 × 20) |
| `HARE-512-kr` | keygen | 100.69 M | 37.4 ms | 26.8 | 37.4 ms | 100 (5 × 20) |
| `HARE-512-kr` | enc | 210.06 M | 77.9 ms | 12.8 | 77.9 ms | 100 (5 × 20) |
| `HARE-512-kr` | dec | 316.46 M | 117 ms | 8.52 | 117 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `HARE-128-kr` | keygen | 46944 | 1448 KiB | 1576 KiB |
| `HARE-128-kr` | enc | 46944 | 1516 KiB | 1584 KiB |
| `HARE-128-kr` | dec | 46944 | 1532 KiB | 1600 KiB |
| `HARE-256-kr` | keygen | 51664 | 1464 KiB | 1684 KiB |
| `HARE-256-kr` | enc | 51664 | 1644 KiB | 3756 KiB |
| `HARE-256-kr` | dec | 51664 | 1684 KiB | 1752 KiB |
| `HARE-384-kr` | keygen | 52304 | 1492 KiB | 1836 KiB |
| `HARE-384-kr` | enc | 52304 | 1844 KiB | 1916 KiB |
| `HARE-384-kr` | dec | 52304 | 1932 KiB | 2000 KiB |
| `HARE-512-kr` | keygen | 52824 | 3488 KiB | 4032 KiB |
| `HARE-512-kr` | enc | 52824 | 4080 KiB | 4144 KiB |
| `HARE-512-kr` | dec | 52824 | 4232 KiB | 4296 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `HARE-128-kr` | 2629 | 2677 | 4688 | 16 |
| `HARE-256-kr` | 6580 | 6676 | 11790 | 32 |
| `HARE-384-kr` | 13157 | 13301 | 23693 | 48 |
| `HARE-512-kr` | 21812 | 22004 | 39294 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — pseudoXOF only (recomputes prefix per call); lives under _shared/

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `HARE-128-kr` | keygen | 4.5% | 0.1% | drng 1, pseudoXOF 6.39 |
| `HARE-128-kr` | enc | 3.2% | 0.1% | drng 2, pseudoXOF 6 |
| `HARE-128-kr` | dec | 3.0% | 0.0% | pseudoXOF 8 |
| `HARE-256-kr` | keygen | 2.4% | 0.0% | drng 1, pseudoXOF 6.55 |
| `HARE-256-kr` | enc | 1.7% | 0.0% | drng 2, pseudoXOF 6 |
| `HARE-256-kr` | dec | 1.6% | 0.0% | pseudoXOF 8 |
| `HARE-384-kr` | keygen | 1.7% | 0.0% | drng 1, pseudoXOF 7.58 |
| `HARE-384-kr` | enc | 1.4% | 0.0% | drng 2, pseudoXOF 6 |
| `HARE-384-kr` | dec | 1.6% | 0.0% | pseudoXOF 9 |
| `HARE-512-kr` | keygen | 2.4% | 0.0% | drng 1, pseudoXOF 7.53 |
| `HARE-512-kr` | enc | 1.6% | 0.0% | drng 2, pseudoXOF 6 |
| `HARE-512-kr` | dec | 1.6% | 0.0% | pseudoXOF 9 |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `HARE-128-kr` | KAT log (sha256 `6e3ec02996e97149…`) | `kat/kem-16/HARE-128-kr.log` |
| `HARE-128-kr` | timing dec | `records/kem-16/HARE-128-kr__dec.json` |
| `HARE-128-kr` | timing enc | `records/kem-16/HARE-128-kr__enc.json` |
| `HARE-128-kr` | timing keygen | `records/kem-16/HARE-128-kr__keygen.json` |
| `HARE-128-kr` | hash profile dec | `profile/kem-16/HARE-128-kr__dec.json` |
| `HARE-128-kr` | hash profile enc | `profile/kem-16/HARE-128-kr__enc.json` |
| `HARE-128-kr` | hash profile keygen | `profile/kem-16/HARE-128-kr__keygen.json` |
| `HARE-256-kr` | KAT log (sha256 `9c49f57f0f59e50d…`) | `kat/kem-16/HARE-256-kr.log` |
| `HARE-256-kr` | timing dec | `records/kem-16/HARE-256-kr__dec.json` |
| `HARE-256-kr` | timing enc | `records/kem-16/HARE-256-kr__enc.json` |
| `HARE-256-kr` | timing keygen | `records/kem-16/HARE-256-kr__keygen.json` |
| `HARE-256-kr` | hash profile dec | `profile/kem-16/HARE-256-kr__dec.json` |
| `HARE-256-kr` | hash profile enc | `profile/kem-16/HARE-256-kr__enc.json` |
| `HARE-256-kr` | hash profile keygen | `profile/kem-16/HARE-256-kr__keygen.json` |
| `HARE-384-kr` | KAT log (sha256 `c8b7e261ad1aea7b…`) | `kat/kem-16/HARE-384-kr.log` |
| `HARE-384-kr` | timing dec | `records/kem-16/HARE-384-kr__dec.json` |
| `HARE-384-kr` | timing enc | `records/kem-16/HARE-384-kr__enc.json` |
| `HARE-384-kr` | timing keygen | `records/kem-16/HARE-384-kr__keygen.json` |
| `HARE-384-kr` | hash profile dec | `profile/kem-16/HARE-384-kr__dec.json` |
| `HARE-384-kr` | hash profile enc | `profile/kem-16/HARE-384-kr__enc.json` |
| `HARE-384-kr` | hash profile keygen | `profile/kem-16/HARE-384-kr__keygen.json` |
| `HARE-512-kr` | KAT log (sha256 `5378649b0f3e29f7…`) | `kat/kem-16/HARE-512-kr.log` |
| `HARE-512-kr` | timing dec | `records/kem-16/HARE-512-kr__dec.json` |
| `HARE-512-kr` | timing enc | `records/kem-16/HARE-512-kr__enc.json` |
| `HARE-512-kr` | timing keygen | `records/kem-16/HARE-512-kr__keygen.json` |
| `HARE-512-kr` | hash profile dec | `profile/kem-16/HARE-512-kr__dec.json` |
| `HARE-512-kr` | hash profile enc | `profile/kem-16/HARE-512-kr__enc.json` |
| `HARE-512-kr` | hash profile keygen | `profile/kem-16/HARE-512-kr__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

