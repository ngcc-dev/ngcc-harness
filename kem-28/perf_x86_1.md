# kem-28 OAEP-NTRU — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-28` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560871911772160.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: OAEP-NTRU
- Implementation versions measured: reference
- Parameter sets: `OAEP-NTRU-648`, `OAEP-NTRU-1296`, `OAEP-NTRU-2592`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-28/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `OAEP-NTRU-648` | guide | PASS |
| `OAEP-NTRU-1296` | guide | PASS |
| `OAEP-NTRU-2592` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `OAEP-NTRU-648` | keygen | 179.1 k | 85.6 µs | 1.17e+04 | 85.6 µs | 51885 (5 × 10377) |
| `OAEP-NTRU-648` | enc | 124.8 k | 59.6 µs | 1.68e+04 | 59.6 µs | 75045 (5 × 15009) |
| `OAEP-NTRU-648` | dec | 127.9 k | 61.1 µs | 1.64e+04 | 61.1 µs | 72730 (5 × 14546) |
| `OAEP-NTRU-1296` | keygen | 493.8 k | 236 µs | 4.24e+03 | 236 µs | 19995 (5 × 3999) |
| `OAEP-NTRU-1296` | enc | 431.5 k | 206 µs | 4.85e+03 | 206 µs | 23500 (5 × 4700) |
| `OAEP-NTRU-1296` | dec | 339.0 k | 162 µs | 6.17e+03 | 162 µs | 30015 (5 × 6003) |
| `OAEP-NTRU-2592` | keygen | 1.19 M | 567 µs | 1.76e+03 | 567 µs | 8515 (5 × 1703) |
| `OAEP-NTRU-2592` | enc | 1.12 M | 533 µs | 1.88e+03 | 533 µs | 9210 (5 × 1842) |
| `OAEP-NTRU-2592` | dec | 955.5 k | 456 µs | 2.19e+03 | 456 µs | 10880 (5 × 2176) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `OAEP-NTRU-648` | keygen | 31913 | 1716 KiB | 1784 KiB |
| `OAEP-NTRU-648` | enc | 31913 | 1680 KiB | 1804 KiB |
| `OAEP-NTRU-648` | dec | 31913 | 1704 KiB | 1808 KiB |
| `OAEP-NTRU-1296` | keygen | 31885 | 1720 KiB | 1800 KiB |
| `OAEP-NTRU-1296` | enc | 31885 | 1724 KiB | 1820 KiB |
| `OAEP-NTRU-1296` | dec | 31885 | 1716 KiB | 1816 KiB |
| `OAEP-NTRU-2592` | keygen | 35173 | 1728 KiB | 1836 KiB |
| `OAEP-NTRU-2592` | enc | 35173 | 1740 KiB | 1860 KiB |
| `OAEP-NTRU-2592` | dec | 35173 | 1764 KiB | 1840 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `OAEP-NTRU-648` | 1053 | 2138 | 1085 | 32 |
| `OAEP-NTRU-1296` | 2430 | 4924 | 2494 | 32 |
| `OAEP-NTRU-2592` | 4860 | 9848 | 4988 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `OAEP-NTRU-648` | keygen | 13% | 13% | drng 2, sm3hash 1 |
| `OAEP-NTRU-648` | enc | 52% | 10% | drng 1, pseudoXOF 2, sm3hash 1 |
| `OAEP-NTRU-648` | dec | 33% | 0.0% | pseudoXOF 2 |
| `OAEP-NTRU-1296` | keygen | 23% | 7.9% | drng 2, pseudohash 1 |
| `OAEP-NTRU-1296` | enc | 59% | 4.5% | drng 1, pseudoXOF 2, pseudohash 1 |
| `OAEP-NTRU-1296` | dec | 42% | 0.0% | pseudoXOF 2 |
| `OAEP-NTRU-2592` | keygen | 20% | 5.7% | drng 2, pseudohash 1 |
| `OAEP-NTRU-2592` | enc | 66% | 3.1% | drng 1, pseudoXOF 2, pseudohash 1 |
| `OAEP-NTRU-2592` | dec | 53% | 0.0% | pseudoXOF 2 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `OAEP-NTRU-648` | KAT log (sha256 `886ccac7024b239f…`) | `kat/kem-28/OAEP-NTRU-648.log` |
| `OAEP-NTRU-648` | timing dec | `records/kem-28/OAEP-NTRU-648__dec.json` |
| `OAEP-NTRU-648` | timing enc | `records/kem-28/OAEP-NTRU-648__enc.json` |
| `OAEP-NTRU-648` | timing keygen | `records/kem-28/OAEP-NTRU-648__keygen.json` |
| `OAEP-NTRU-648` | hash profile dec | `profile/kem-28/OAEP-NTRU-648__dec.json` |
| `OAEP-NTRU-648` | hash profile enc | `profile/kem-28/OAEP-NTRU-648__enc.json` |
| `OAEP-NTRU-648` | hash profile keygen | `profile/kem-28/OAEP-NTRU-648__keygen.json` |
| `OAEP-NTRU-1296` | KAT log (sha256 `cd9f7c70eb0e9a17…`) | `kat/kem-28/OAEP-NTRU-1296.log` |
| `OAEP-NTRU-1296` | timing dec | `records/kem-28/OAEP-NTRU-1296__dec.json` |
| `OAEP-NTRU-1296` | timing enc | `records/kem-28/OAEP-NTRU-1296__enc.json` |
| `OAEP-NTRU-1296` | timing keygen | `records/kem-28/OAEP-NTRU-1296__keygen.json` |
| `OAEP-NTRU-1296` | hash profile dec | `profile/kem-28/OAEP-NTRU-1296__dec.json` |
| `OAEP-NTRU-1296` | hash profile enc | `profile/kem-28/OAEP-NTRU-1296__enc.json` |
| `OAEP-NTRU-1296` | hash profile keygen | `profile/kem-28/OAEP-NTRU-1296__keygen.json` |
| `OAEP-NTRU-2592` | KAT log (sha256 `fd28bf1da00f1daf…`) | `kat/kem-28/OAEP-NTRU-2592.log` |
| `OAEP-NTRU-2592` | timing dec | `records/kem-28/OAEP-NTRU-2592__dec.json` |
| `OAEP-NTRU-2592` | timing enc | `records/kem-28/OAEP-NTRU-2592__enc.json` |
| `OAEP-NTRU-2592` | timing keygen | `records/kem-28/OAEP-NTRU-2592__keygen.json` |
| `OAEP-NTRU-2592` | hash profile dec | `profile/kem-28/OAEP-NTRU-2592__dec.json` |
| `OAEP-NTRU-2592` | hash profile enc | `profile/kem-28/OAEP-NTRU-2592__enc.json` |
| `OAEP-NTRU-2592` | hash profile keygen | `profile/kem-28/OAEP-NTRU-2592__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

