# kem-07 BRQC — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-07` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560843751215104.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: BRQC
- Implementation versions measured: reference
- Parameter sets: `BRQC-128`, `BRQC-256`, `BRQC-512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-07/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `BRQC-128` | guide | PASS |
| `BRQC-256` | guide | PASS |
| `BRQC-512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `BRQC-128` | keygen | 3.03 M | 1.45 ms | 690 | 1.45 ms | 3365 (5 × 673) |
| `BRQC-128` | enc | 6.65 M | 3.17 ms | 315 | 3.17 ms | 1565 (5 × 313) |
| `BRQC-128` | dec | 94.14 M | 45 ms | 22.2 | 45 ms | 115 (5 × 23) |
| `BRQC-256` | keygen | 9.07 M | 4.33 ms | 231 | 4.33 ms | 1225 (5 × 245) |
| `BRQC-256` | enc | 18.72 M | 8.94 ms | 112 | 8.93 ms | 580 (5 × 116) |
| `BRQC-256` | dec | 256.46 M | 122 ms | 8.16 | 123 ms | 100 (5 × 20) |
| `BRQC-512` | keygen | 26.53 M | 12.7 ms | 78.9 | 12.7 ms | 410 (5 × 82) |
| `BRQC-512` | enc | 55.45 M | 26.5 ms | 37.8 | 26.5 ms | 190 (5 × 38) |
| `BRQC-512` | dec | 1.07 G | 510 ms | 1.96 | 510 ms | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `BRQC-128` | keygen | 137057 | 1796 KiB | 1920 KiB |
| `BRQC-128` | enc | 137057 | 1856 KiB | 1928 KiB |
| `BRQC-128` | dec | 137057 | 1860 KiB | 1932 KiB |
| `BRQC-256` | keygen | 137913 | 1800 KiB | 1936 KiB |
| `BRQC-256` | enc | 137913 | 1884 KiB | 1988 KiB |
| `BRQC-256` | dec | 137913 | 1916 KiB | 1996 KiB |
| `BRQC-512` | keygen | 138001 | 1828 KiB | 1988 KiB |
| `BRQC-512` | enc | 138001 | 2000 KiB | 2072 KiB |
| `BRQC-512` | dec | 138001 | 2056 KiB | 2128 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `BRQC-128` | 1954 | 2066 | 3844 | 64 |
| `BRQC-256` | 3345 | 3471 | 6626 | 64 |
| `BRQC-512` | 6562 | 6712 | 13060 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — pseudohash for G/H; seed expansion via per-seed ICCS DRNG instances; XKCP Keccak dead

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `BRQC-128` | keygen | 0.0% | 7.7% | drng 10 |
| `BRQC-128` | enc | 5.3% | 3.7% | drng 11, pseudohash 2 |
| `BRQC-128` | dec | 0.4% | 0.6% | drng 17, pseudohash 2 |
| `BRQC-256` | keygen | 0.0% | 4.7% | drng 10.6 |
| `BRQC-256` | enc | 3.1% | 2.1% | drng 11.3, pseudohash 2 |
| `BRQC-256` | dec | 0.2% | 0.4% | drng 18, pseudohash 2 |
| `BRQC-512` | keygen | 0.0% | 2.9% | drng 10.3 |
| `BRQC-512` | enc | 2.1% | 1.3% | drng 11, pseudohash 2 |
| `BRQC-512` | dec | 0.1% | 0.2% | drng 17, pseudohash 2 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `BRQC-128` | KAT log (sha256 `6d0ab26eb7ee280a…`) | `kat/kem-07/BRQC-128.log` |
| `BRQC-128` | timing dec | `records/kem-07/BRQC-128__dec.json` |
| `BRQC-128` | timing enc | `records/kem-07/BRQC-128__enc.json` |
| `BRQC-128` | timing keygen | `records/kem-07/BRQC-128__keygen.json` |
| `BRQC-128` | hash profile dec | `profile/kem-07/BRQC-128__dec.json` |
| `BRQC-128` | hash profile enc | `profile/kem-07/BRQC-128__enc.json` |
| `BRQC-128` | hash profile keygen | `profile/kem-07/BRQC-128__keygen.json` |
| `BRQC-256` | KAT log (sha256 `401006efa186c1d1…`) | `kat/kem-07/BRQC-256.log` |
| `BRQC-256` | timing dec | `records/kem-07/BRQC-256__dec.json` |
| `BRQC-256` | timing enc | `records/kem-07/BRQC-256__enc.json` |
| `BRQC-256` | timing keygen | `records/kem-07/BRQC-256__keygen.json` |
| `BRQC-256` | hash profile dec | `profile/kem-07/BRQC-256__dec.json` |
| `BRQC-256` | hash profile enc | `profile/kem-07/BRQC-256__enc.json` |
| `BRQC-256` | hash profile keygen | `profile/kem-07/BRQC-256__keygen.json` |
| `BRQC-512` | KAT log (sha256 `55c93e9132eca886…`) | `kat/kem-07/BRQC-512.log` |
| `BRQC-512` | timing dec | `records/kem-07/BRQC-512__dec.json` |
| `BRQC-512` | timing enc | `records/kem-07/BRQC-512__enc.json` |
| `BRQC-512` | timing keygen | `records/kem-07/BRQC-512__keygen.json` |
| `BRQC-512` | hash profile dec | `profile/kem-07/BRQC-512__dec.json` |
| `BRQC-512` | hash profile enc | `profile/kem-07/BRQC-512__enc.json` |
| `BRQC-512` | hash profile keygen | `profile/kem-07/BRQC-512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

