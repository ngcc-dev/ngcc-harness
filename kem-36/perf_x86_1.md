# kem-36 TRIKE — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-36` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560881424453632.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: TRIKE
- Implementation versions measured: reference
- Parameter sets: `TRIKE-2`, `TRIKE-5`, `TRIKE-7`, `TRIKE-9`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-36/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `TRIKE-2` | guide | PASS |
| `TRIKE-5` | guide | PASS |
| `TRIKE-7` | guide | PASS |
| `TRIKE-9` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `TRIKE-2` | keygen | 101.46 M | 48.5 ms | 20.6 | 48.5 ms | 105 (5 × 21) |
| `TRIKE-2` | enc | 9.36 M | 4.47 ms | 224 | 4.47 ms | 1120 (5 × 224) |
| `TRIKE-2` | dec | 51.33 M | 24.7 ms | 40.4 | 24.7 ms | 205 (5 × 41) |
| `TRIKE-5` | keygen | 740.74 M | 355 ms | 2.82 | 354 ms | 100 (5 × 20) |
| `TRIKE-5` | enc | 69.36 M | 33.2 ms | 30.2 | 33.2 ms | 155 (5 × 31) |
| `TRIKE-5` | dec | 203.28 M | 97.3 ms | 10.3 | 97.3 ms | 100 (5 × 20) |
| `TRIKE-7` | keygen | 2.41 G | 1.15 s | 0.868 | 1.15 s | 100 (5 × 20) |
| `TRIKE-7` | enc | 203.56 M | 97.3 ms | 10.3 | 97.3 ms | 100 (5 × 20) |
| `TRIKE-7` | dec | 598.79 M | 286 ms | 3.49 | 286 ms | 100 (5 × 20) |
| `TRIKE-9` | keygen | 3.04 G | 1.46 s | 0.685 | 1.46 s | 100 (5 × 20) |
| `TRIKE-9` | enc | 206.80 M | 98.9 ms | 10.1 | 98.9 ms | 100 (5 × 20) |
| `TRIKE-9` | dec | 1.17 G | 559 ms | 1.79 | 559 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `TRIKE-2` | keygen | 32473 | 1700 KiB | 1868 KiB |
| `TRIKE-2` | enc | 32473 | 1780 KiB | 1856 KiB |
| `TRIKE-2` | dec | 32473 | 1756 KiB | 2140 KiB |
| `TRIKE-5` | keygen | 33105 | 1732 KiB | 2056 KiB |
| `TRIKE-5` | enc | 33105 | 1940 KiB | 2056 KiB |
| `TRIKE-5` | dec | 33105 | 1900 KiB | 2532 KiB |
| `TRIKE-7` | keygen | 32417 | 1736 KiB | 2196 KiB |
| `TRIKE-7` | enc | 32417 | 2116 KiB | 2180 KiB |
| `TRIKE-7` | dec | 32417 | 2128 KiB | 3268 KiB |
| `TRIKE-9` | keygen | 32033 | 1796 KiB | 2232 KiB |
| `TRIKE-9` | enc | 32033 | 2156 KiB | 2308 KiB |
| `TRIKE-9` | dec | 32033 | 2196 KiB | 3836 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `TRIKE-2` | 1980 | 6328 | 3928 | 32 |
| `TRIKE-5` | 4453 | 13988 | 8874 | 32 |
| `TRIKE-7` | 8776 | 27260 | 17488 | 64 |
| `TRIKE-9` | 14320 | 44228 | 28576 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `TRIKE-2` | keygen | 0.0% | 0.8% | drng 111 |
| `TRIKE-2` | enc | 4.7% | 16% | drng 267, pseudohash 2 |
| `TRIKE-2` | dec | 0.8% | 2.8% | drng 266, pseudohash 2 |
| `TRIKE-5` | keygen | 0.0% | 0.2% | drng 171 |
| `TRIKE-5` | enc | 1.4% | 3.7% | drng 433, pseudohash 2 |
| `TRIKE-5` | dec | 0.5% | 1.2% | drng 432, pseudohash 2 |
| `TRIKE-7` | keygen | 0.0% | 0.1% | drng 255 |
| `TRIKE-7` | enc | 0.9% | 2.1% | drng 663, pseudohash 2 |
| `TRIKE-7` | dec | 0.3% | 0.7% | drng 662, pseudohash 2 |
| `TRIKE-9` | keygen | 0.0% | 0.1% | drng 339 |
| `TRIKE-9` | enc | 1.5% | 2.9% | drng 881, pseudohash 2 |
| `TRIKE-9` | dec | 0.3% | 0.5% | drng 880, pseudohash 2 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `TRIKE-2` | KAT log (sha256 `f6274d9cafb268a9…`) | `kat/kem-36/TRIKE-2.log` |
| `TRIKE-2` | timing dec | `records/kem-36/TRIKE-2__dec.json` |
| `TRIKE-2` | timing enc | `records/kem-36/TRIKE-2__enc.json` |
| `TRIKE-2` | timing keygen | `records/kem-36/TRIKE-2__keygen.json` |
| `TRIKE-2` | hash profile dec | `profile/kem-36/TRIKE-2__dec.json` |
| `TRIKE-2` | hash profile enc | `profile/kem-36/TRIKE-2__enc.json` |
| `TRIKE-2` | hash profile keygen | `profile/kem-36/TRIKE-2__keygen.json` |
| `TRIKE-5` | KAT log (sha256 `cfb1dfb374afbab0…`) | `kat/kem-36/TRIKE-5.log` |
| `TRIKE-5` | timing dec | `records/kem-36/TRIKE-5__dec.json` |
| `TRIKE-5` | timing enc | `records/kem-36/TRIKE-5__enc.json` |
| `TRIKE-5` | timing keygen | `records/kem-36/TRIKE-5__keygen.json` |
| `TRIKE-5` | hash profile dec | `profile/kem-36/TRIKE-5__dec.json` |
| `TRIKE-5` | hash profile enc | `profile/kem-36/TRIKE-5__enc.json` |
| `TRIKE-5` | hash profile keygen | `profile/kem-36/TRIKE-5__keygen.json` |
| `TRIKE-7` | KAT log (sha256 `4602e1ee0e81d100…`) | `kat/kem-36/TRIKE-7.log` |
| `TRIKE-7` | timing dec | `records/kem-36/TRIKE-7__dec.json` |
| `TRIKE-7` | timing enc | `records/kem-36/TRIKE-7__enc.json` |
| `TRIKE-7` | timing keygen | `records/kem-36/TRIKE-7__keygen.json` |
| `TRIKE-7` | hash profile dec | `profile/kem-36/TRIKE-7__dec.json` |
| `TRIKE-7` | hash profile enc | `profile/kem-36/TRIKE-7__enc.json` |
| `TRIKE-7` | hash profile keygen | `profile/kem-36/TRIKE-7__keygen.json` |
| `TRIKE-9` | KAT log (sha256 `bcf110980810c047…`) | `kat/kem-36/TRIKE-9.log` |
| `TRIKE-9` | timing dec | `records/kem-36/TRIKE-9__dec.json` |
| `TRIKE-9` | timing enc | `records/kem-36/TRIKE-9__enc.json` |
| `TRIKE-9` | timing keygen | `records/kem-36/TRIKE-9__keygen.json` |
| `TRIKE-9` | hash profile dec | `profile/kem-36/TRIKE-9__dec.json` |
| `TRIKE-9` | hash profile enc | `profile/kem-36/TRIKE-9__enc.json` |
| `TRIKE-9` | hash profile keygen | `profile/kem-36/TRIKE-9__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

