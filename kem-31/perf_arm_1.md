# kem-31 QIMEN-PIKE — performance on AArch64 (system arm_1)

[Performance arm_1](../performance/summary_arm_1.md) › `kem-31` · [method](../performance/method_arm_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560872306036736.html)

Independent measurement following the structure of the NICCS ARM self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: QIMEN-PIKE
- Implementation versions measured: reference
- Parameter sets: `NGCC-1`, `NGCC-2`, `NGCC-3`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-31/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `NGCC-1` | harness-default | PASS |
| `NGCC-2` | harness-default | PASS |
| `NGCC-3` | harness-default | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `NGCC-1` | keygen | 348.67 M | 130 ms | 7.7 | 130 ms | 100 (5 × 20) |
| `NGCC-1` | enc | 112.40 M | 41.7 ms | 24 | 41.6 ms | 100 (5 × 20) |
| `NGCC-1` | dec | 173.12 M | 64.5 ms | 15.5 | 64.3 ms | 100 (5 × 20) |
| `NGCC-2` | keygen | 1.13 G | 419 ms | 2.38 | 413 ms | 100 (5 × 20) |
| `NGCC-2` | enc | 354.46 M | 131 ms | 7.6 | 131 ms | 100 (5 × 20) |
| `NGCC-2` | dec | 551.85 M | 205 ms | 4.87 | 205 ms | 100 (5 × 20) |
| `NGCC-3` | keygen | 7.20 G | 2.67 s | 0.374 | 2.67 s | 100 (5 × 20) |
| `NGCC-3` | enc | 2.69 G | 997 ms | 1 | 997 ms | 100 (5 × 20) |
| `NGCC-3` | dec | 4.12 G | 1.53 s | 0.655 | 1.53 s | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `NGCC-1` | keygen | 380256 | 1656 KiB | 32732 KiB |
| `NGCC-1` | enc | 380256 | 3856 KiB | 3920 KiB |
| `NGCC-1` | dec | 380256 | 4608 KiB | 19792 KiB |
| `NGCC-2` | keygen | 500220 | 1660 KiB | 81844 KiB |
| `NGCC-2` | enc | 500220 | 6396 KiB | 6460 KiB |
| `NGCC-2` | dec | 500220 | 8388 KiB | 48016 KiB |
| `NGCC-3` | keygen | 1013136 | 1668 KiB | 317640 KiB |
| `NGCC-3` | enc | 1013136 | 18148 KiB | 18220 KiB |
| `NGCC-3` | dec | 1013136 | 25976 KiB | 183216 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `NGCC-1` | 389 | 472 | 602 | 32 |
| `NGCC-2` | 592 | 734 | 882 | 32 |
| `NGCC-3` | 1198 | 1497 | 1746 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **bypass** — own SM3 + counter XOF (pike_hash.c) for G, KDF and streams; DRNG only

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `NGCC-1` | keygen | 0.0% | 2.2% | drng 1.93e+03 |
| `NGCC-1` | enc | 0.0% | 0.0% | drng 1 |
| `NGCC-1` | dec | 0.0% | 0.0% | – |
| `NGCC-2` | keygen | 0.0% | 0.4% | drng 1.04e+03 |
| `NGCC-2` | enc | 0.0% | 0.0% | drng 1 |
| `NGCC-2` | dec | 0.0% | 0.0% | – |
| `NGCC-3` | keygen | 0.0% | 0.1% | drng 1.18e+03 |
| `NGCC-3` | enc | 0.0% | 0.0% | drng 1 |
| `NGCC-3` | dec | 0.0% | 0.0% | – |

## 7. Raw evidence index

Paths are relative to `performance/data/arm_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `NGCC-1` | KAT log (sha256 `88c74f0596728d5c…`) | `kat/kem-31/NGCC-1.log` |
| `NGCC-1` | timing dec | `records/kem-31/NGCC-1__dec.json` |
| `NGCC-1` | timing enc | `records/kem-31/NGCC-1__enc.json` |
| `NGCC-1` | timing keygen | `records/kem-31/NGCC-1__keygen.json` |
| `NGCC-1` | hash profile dec | `profile/kem-31/NGCC-1__dec.json` |
| `NGCC-1` | hash profile enc | `profile/kem-31/NGCC-1__enc.json` |
| `NGCC-1` | hash profile keygen | `profile/kem-31/NGCC-1__keygen.json` |
| `NGCC-2` | KAT log (sha256 `05901ebe10c60488…`) | `kat/kem-31/NGCC-2.log` |
| `NGCC-2` | timing dec | `records/kem-31/NGCC-2__dec.json` |
| `NGCC-2` | timing enc | `records/kem-31/NGCC-2__enc.json` |
| `NGCC-2` | timing keygen | `records/kem-31/NGCC-2__keygen.json` |
| `NGCC-2` | hash profile dec | `profile/kem-31/NGCC-2__dec.json` |
| `NGCC-2` | hash profile enc | `profile/kem-31/NGCC-2__enc.json` |
| `NGCC-2` | hash profile keygen | `profile/kem-31/NGCC-2__keygen.json` |
| `NGCC-3` | KAT log (sha256 `566d6f919997e696…`) | `kat/kem-31/NGCC-3.log` |
| `NGCC-3` | timing dec | `records/kem-31/NGCC-3__dec.json` |
| `NGCC-3` | timing enc | `records/kem-31/NGCC-3__enc.json` |
| `NGCC-3` | timing keygen | `records/kem-31/NGCC-3__keygen.json` |
| `NGCC-3` | hash profile dec | `profile/kem-31/NGCC-3__dec.json` |
| `NGCC-3` | hash profile enc | `profile/kem-31/NGCC-3__enc.json` |
| `NGCC-3` | hash profile keygen | `profile/kem-31/NGCC-3__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

