# kem-15 FLIT — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-15` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560844812374016.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: FLIT
- Implementation versions measured: reference
- Parameter sets: `FLIT128_REF`, `FLIT256_REF`, `FLIT512_REF`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-15/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `FLIT128_REF` | guide | PASS |
| `FLIT256_REF` | guide | PASS |
| `FLIT512_REF` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `FLIT128_REF` | keygen | 140.0 k | 66.9 µs | 1.49e+04 | 66.9 µs | 69615 (5 × 13923) |
| `FLIT128_REF` | enc | 121.3 k | 57.9 µs | 1.73e+04 | 57.9 µs | 76440 (5 × 15288) |
| `FLIT128_REF` | dec | 156.9 k | 75 µs | 1.33e+04 | 74.9 µs | 59630 (5 × 11926) |
| `FLIT256_REF` | keygen | 348.2 k | 166 µs | 6.01e+03 | 166 µs | 30815 (5 × 6163) |
| `FLIT256_REF` | enc | 255.5 k | 122 µs | 8.19e+03 | 122 µs | 38830 (5 × 7766) |
| `FLIT256_REF` | dec | 362.5 k | 173 µs | 5.77e+03 | 173 µs | 27475 (5 × 5495) |
| `FLIT512_REF` | keygen | 1.41 M | 674 µs | 1.48e+03 | 674 µs | 6540 (5 × 1308) |
| `FLIT512_REF` | enc | 943.2 k | 451 µs | 2.22e+03 | 450 µs | 10400 (5 × 2080) |
| `FLIT512_REF` | dec | 1.26 M | 602 µs | 1.66e+03 | 602 µs | 7995 (5 × 1599) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `FLIT128_REF` | keygen | 31873 | 1684 KiB | 1800 KiB |
| `FLIT128_REF` | enc | 31873 | 1656 KiB | 1784 KiB |
| `FLIT128_REF` | dec | 31873 | 1716 KiB | 1780 KiB |
| `FLIT256_REF` | keygen | 32321 | 1684 KiB | 1804 KiB |
| `FLIT256_REF` | enc | 32321 | 1720 KiB | 1816 KiB |
| `FLIT256_REF` | dec | 32321 | 1720 KiB | 1808 KiB |
| `FLIT512_REF` | keygen | 31745 | 1724 KiB | 1832 KiB |
| `FLIT512_REF` | enc | 31745 | 1740 KiB | 1824 KiB |
| `FLIT512_REF` | dec | 31745 | 1736 KiB | 1804 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `FLIT128_REF` | 615 | 1351 | 512 | 32 |
| `FLIT256_REF` | 1229 | 2605 | 1024 | 32 |
| `FLIT512_REF` | 3072 | 6336 | 2304 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `FLIT128_REF` | keygen | 31% | 6.6% | drng 2, pseudoXOF 2.98, sm3hash 1 |
| `FLIT128_REF` | enc | 52% | 3.8% | drng 1, pseudoXOF 3, pseudohash 1, sm3hash 3 |
| `FLIT128_REF` | dec | 31% | 0.0% | pseudoXOF 3, pseudohash 1, sm3hash 1 |
| `FLIT256_REF` | keygen | 28% | 2.7% | drng 2, pseudoXOF 3.01, sm3hash 1 |
| `FLIT256_REF` | enc | 42% | 1.8% | drng 1, pseudoXOF 3, pseudohash 1, sm3hash 3 |
| `FLIT256_REF` | dec | 22% | 0.0% | pseudoXOF 3, pseudohash 1, sm3hash 1 |
| `FLIT512_REF` | keygen | 29% | 0.9% | drng 2, pseudoXOF 3.03, pseudohash 1 |
| `FLIT512_REF` | enc | 52% | 0.7% | drng 1, pseudoXOF 3, pseudohash 4 |
| `FLIT512_REF` | dec | 27% | 0.0% | pseudoXOF 3, pseudohash 2 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `FLIT128_REF` | KAT log (sha256 `d2603a7876bbd60f…`) | `kat/kem-15/FLIT128_REF.log` |
| `FLIT128_REF` | timing dec | `records/kem-15/FLIT128_REF__dec.json` |
| `FLIT128_REF` | timing enc | `records/kem-15/FLIT128_REF__enc.json` |
| `FLIT128_REF` | timing keygen | `records/kem-15/FLIT128_REF__keygen.json` |
| `FLIT128_REF` | hash profile dec | `profile/kem-15/FLIT128_REF__dec.json` |
| `FLIT128_REF` | hash profile enc | `profile/kem-15/FLIT128_REF__enc.json` |
| `FLIT128_REF` | hash profile keygen | `profile/kem-15/FLIT128_REF__keygen.json` |
| `FLIT256_REF` | KAT log (sha256 `8f01977fc7f306e3…`) | `kat/kem-15/FLIT256_REF.log` |
| `FLIT256_REF` | timing dec | `records/kem-15/FLIT256_REF__dec.json` |
| `FLIT256_REF` | timing enc | `records/kem-15/FLIT256_REF__enc.json` |
| `FLIT256_REF` | timing keygen | `records/kem-15/FLIT256_REF__keygen.json` |
| `FLIT256_REF` | hash profile dec | `profile/kem-15/FLIT256_REF__dec.json` |
| `FLIT256_REF` | hash profile enc | `profile/kem-15/FLIT256_REF__enc.json` |
| `FLIT256_REF` | hash profile keygen | `profile/kem-15/FLIT256_REF__keygen.json` |
| `FLIT512_REF` | KAT log (sha256 `959bf686d8aadbde…`) | `kat/kem-15/FLIT512_REF.log` |
| `FLIT512_REF` | timing dec | `records/kem-15/FLIT512_REF__dec.json` |
| `FLIT512_REF` | timing enc | `records/kem-15/FLIT512_REF__enc.json` |
| `FLIT512_REF` | timing keygen | `records/kem-15/FLIT512_REF__keygen.json` |
| `FLIT512_REF` | hash profile dec | `profile/kem-15/FLIT512_REF__dec.json` |
| `FLIT512_REF` | hash profile enc | `profile/kem-15/FLIT512_REF__enc.json` |
| `FLIT512_REF` | hash profile keygen | `profile/kem-15/FLIT512_REF__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

