# kem-26 NSS-HQC — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-26` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560863191814144.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: NSS-HQC
- Implementation versions measured: reference
- Parameter sets: `HQC-128`, `HQC-256`, `HQC-384`, `HQC-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-26/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `HQC-128` | guide | PASS |
| `HQC-256` | guide | PASS |
| `HQC-384` | guide | PASS |
| `HQC-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `HQC-128` | keygen | 36.60 M | 17.5 ms | 57.1 | 17.5 ms | 295 (5 × 59) |
| `HQC-128` | enc | 64.37 M | 30.8 ms | 32.5 | 30.8 ms | 165 (5 × 33) |
| `HQC-128` | dec | 128.53 M | 61.5 ms | 16.3 | 61.6 ms | 100 (5 × 20) |
| `HQC-256` | keygen | 108.08 M | 51.7 ms | 19.4 | 51.7 ms | 100 (5 × 20) |
| `HQC-256` | enc | 194.06 M | 92.8 ms | 10.8 | 92.8 ms | 100 (5 × 20) |
| `HQC-256` | dec | 343.84 M | 164 ms | 6.08 | 164 ms | 100 (5 × 20) |
| `HQC-384` | keygen | 366.16 M | 176 ms | 5.67 | 175 ms | 100 (5 × 20) |
| `HQC-384` | enc | 661.21 M | 317 ms | 3.15 | 316 ms | 100 (5 × 20) |
| `HQC-384` | dec | 1.09 G | 521 ms | 1.92 | 519 ms | 100 (5 × 20) |
| `HQC-512` | keygen | 868.07 M | 416 ms | 2.4 | 415 ms | 100 (5 × 20) |
| `HQC-512` | enc | 1.58 G | 759 ms | 1.32 | 762 ms | 100 (5 × 20) |
| `HQC-512` | dec | 2.53 G | 1.21 s | 0.824 | 1.21 s | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `HQC-128` | keygen | 42913 | 1720 KiB | 1932 KiB |
| `HQC-128` | enc | 42913 | 1864 KiB | 1940 KiB |
| `HQC-128` | dec | 42913 | 1864 KiB | 1980 KiB |
| `HQC-256` | keygen | 43305 | 1724 KiB | 2052 KiB |
| `HQC-256` | enc | 43305 | 1988 KiB | 2060 KiB |
| `HQC-256` | dec | 43305 | 2016 KiB | 2084 KiB |
| `HQC-384` | keygen | 43549 | 1712 KiB | 2388 KiB |
| `HQC-384` | enc | 43549 | 2316 KiB | 2420 KiB |
| `HQC-384` | dec | 43549 | 2364 KiB | 2440 KiB |
| `HQC-512` | keygen | 44941 | 1776 KiB | 2808 KiB |
| `HQC-512` | enc | 44941 | 2792 KiB | 2880 KiB |
| `HQC-512` | dec | 44941 | 2856 KiB | 2956 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `HQC-128` | 3713 | 3777 | 5185 | 32 |
| `HQC-256` | 6844 | 6908 | 9084 | 32 |
| `HQC-384` | 15371 | 15467 | 18571 | 48 |
| `HQC-512` | 27302 | 27430 | 31462 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **bypass** — own inline Keccak (SHA3-512, SHAKE256) for everything; DRNG for randomness

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `HQC-128` | keygen | 0.0% | 0.0% | drng 3 |
| `HQC-128` | enc | 0.0% | 0.0% | drng 2 |
| `HQC-128` | dec | 0.0% | 0.0% | – |
| `HQC-256` | keygen | 0.0% | 0.0% | drng 3 |
| `HQC-256` | enc | 0.0% | 0.0% | drng 2 |
| `HQC-256` | dec | 0.0% | 0.0% | – |
| `HQC-384` | keygen | 0.0% | 0.0% | drng 3 |
| `HQC-384` | enc | 0.0% | 0.0% | drng 2 |
| `HQC-384` | dec | 0.0% | 0.0% | – |
| `HQC-512` | keygen | 0.0% | 0.0% | drng 3 |
| `HQC-512` | enc | 0.0% | 0.0% | drng 2 |
| `HQC-512` | dec | 0.0% | 0.0% | – |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `HQC-128` | KAT log (sha256 `0ae3f10578153ac0…`) | `kat/kem-26/HQC-128.log` |
| `HQC-128` | timing dec | `records/kem-26/HQC-128__dec.json` |
| `HQC-128` | timing enc | `records/kem-26/HQC-128__enc.json` |
| `HQC-128` | timing keygen | `records/kem-26/HQC-128__keygen.json` |
| `HQC-128` | hash profile dec | `profile/kem-26/HQC-128__dec.json` |
| `HQC-128` | hash profile enc | `profile/kem-26/HQC-128__enc.json` |
| `HQC-128` | hash profile keygen | `profile/kem-26/HQC-128__keygen.json` |
| `HQC-256` | KAT log (sha256 `cfb6f411b0029fb0…`) | `kat/kem-26/HQC-256.log` |
| `HQC-256` | timing dec | `records/kem-26/HQC-256__dec.json` |
| `HQC-256` | timing enc | `records/kem-26/HQC-256__enc.json` |
| `HQC-256` | timing keygen | `records/kem-26/HQC-256__keygen.json` |
| `HQC-256` | hash profile dec | `profile/kem-26/HQC-256__dec.json` |
| `HQC-256` | hash profile enc | `profile/kem-26/HQC-256__enc.json` |
| `HQC-256` | hash profile keygen | `profile/kem-26/HQC-256__keygen.json` |
| `HQC-384` | KAT log (sha256 `3ccb17273638e539…`) | `kat/kem-26/HQC-384.log` |
| `HQC-384` | timing dec | `records/kem-26/HQC-384__dec.json` |
| `HQC-384` | timing enc | `records/kem-26/HQC-384__enc.json` |
| `HQC-384` | timing keygen | `records/kem-26/HQC-384__keygen.json` |
| `HQC-384` | hash profile dec | `profile/kem-26/HQC-384__dec.json` |
| `HQC-384` | hash profile enc | `profile/kem-26/HQC-384__enc.json` |
| `HQC-384` | hash profile keygen | `profile/kem-26/HQC-384__keygen.json` |
| `HQC-512` | KAT log (sha256 `10e89e600d76e1dc…`) | `kat/kem-26/HQC-512.log` |
| `HQC-512` | timing dec | `records/kem-26/HQC-512__dec.json` |
| `HQC-512` | timing enc | `records/kem-26/HQC-512__enc.json` |
| `HQC-512` | timing keygen | `records/kem-26/HQC-512__keygen.json` |
| `HQC-512` | hash profile dec | `profile/kem-26/HQC-512__dec.json` |
| `HQC-512` | hash profile enc | `profile/kem-26/HQC-512__enc.json` |
| `HQC-512` | hash profile keygen | `profile/kem-26/HQC-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

