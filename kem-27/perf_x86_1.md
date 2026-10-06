# kem-27 NTRE Key Encapsulation Mechanism — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-27` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560863326031872.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: NTRE Key Encapsulation Mechanism
- Implementation versions measured: reference
- Parameter sets: `NTRE-128`, `NTRE-256`, `NTRE-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-27/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `NTRE-128` | guide | PASS |
| `NTRE-256` | guide | PASS |
| `NTRE-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `NTRE-128` | keygen | 142.3 k | 68 µs | 1.47e+04 | 67.9 µs | 63495 (5 × 12699) |
| `NTRE-128` | enc | 113.7 k | 54.3 µs | 1.84e+04 | 54.3 µs | 80195 (5 × 16039) |
| `NTRE-128` | dec | 113.2 k | 54.1 µs | 1.85e+04 | 54.1 µs | 80620 (5 × 16124) |
| `NTRE-256` | keygen | 261.1 k | 125 µs | 8.02e+03 | 125 µs | 36490 (5 × 7298) |
| `NTRE-256` | enc | 220.1 k | 105 µs | 9.51e+03 | 105 µs | 44055 (5 × 8811) |
| `NTRE-256` | dec | 230.7 k | 110 µs | 9.07e+03 | 110 µs | 42380 (5 × 8476) |
| `NTRE-512` | keygen | 574.5 k | 274 µs | 3.64e+03 | 274 µs | 17400 (5 × 3480) |
| `NTRE-512` | enc | 535.8 k | 256 µs | 3.91e+03 | 256 µs | 18895 (5 × 3779) |
| `NTRE-512` | dec | 541.9 k | 259 µs | 3.86e+03 | 259 µs | 18455 (5 × 3691) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `NTRE-128` | keygen | 26833 | 1696 KiB | 1788 KiB |
| `NTRE-128` | enc | 26833 | 1708 KiB | 1780 KiB |
| `NTRE-128` | dec | 26833 | 1704 KiB | 1792 KiB |
| `NTRE-256` | keygen | 27625 | 1712 KiB | 1788 KiB |
| `NTRE-256` | enc | 27625 | 1704 KiB | 1808 KiB |
| `NTRE-256` | dec | 27625 | 1680 KiB | 1808 KiB |
| `NTRE-512` | keygen | 28753 | 1700 KiB | 1780 KiB |
| `NTRE-512` | enc | 28753 | 1736 KiB | 1800 KiB |
| `NTRE-512` | dec | 28753 | 1732 KiB | 1796 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `NTRE-128` | 972 | 1976 | 972 | 64 |
| `NTRE-256` | 1944 | 3920 | 1944 | 64 |
| `NTRE-512` | 3456 | 6944 | 3456 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `NTRE-128` | keygen | 28% | 6.6% | drng 2, pseudoXOF 2, sm3hash 1 |
| `NTRE-128` | enc | 49% | 5.4% | drng 1, pseudoXOF 2, sm3hash 1 |
| `NTRE-128` | dec | 30% | 0.0% | pseudoXOF 2 |
| `NTRE-256` | keygen | 29% | 3.5% | drng 2, pseudoXOF 2, sm3hash 1 |
| `NTRE-256` | enc | 49% | 3.4% | drng 1, pseudoXOF 2, sm3hash 1 |
| `NTRE-256` | dec | 28% | 0.0% | pseudoXOF 2 |
| `NTRE-512` | keygen | 22% | 1.6% | drng 2, pseudoXOF 2, sm3hash 1 |
| `NTRE-512` | enc | 44% | 2.0% | drng 1, pseudoXOF 2, sm3hash 1 |
| `NTRE-512` | dec | 30% | 0.0% | pseudoXOF 2 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `NTRE-128` | KAT log (sha256 `ab39c026e265dc9c…`) | `kat/kem-27/NTRE-128.log` |
| `NTRE-128` | timing dec | `records/kem-27/NTRE-128__dec.json` |
| `NTRE-128` | timing enc | `records/kem-27/NTRE-128__enc.json` |
| `NTRE-128` | timing keygen | `records/kem-27/NTRE-128__keygen.json` |
| `NTRE-128` | hash profile dec | `profile/kem-27/NTRE-128__dec.json` |
| `NTRE-128` | hash profile enc | `profile/kem-27/NTRE-128__enc.json` |
| `NTRE-128` | hash profile keygen | `profile/kem-27/NTRE-128__keygen.json` |
| `NTRE-256` | KAT log (sha256 `22af84c4f0f20a40…`) | `kat/kem-27/NTRE-256.log` |
| `NTRE-256` | timing dec | `records/kem-27/NTRE-256__dec.json` |
| `NTRE-256` | timing enc | `records/kem-27/NTRE-256__enc.json` |
| `NTRE-256` | timing keygen | `records/kem-27/NTRE-256__keygen.json` |
| `NTRE-256` | hash profile dec | `profile/kem-27/NTRE-256__dec.json` |
| `NTRE-256` | hash profile enc | `profile/kem-27/NTRE-256__enc.json` |
| `NTRE-256` | hash profile keygen | `profile/kem-27/NTRE-256__keygen.json` |
| `NTRE-512` | KAT log (sha256 `70b4df4abed55279…`) | `kat/kem-27/NTRE-512.log` |
| `NTRE-512` | timing dec | `records/kem-27/NTRE-512__dec.json` |
| `NTRE-512` | timing enc | `records/kem-27/NTRE-512__enc.json` |
| `NTRE-512` | timing keygen | `records/kem-27/NTRE-512__keygen.json` |
| `NTRE-512` | hash profile dec | `profile/kem-27/NTRE-512__dec.json` |
| `NTRE-512` | hash profile enc | `profile/kem-27/NTRE-512__enc.json` |
| `NTRE-512` | hash profile keygen | `profile/kem-27/NTRE-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

