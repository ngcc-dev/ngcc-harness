# sign-15 MORNING-ATLAS — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `sign-15` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101561078120534016.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: digital signature
- Algorithm: MORNING-ATLAS
- Implementation versions measured: reference
- Parameter sets: `lwrdsa128`, `lwrdsa192`, `lwrdsa256`, `lwrdsa512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `sign-15/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `lwrdsa128` | harness-default | OVERFLOW [1] |
| `lwrdsa192` | harness-default | OVERFLOW [1] |
| `lwrdsa256` | harness-default | OVERFLOW [1] |
| `lwrdsa512` | harness-default | OVERFLOW [1] |

[1] OVERFLOW: sig_sign returns (and writes) signatures up to 64 bytes longer than the declared maximum sn length (confirmed finding sign-15-1, out-of-bounds heap disclosure); timed with a 4 KiB guard buffer and the excess recorded. In addition, sig_verify accepted the honest signature once and then rejected the same unchanged inputs on later calls, so the verification time is that of calls with inconsistent verdicts (counted as verify_rejections in each record). These instances are timed anyway; their output is not validated.

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `lwrdsa128` | keygen | 2.78 M | 1.33 ms | 753 | 1.33 ms | 3620 (5 × 724) |
| `lwrdsa128` | sign | 3.89 M | 1.86 ms | 538 | 1.86 ms | 2690 (5 × 538) |
| `lwrdsa128` | verify | 2.88 M | 1.38 ms | 726 | 1.38 ms | 3605 (5 × 721) |
| `lwrdsa192` | keygen | 6.61 M | 3.16 ms | 317 | 3.16 ms | 1545 (5 × 309) |
| `lwrdsa192` | sign | 8.80 M | 4.21 ms | 238 | 4.21 ms | 945 (5 × 189) |
| `lwrdsa192` | verify | 6.81 M | 3.25 ms | 307 | 3.26 ms | 1540 (5 × 308) |
| `lwrdsa256` | keygen | 4.23 M | 2.02 ms | 495 | 2.02 ms | 2420 (5 × 484) |
| `lwrdsa256` | sign | 12.39 M | 5.92 ms | 169 | 5.91 ms | 1080 (5 × 216) |
| `lwrdsa256` | verify | 4.44 M | 2.12 ms | 472 | 2.12 ms | 2330 (5 × 466) |
| `lwrdsa512` | keygen | 12.34 M | 5.9 ms | 169 | 5.9 ms | 835 (5 × 167) |
| `lwrdsa512` | sign | 43.40 M | 20.7 ms | 48.2 | 20.8 ms | 445 (5 × 89) |
| `lwrdsa512` | verify | 12.96 M | 6.19 ms | 161 | 6.2 ms | 810 (5 × 162) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `lwrdsa128` | keygen | – | 1708 KiB | 1824 KiB |
| `lwrdsa128` | sign | – | 1752 KiB | 1980 KiB |
| `lwrdsa128` | verify | – | 1812 KiB | 1908 KiB |
| `lwrdsa192` | keygen | – | 1724 KiB | 1896 KiB |
| `lwrdsa192` | sign | – | 1804 KiB | 2000 KiB |
| `lwrdsa192` | verify | – | 1880 KiB | 1948 KiB |
| `lwrdsa256` | keygen | – | 1712 KiB | 1920 KiB |
| `lwrdsa256` | sign | – | 1824 KiB | 2000 KiB |
| `lwrdsa256` | verify | – | 1916 KiB | 2012 KiB |
| `lwrdsa512` | keygen | – | 1716 KiB | 2060 KiB |
| `lwrdsa512` | sign | – | 1964 KiB | 2260 KiB |
| `lwrdsa512` | verify | – | 2152 KiB | 2220 KiB |

## 6. Transmission and storage overhead

