# sign-01 Aigis-Sig+ — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-01` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101567126852161536.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: Aigis-Sig+
- Implementation versions measured: reference
- Parameter sets: `Aigis-sig1`, `Aigis-sig2`, `Aigis-sig3`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-01/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Aigis-sig1` | guide | PASS |
| `Aigis-sig2` | guide | PASS |
| `Aigis-sig3` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Aigis-sig1` | keygen | 610.8 k | 292 µs | 3.43e+03 | 292 µs | 15655 (5 × 3131) |
| `Aigis-sig1` | sign | 10.50 M | 5.01 ms | 199 | 5.01 ms | 1005 (5 × 201) |
| `Aigis-sig1` | verify | 531.0 k | 254 µs | 3.94e+03 | 254 µs | 18585 (5 × 3717) |
| `Aigis-sig2` | keygen | 1.91 M | 912 µs | 1.1e+03 | 912 µs | 5235 (5 × 1047) |
| `Aigis-sig2` | sign | 7.97 M | 3.81 ms | 263 | 3.81 ms | 1310 (5 × 262) |
| `Aigis-sig2` | verify | 1.79 M | 854 µs | 1.17e+03 | 854 µs | 5750 (5 × 1150) |
| `Aigis-sig3` | keygen | 6.37 M | 3.05 ms | 328 | 3.05 ms | 1585 (5 × 317) |
| `Aigis-sig3` | sign | 8.89 M | 4.25 ms | 235 | 4.25 ms | 1175 (5 × 235) |
| `Aigis-sig3` | verify | 6.22 M | 2.97 ms | 337 | 2.97 ms | 1680 (5 × 336) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Aigis-sig1` | keygen | 47889 | 1716 KiB | 1852 KiB |
| `Aigis-sig1` | sign | 47889 | 1764 KiB | 1872 KiB |
| `Aigis-sig1` | verify | 47889 | 1752 KiB | 1880 KiB |
| `Aigis-sig2` | keygen | 48905 | 1720 KiB | 1864 KiB |
| `Aigis-sig2` | sign | 48905 | 1796 KiB | 1928 KiB |
| `Aigis-sig2` | verify | 48905 | 1788 KiB | 1920 KiB |
| `Aigis-sig3` | keygen | 48777 | 1736 KiB | 2044 KiB |
| `Aigis-sig3` | sign | 48777 | 1948 KiB | 2168 KiB |
| `Aigis-sig3` | verify | 48777 | 2052 KiB | 2164 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | signature |
|---|---|---|---|
| `Aigis-sig1` | 928 | 2800 | 2015 (maximum) |
| `Aigis-sig2` | 1824 | 4976 | 4533 (maximum) |
| `Aigis-sig3` | 4672 | 8800 | 9134 (maximum) |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only** — own fips202.c compiled but unreachable from the API

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Aigis-sig1` | keygen | 7.4% | 64% | drng 25, pseudoXOF 2 |
| `Aigis-sig1` | sign | 64% | 6.2% | drng 57, pseudoXOF 75, sm3hash 37 |
| `Aigis-sig1` | verify | 11% | 63% | drng 21, pseudoXOF 2, sm3hash 1 |
| `Aigis-sig2` | keygen | 4.4% | 73% | drng 88, pseudoXOF 2 |
| `Aigis-sig2` | sign | 55% | 18% | drng 90, pseudoXOF 45, sm3hash 11 |
| `Aigis-sig2` | verify | 5.7% | 73% | drng 80, pseudoXOF 2, sm3hash 1 |
| `Aigis-sig3` | keygen | 5.0% | 74% | drng 293, pseudoXOF 2 |
| `Aigis-sig3` | sign | 28% | 51% | drng 278, pseudoXOF 15, pseudohash 2 |
| `Aigis-sig3` | verify | 5.9% | 73% | drng 277, pseudoXOF 2, pseudohash 1 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Aigis-sig1` | KAT log (sha256 `7484d9731dcb0814…`) | `kat/sign-01/Aigis-sig1.log` |
| `Aigis-sig1` | timing keygen | `records/sign-01/Aigis-sig1__keygen.json` |
| `Aigis-sig1` | timing sign | `records/sign-01/Aigis-sig1__sign.json` |
| `Aigis-sig1` | timing verify | `records/sign-01/Aigis-sig1__verify.json` |
| `Aigis-sig1` | hash profile keygen | `profile/sign-01/Aigis-sig1__keygen.json` |
| `Aigis-sig1` | hash profile sign | `profile/sign-01/Aigis-sig1__sign.json` |
| `Aigis-sig1` | hash profile verify | `profile/sign-01/Aigis-sig1__verify.json` |
| `Aigis-sig2` | KAT log (sha256 `5f6bc4bb7c1ba8cc…`) | `kat/sign-01/Aigis-sig2.log` |
| `Aigis-sig2` | timing keygen | `records/sign-01/Aigis-sig2__keygen.json` |
| `Aigis-sig2` | timing sign | `records/sign-01/Aigis-sig2__sign.json` |
| `Aigis-sig2` | timing verify | `records/sign-01/Aigis-sig2__verify.json` |
| `Aigis-sig2` | hash profile keygen | `profile/sign-01/Aigis-sig2__keygen.json` |
| `Aigis-sig2` | hash profile sign | `profile/sign-01/Aigis-sig2__sign.json` |
| `Aigis-sig2` | hash profile verify | `profile/sign-01/Aigis-sig2__verify.json` |
| `Aigis-sig3` | KAT log (sha256 `e166474a95effefa…`) | `kat/sign-01/Aigis-sig3.log` |
| `Aigis-sig3` | timing keygen | `records/sign-01/Aigis-sig3__keygen.json` |
| `Aigis-sig3` | timing sign | `records/sign-01/Aigis-sig3__sign.json` |
| `Aigis-sig3` | timing verify | `records/sign-01/Aigis-sig3__verify.json` |
| `Aigis-sig3` | hash profile keygen | `profile/sign-01/Aigis-sig3__keygen.json` |
| `Aigis-sig3` | hash profile sign | `profile/sign-01/Aigis-sig3__sign.json` |
| `Aigis-sig3` | hash profile verify | `profile/sign-01/Aigis-sig3__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

