# kem-01 Aigis-Enc+ — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-01` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560842975268864.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: Aigis-Enc+
- Implementation versions measured: reference
- Parameter sets: `Aigis-enc1`, `Aigis-enc2`, `Aigis-enc3`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-01/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Aigis-enc1` | guide | PASS |
| `Aigis-enc2` | guide | PASS |
| `Aigis-enc3` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Aigis-enc1` | keygen | 269.2 k | 129 µs | 7.78e+03 | 129 µs | 35300 (5 × 7060) |
| `Aigis-enc1` | enc | 331.4 k | 158 µs | 6.32e+03 | 158 µs | 30135 (5 × 6027) |
| `Aigis-enc1` | dec | 373.2 k | 178 µs | 5.61e+03 | 178 µs | 26820 (5 × 5364) |
| `Aigis-enc2` | keygen | 448.1 k | 214 µs | 4.67e+03 | 214 µs | 21835 (5 × 4367) |
| `Aigis-enc2` | enc | 602.4 k | 288 µs | 3.47e+03 | 288 µs | 16975 (5 × 3395) |
| `Aigis-enc2` | dec | 705.1 k | 337 µs | 2.97e+03 | 337 µs | 14445 (5 × 2889) |
| `Aigis-enc3` | keygen | 1.01 M | 482 µs | 2.08e+03 | 482 µs | 9835 (5 × 1967) |
| `Aigis-enc3` | enc | 1.35 M | 644 µs | 1.55e+03 | 644 µs | 7565 (5 × 1513) |
| `Aigis-enc3` | dec | 1.61 M | 770 µs | 1.3e+03 | 770 µs | 6380 (5 × 1276) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Aigis-enc1` | keygen | 40689 | 1720 KiB | 1792 KiB |
| `Aigis-enc1` | enc | 40689 | 1680 KiB | 1808 KiB |
| `Aigis-enc1` | dec | 40689 | 1728 KiB | 1820 KiB |
| `Aigis-enc2` | keygen | 45033 | 1720 KiB | 1828 KiB |
| `Aigis-enc2` | enc | 45033 | 1748 KiB | 1812 KiB |
| `Aigis-enc2` | dec | 45033 | 1732 KiB | 1836 KiB |
| `Aigis-enc3` | keygen | 51537 | 1720 KiB | 1844 KiB |
| `Aigis-enc3` | enc | 51537 | 1768 KiB | 1840 KiB |
| `Aigis-enc3` | dec | 51537 | 1788 KiB | 1872 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `Aigis-enc1` | 656 | 1456 | 896 | 16 |
| `Aigis-enc2` | 1312 | 2912 | 1664 | 32 |
| `Aigis-enc3` | 2624 | 5824 | 3584 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — own fips202.c compiled but unreachable from the API

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Aigis-enc1` | keygen | 30% | 37% | drng 15, pseudoXOF 3, sm3hash 1 |
| `Aigis-enc1` | enc | 35% | 28% | drng 13, pseudoXOF 4, sm3hash 1 |
| `Aigis-enc1` | dec | 33% | 23% | drng 12, pseudoXOF 4, sm3hash 1 |
| `Aigis-enc2` | keygen | 30% | 36% | drng 23, pseudoXOF 2, pseudohash 1, sm3hash 1 |
| `Aigis-enc2` | enc | 35% | 26% | drng 21, pseudoXOF 3, pseudohash 1, sm3hash 1 |
| `Aigis-enc2` | dec | 31% | 21% | drng 20, pseudoXOF 4, pseudohash 1 |
| `Aigis-enc3` | keygen | 38% | 31% | drng 42.8, pseudoXOF 2, pseudohash 2 |
| `Aigis-enc3` | enc | 39% | 23% | drng 41, pseudoXOF 3, pseudohash 2 |
| `Aigis-enc3` | dec | 35% | 19% | drng 40, pseudoXOF 4, pseudohash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Aigis-enc1` | KAT log (sha256 `b11498bb40d35ef1…`) | `kat/kem-01/Aigis-enc1.log` |
| `Aigis-enc1` | timing dec | `records/kem-01/Aigis-enc1__dec.json` |
| `Aigis-enc1` | timing enc | `records/kem-01/Aigis-enc1__enc.json` |
| `Aigis-enc1` | timing keygen | `records/kem-01/Aigis-enc1__keygen.json` |
| `Aigis-enc1` | hash profile dec | `profile/kem-01/Aigis-enc1__dec.json` |
| `Aigis-enc1` | hash profile enc | `profile/kem-01/Aigis-enc1__enc.json` |
| `Aigis-enc1` | hash profile keygen | `profile/kem-01/Aigis-enc1__keygen.json` |
| `Aigis-enc2` | KAT log (sha256 `0e34c80db17af3ba…`) | `kat/kem-01/Aigis-enc2.log` |
| `Aigis-enc2` | timing dec | `records/kem-01/Aigis-enc2__dec.json` |
| `Aigis-enc2` | timing enc | `records/kem-01/Aigis-enc2__enc.json` |
| `Aigis-enc2` | timing keygen | `records/kem-01/Aigis-enc2__keygen.json` |
| `Aigis-enc2` | hash profile dec | `profile/kem-01/Aigis-enc2__dec.json` |
| `Aigis-enc2` | hash profile enc | `profile/kem-01/Aigis-enc2__enc.json` |
| `Aigis-enc2` | hash profile keygen | `profile/kem-01/Aigis-enc2__keygen.json` |
| `Aigis-enc3` | KAT log (sha256 `576d178dade126a3…`) | `kat/kem-01/Aigis-enc3.log` |
| `Aigis-enc3` | timing dec | `records/kem-01/Aigis-enc3__dec.json` |
| `Aigis-enc3` | timing enc | `records/kem-01/Aigis-enc3__enc.json` |
| `Aigis-enc3` | timing keygen | `records/kem-01/Aigis-enc3__keygen.json` |
| `Aigis-enc3` | hash profile dec | `profile/kem-01/Aigis-enc3__dec.json` |
| `Aigis-enc3` | hash profile enc | `profile/kem-01/Aigis-enc3__enc.json` |
| `Aigis-enc3` | hash profile keygen | `profile/kem-01/Aigis-enc3__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

