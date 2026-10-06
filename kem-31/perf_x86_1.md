# kem-31 QIMEN-PIKE — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-31` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560872306036736.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: QIMEN-PIKE
- Implementation versions measured: reference
- Parameter sets: `NGCC-1`, `NGCC-2`, `NGCC-3`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-31/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `NGCC-1` | harness-default | PASS |
| `NGCC-2` | harness-default | PASS |
| `NGCC-3` | harness-default | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `NGCC-1` | keygen | 391.07 M | 187 ms | 5.34 | 187 ms | 100 (5 × 20) |
| `NGCC-1` | enc | 129.56 M | 61.9 ms | 16.2 | 61.9 ms | 100 (5 × 20) |
| `NGCC-1` | dec | 200.66 M | 96.1 ms | 10.4 | 96.1 ms | 100 (5 × 20) |
| `NGCC-2` | keygen | 1.28 G | 617 ms | 1.62 | 614 ms | 100 (5 × 20) |
| `NGCC-2` | enc | 409.71 M | 197 ms | 5.07 | 196 ms | 100 (5 × 20) |
| `NGCC-2` | dec | 638.67 M | 306 ms | 3.27 | 306 ms | 100 (5 × 20) |
| `NGCC-3` | keygen | 8.51 G | 4.09 s | 0.244 | 4.09 s | 100 (5 × 20) |
| `NGCC-3` | enc | 3.13 G | 1.5 s | 0.666 | 1.5 s | 100 (5 × 20) |
| `NGCC-3` | dec | 4.79 G | 2.3 s | 0.435 | 2.3 s | 100 (5 × 20) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `NGCC-1` | keygen | 406725 | 1964 KiB | 33016 KiB |
| `NGCC-1` | enc | 406725 | 4148 KiB | 4212 KiB |
| `NGCC-1` | dec | 406725 | 4888 KiB | 20116 KiB |
| `NGCC-2` | keygen | 521193 | 1972 KiB | 82072 KiB |
| `NGCC-2` | enc | 521193 | 6648 KiB | 6712 KiB |
| `NGCC-2` | dec | 521193 | 8584 KiB | 48224 KiB |
| `NGCC-3` | keygen | 1018501 | 2004 KiB | 319592 KiB |
| `NGCC-3` | enc | 1018501 | 20100 KiB | 20180 KiB |
| `NGCC-3` | dec | 1018501 | 27632 KiB | 184904 KiB |

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
| `NGCC-1` | keygen | 0.0% | 2.1% | drng 1.88e+03 |
| `NGCC-1` | enc | 0.0% | 0.0% | drng 1 |
| `NGCC-1` | dec | 0.0% | 0.0% | – |
| `NGCC-2` | keygen | 0.0% | 0.4% | drng 1.03e+03 |
| `NGCC-2` | enc | 0.0% | 0.0% | drng 1 |
| `NGCC-2` | dec | 0.0% | 0.0% | – |
| `NGCC-3` | keygen | 0.0% | 0.1% | drng 1.18e+03 |
| `NGCC-3` | enc | 0.0% | 0.0% | drng 1 |
| `NGCC-3` | dec | 0.0% | 0.0% | – |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `NGCC-1` | KAT log (sha256 `2c845310e7156118…`) | `kat/kem-31/NGCC-1.log` |
| `NGCC-1` | timing dec | `records/kem-31/NGCC-1__dec.json` |
| `NGCC-1` | timing enc | `records/kem-31/NGCC-1__enc.json` |
| `NGCC-1` | timing keygen | `records/kem-31/NGCC-1__keygen.json` |
| `NGCC-1` | hash profile dec | `profile/kem-31/NGCC-1__dec.json` |
| `NGCC-1` | hash profile enc | `profile/kem-31/NGCC-1__enc.json` |
| `NGCC-1` | hash profile keygen | `profile/kem-31/NGCC-1__keygen.json` |
| `NGCC-2` | KAT log (sha256 `140cf05be0fb553b…`) | `kat/kem-31/NGCC-2.log` |
| `NGCC-2` | timing dec | `records/kem-31/NGCC-2__dec.json` |
| `NGCC-2` | timing enc | `records/kem-31/NGCC-2__enc.json` |
| `NGCC-2` | timing keygen | `records/kem-31/NGCC-2__keygen.json` |
| `NGCC-2` | hash profile dec | `profile/kem-31/NGCC-2__dec.json` |
| `NGCC-2` | hash profile enc | `profile/kem-31/NGCC-2__enc.json` |
| `NGCC-2` | hash profile keygen | `profile/kem-31/NGCC-2__keygen.json` |
| `NGCC-3` | KAT log (sha256 `f903879c280b0b43…`) | `kat/kem-31/NGCC-3.log` |
| `NGCC-3` | timing dec | `records/kem-31/NGCC-3__dec.json` |
| `NGCC-3` | timing enc | `records/kem-31/NGCC-3__enc.json` |
| `NGCC-3` | timing keygen | `records/kem-31/NGCC-3__keygen.json` |
| `NGCC-3` | hash profile dec | `profile/kem-31/NGCC-3__dec.json` |
| `NGCC-3` | hash profile enc | `profile/kem-31/NGCC-3__enc.json` |
| `NGCC-3` | hash profile keygen | `profile/kem-31/NGCC-3__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

