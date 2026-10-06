# kem-32 Quasi-Cyclic Twisted McEliece Key Encapsulation Mechanism — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-32` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560872431865856.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: Quasi-Cyclic Twisted McEliece Key Encapsulation Mechanism
- Implementation versions measured: reference
- Parameter sets: `QCTM128`, `QCTM256`, `QCTM512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-32/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `QCTM128` | guide | PASS |
| `QCTM256` | guide | PASS |
| `QCTM512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `QCTM128` | keygen | 53.85 G | 25.9 s | 0.0387 | 25.9 s | 65 (5 × 13) |
| `QCTM128` | enc | 18.43 M | 8.83 ms | 113 | 8.83 ms | 675 (5 × 135) |
| `QCTM128` | dec | 5.90 G | 2.83 s | 0.354 | 2.83 s | 100 (5 × 20) |
| `QCTM256` | keygen | 132.38 G | 63.8 s | 0.0157 | 64.1 s | 10 (5 × 2) |
| `QCTM256` | enc | 53.10 M | 25.4 ms | 39.4 | 25.3 ms | 210 (5 × 42) |
| `QCTM256` | dec | 29.11 G | 14 s | 0.0714 | 14 s | 60 (5 × 12) |
| `QCTM512` | keygen | 793.78 G | 382 s | 0.00262 | 382 s | 2 (2 × 1) |
| `QCTM512` | enc | 211.05 M | 101 ms | 9.92 | 99 ms | 100 (5 × 20) |
| `QCTM512` | dec | 204.27 G | 98.5 s | 0.0102 | 98.5 s | 5 (5 × 1) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `QCTM128` | keygen | 94153 | 3772 KiB | 148244 KiB |
| `QCTM128` | enc | 94153 | 10372 KiB | 148096 KiB |
| `QCTM128` | dec | 94153 | 13368 KiB | 148036 KiB |
| `QCTM256` | keygen | 92073 | 3724 KiB | 473396 KiB |
| `QCTM256` | enc | 92073 | 11836 KiB | 472748 KiB |
| `QCTM256` | dec | 92073 | 22128 KiB | 472772 KiB |
| `QCTM512` | keygen | 93385 | 3680 KiB | 1828216 KiB |
| `QCTM512` | enc | 93385 | 17344 KiB | 1825844 KiB |
| `QCTM512` | dec | 93385 | 17336 KiB | 1828260 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `QCTM128` | 167987 | 192545 | 640 | 32 |
| `QCTM256` | 595662 | 641942 | 1153 | 32 |
| `QCTM512` | 2374727 | 2467243 | 2264 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **bypass** — own SHAKE256 for session key/keygen expansion; OpenSSL AES-256-CTR-DRBG seeded by the ICCS DRNG; auxfunc not linked

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `QCTM128` | keygen | 0.0% | 0.0% | drng 1 |
| `QCTM128` | enc | 0.0% | 0.0% | – |
| `QCTM128` | dec | 0.0% | 0.0% | – |
| `QCTM256` | keygen | 0.0% | 0.0% | drng 1 |
| `QCTM256` | enc | 0.0% | 0.0% | – |
| `QCTM256` | dec | 0.0% | 0.0% | – |
| `QCTM512` | keygen | 0.0% | 0.0% | drng 1 |
| `QCTM512` | enc | 0.0% | 0.0% | – |
| `QCTM512` | dec | 0.0% | 0.0% | – |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `QCTM128` | KAT log (sha256 `0d5002621bfae766…`) | `kat/kem-32/QCTM128.log` |
| `QCTM128` | timing dec | `records/kem-32/QCTM128__dec.json` |
| `QCTM128` | timing enc | `records/kem-32/QCTM128__enc.json` |
| `QCTM128` | timing keygen | `records/kem-32/QCTM128__keygen.json` |
| `QCTM128` | hash profile dec | `profile/kem-32/QCTM128__dec.json` |
| `QCTM128` | hash profile enc | `profile/kem-32/QCTM128__enc.json` |
| `QCTM128` | hash profile keygen | `profile/kem-32/QCTM128__keygen.json` |
| `QCTM256` | KAT log (sha256 `ac7d307c4f89faa6…`) | `kat/kem-32/QCTM256.log` |
| `QCTM256` | timing dec | `records/kem-32/QCTM256__dec.json` |
| `QCTM256` | timing enc | `records/kem-32/QCTM256__enc.json` |
| `QCTM256` | timing keygen | `records/kem-32/QCTM256__keygen.json` |
| `QCTM256` | hash profile dec | `profile/kem-32/QCTM256__dec.json` |
| `QCTM256` | hash profile enc | `profile/kem-32/QCTM256__enc.json` |
| `QCTM256` | hash profile keygen | `profile/kem-32/QCTM256__keygen.json` |
| `QCTM512` | KAT log (sha256 `d556ba96fa877051…`) | `kat/kem-32/QCTM512.log` |
| `QCTM512` | timing dec | `records/kem-32/QCTM512__dec.json` |
| `QCTM512` | timing enc | `records/kem-32/QCTM512__enc.json` |
| `QCTM512` | timing keygen | `records/kem-32/QCTM512__keygen.json` |
| `QCTM512` | hash profile dec | `profile/kem-32/QCTM512__dec.json` |
| `QCTM512` | hash profile enc | `profile/kem-32/QCTM512__enc.json` |
| `QCTM512` | hash profile keygen | `profile/kem-32/QCTM512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