| instance | public key | secret key | signature |
|---|---|---|---|
| `lwrdsa128` | 1328 | 2128 | 2081 |
| `lwrdsa192` | 2112 | 3152 | 3365 |
| `lwrdsa256` | 2848 | 4016 | 4656 |
| `lwrdsa512` | 6688 | 7920 | 10081 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `lwrdsa128` | keygen | 83% | 0.2% | drng 1, pseudoXOF 62 |
| `lwrdsa128` | sign | 70% | 0.0% | pseudoXOF 62 |
| `lwrdsa128` | verify | 81% | 0.0% | pseudoXOF 57 |
| `lwrdsa192` | keygen | 83% | 0.1% | drng 1, pseudoXOF 142 |
| `lwrdsa192` | sign | 71% | 0.0% | pseudoXOF 143 |
| `lwrdsa192` | verify | 82% | 0.0% | pseudoXOF 134 |
| `lwrdsa256` | keygen | 61% | 0.1% | drng 1, pseudoXOF 72 |
| `lwrdsa256` | sign | 35% | 0.0% | pseudoXOF 81 |
| `lwrdsa256` | verify | 58% | 0.0% | pseudoXOF 59 |
| `lwrdsa512` | keygen | 58% | 0.0% | drng 1, pseudoXOF 86 |
| `lwrdsa512` | sign | 46% | 0.0% | pseudoXOF 227 |
| `lwrdsa512` | verify | 55% | 0.0% | pseudoXOF 59 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `lwrdsa128` | KAT log (sha256 `702480bc43313e49…`) | `kat/sign-15/lwrdsa128.log` |
| `lwrdsa128` | timing keygen | `records/sign-15/lwrdsa128__keygen.json` |
| `lwrdsa128` | timing sign | `records/sign-15/lwrdsa128__sign.json` |
| `lwrdsa128` | timing verify | `records/sign-15/lwrdsa128__verify.json` |
| `lwrdsa128` | hash profile keygen | `profile/sign-15/lwrdsa128__keygen.json` |
| `lwrdsa128` | hash profile sign | `profile/sign-15/lwrdsa128__sign.json` |
| `lwrdsa128` | hash profile verify | `profile/sign-15/lwrdsa128__verify.json` |
| `lwrdsa192` | KAT log (sha256 `ebe44b51dc5aa3cc…`) | `kat/sign-15/lwrdsa192.log` |
| `lwrdsa192` | timing keygen | `records/sign-15/lwrdsa192__keygen.json` |
| `lwrdsa192` | timing sign | `records/sign-15/lwrdsa192__sign.json` |
| `lwrdsa192` | timing verify | `records/sign-15/lwrdsa192__verify.json` |
| `lwrdsa192` | hash profile keygen | `profile/sign-15/lwrdsa192__keygen.json` |
| `lwrdsa192` | hash profile sign | `profile/sign-15/lwrdsa192__sign.json` |
| `lwrdsa192` | hash profile verify | `profile/sign-15/lwrdsa192__verify.json` |
| `lwrdsa256` | KAT log (sha256 `426000e406d37405…`) | `kat/sign-15/lwrdsa256.log` |
| `lwrdsa256` | timing keygen | `records/sign-15/lwrdsa256__keygen.json` |
| `lwrdsa256` | timing sign | `records/sign-15/lwrdsa256__sign.json` |
| `lwrdsa256` | timing verify | `records/sign-15/lwrdsa256__verify.json` |
| `lwrdsa256` | hash profile keygen | `profile/sign-15/lwrdsa256__keygen.json` |
| `lwrdsa256` | hash profile sign | `profile/sign-15/lwrdsa256__sign.json` |
| `lwrdsa256` | hash profile verify | `profile/sign-15/lwrdsa256__verify.json` |
| `lwrdsa512` | KAT log (sha256 `9aef369292d1cd22…`) | `kat/sign-15/lwrdsa512.log` |
| `lwrdsa512` | timing keygen | `records/sign-15/lwrdsa512__keygen.json` |
| `lwrdsa512` | timing sign | `records/sign-15/lwrdsa512__sign.json` |
| `lwrdsa512` | timing verify | `records/sign-15/lwrdsa512__verify.json` |
| `lwrdsa512` | hash profile keygen | `profile/sign-15/lwrdsa512__keygen.json` |
| `lwrdsa512` | hash profile sign | `profile/sign-15/lwrdsa512__sign.json` |
| `lwrdsa512` | hash profile verify | `profile/sign-15/lwrdsa512__verify.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

