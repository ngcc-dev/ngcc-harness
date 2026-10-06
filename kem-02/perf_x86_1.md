# kem-02 Amoeba — performance on x86-64 (system x86_1)

[Performance x86_1](../performance/summary_x86_1.md) › `kem-02` · [method](../performance/method_x86_1.md) · [NICCS page](https://www.niccs.org.cn/niccs/Round1Additional/pc/content/content_2101560843117875200.html)

Independent measurement following the structure of the NICCS x86 self-assessment guide, §3.5 (1)–(7). Not a submitter self-assessment.

## 1. Basic information

- Category: public-key algorithm; function: key encapsulation
- Algorithm: Amoeba
- Implementation versions measured: reference
- Parameter sets: `Amoeba128`, `Amoeba192`, `Amoeba256`, `Amoeba384`, `Amoeba512`

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

Each library was checked against the submitted KAT vectors (SHA-256 manifest `kem-02/kat.sha256` in the harness) before timing.

| instance | build flags | KAT |
|---|---|---|
| `Amoeba128` | guide | PASS |
| `Amoeba192` | guide | PASS |
| `Amoeba256` | guide | PASS |
| `Amoeba384` | guide | PASS |
| `Amoeba512` | guide | PASS |

## 4. Performance

| instance | operation | mean cycles | mean time | ops/s | median trial | n (trials × iters) |
|---|---|---|---|---|---|---|
| `Amoeba128` | keygen | 379.2 k | 181 µs | 5.52e+03 | 181 µs | 26520 (5 × 5304) |
| `Amoeba128` | enc | 525.7 k | 251 µs | 3.98e+03 | 251 µs | 19350 (5 × 3870) |
| `Amoeba128` | dec | 590.4 k | 282 µs | 3.55e+03 | 282 µs | 17215 (5 × 3443) |
| `Amoeba192` | keygen | 516.8 k | 247 µs | 4.05e+03 | 247 µs | 19470 (5 × 3894) |
| `Amoeba192` | enc | 732.8 k | 350 µs | 2.86e+03 | 350 µs | 14070 (5 × 2814) |
| `Amoeba192` | dec | 832.4 k | 398 µs | 2.51e+03 | 398 µs | 12320 (5 × 2464) |
| `Amoeba256` | keygen | 622.2 k | 297 µs | 3.36e+03 | 297 µs | 12970 (5 × 2594) |
| `Amoeba256` | enc | 858.4 k | 410 µs | 2.44e+03 | 410 µs | 11880 (5 × 2376) |
| `Amoeba256` | dec | 993.2 k | 474 µs | 2.11e+03 | 474 µs | 10325 (5 × 2065) |
| `Amoeba384` | keygen | 923.9 k | 441 µs | 2.27e+03 | 441 µs | 10820 (5 × 2164) |
| `Amoeba384` | enc | 1.28 M | 610 µs | 1.64e+03 | 610 µs | 8035 (5 × 1607) |
| `Amoeba384` | dec | 1.49 M | 713 µs | 1.4e+03 | 713 µs | 6950 (5 × 1390) |
| `Amoeba512` | keygen | 1.21 M | 577 µs | 1.73e+03 | 577 µs | 8440 (5 × 1688) |
| `Amoeba512` | enc | 1.66 M | 791 µs | 1.26e+03 | 791 µs | 6250 (5 × 1250) |
| `Amoeba512` | dec | 1.94 M | 925 µs | 1.08e+03 | 925 µs | 5405 (5 × 1081) |

## 5. Resource consumption

Static memory is approximated by the library's loadable ELF segments; peak memory is the process high-water mark (VmHWM) including the benchmark driver and libc, so both are upper-bound proxies rather than isolated algorithm memory.

| instance | operation | static: ELF image (bytes) | baseline RSS | peak RSS |
|---|---|---|---|---|
| `Amoeba128` | keygen | 51409 | 1716 KiB | 1804 KiB |
| `Amoeba128` | enc | 51409 | 1704 KiB | 1832 KiB |
| `Amoeba128` | dec | 51409 | 1716 KiB | 1780 KiB |
| `Amoeba192` | keygen | 51641 | 1724 KiB | 1808 KiB |
| `Amoeba192` | enc | 51641 | 1732 KiB | 1796 KiB |
| `Amoeba192` | dec | 51641 | 1736 KiB | 1800 KiB |
| `Amoeba256` | keygen | 51489 | 1672 KiB | 1816 KiB |
| `Amoeba256` | enc | 51489 | 1732 KiB | 1848 KiB |
| `Amoeba256` | dec | 51489 | 1660 KiB | 1788 KiB |
| `Amoeba384` | keygen | 51617 | 1716 KiB | 1804 KiB |
| `Amoeba384` | enc | 51617 | 1756 KiB | 1824 KiB |
| `Amoeba384` | dec | 51617 | 1768 KiB | 1872 KiB |
| `Amoeba512` | keygen | 51649 | 1728 KiB | 1864 KiB |
| `Amoeba512` | enc | 51649 | 1800 KiB | 1884 KiB |
| `Amoeba512` | dec | 51649 | 1804 KiB | 1872 KiB |

## 6. Transmission and storage overhead

External public-key, ciphertext and signature sizes follow the curated `performance/external_sizes.csv` catalog; secret-key and shared-secret lengths remain API figures. See [the size audit](../performance/EXTERNAL_SIZE_AUDIT.md) for disagreements.

| instance | public key | secret key | ciphertext | shared secret |
|---|---|---|---|---|
| `Amoeba128` | 784 | 1712 | 1047 | 64 |
| `Amoeba192` | 1144 | 2504 | 1581 | 64 |
| `Amoeba256` | 1648 | 3440 | 1833 | 64 |
| `Amoeba384` | 2440 | 5096 | 2769 | 64 |
| `Amoeba512` | 3520 | 7040 | 3914 | 64 |

## Symmetric primitives

[Survey](../performance/symmetric-survey.md) verdict: **ICCS-only**

Share of each operation spent in the ICCS placeholder hash functions and in the ICCS DRNG (reference build, measured in the same run with link-time wrappers):

| instance | operation | pseudohash/XOF/sm3 | DRNG | calls per operation |
|---|---|---|---|---|
| `Amoeba128` | keygen | 74% | 3.2% | drng 2, pseudoXOF 20.5 |
| `Amoeba128` | enc | 71% | 1.2% | drng 1, pseudoXOF 23, sm3hash 4 |
| `Amoeba128` | dec | 63% | 0.0% | pseudoXOF 23, sm3hash 4 |
| `Amoeba192` | keygen | 73% | 2.4% | drng 2, pseudoXOF 23.5 |
| `Amoeba192` | enc | 70% | 0.9% | drng 1, pseudoXOF 26, sm3hash 4 |
| `Amoeba192` | dec | 61% | 0.0% | pseudoXOF 26, sm3hash 4 |
| `Amoeba256` | keygen | 71% | 2.0% | drng 2, pseudoXOF 26.5 |
| `Amoeba256` | enc | 67% | 0.7% | drng 1, pseudoXOF 28, sm3hash 4 |
| `Amoeba256` | dec | 58% | 0.0% | pseudoXOF 28, sm3hash 4 |
| `Amoeba384` | keygen | 71% | 1.3% | drng 2, pseudoXOF 36.5 |
| `Amoeba384` | enc | 67% | 0.5% | drng 1, pseudoXOF 38, sm3hash 4 |
| `Amoeba384` | dec | 57% | 0.0% | pseudoXOF 38, sm3hash 4 |
| `Amoeba512` | keygen | 71% | 1.0% | drng 2, pseudoXOF 46.6 |
| `Amoeba512` | enc | 67% | 0.4% | drng 1, pseudoXOF 47, sm3hash 4 |
| `Amoeba512` | dec | 57% | 0.0% | pseudoXOF 47, sm3hash 4 |

## 7. Raw evidence index

Paths are relative to `performance/data/x86_1/` in the [harness](https://github.com/ngcc-dev/ngcc-harness); each JSON file records its commands, environment and trials.

| instance | item | file |
|---|---|---|
| `Amoeba128` | KAT log (sha256 `9df784e681bd251d…`) | `kat/kem-02/Amoeba128.log` |
| `Amoeba128` | timing dec | `records/kem-02/Amoeba128__dec.json` |
| `Amoeba128` | timing enc | `records/kem-02/Amoeba128__enc.json` |
| `Amoeba128` | timing keygen | `records/kem-02/Amoeba128__keygen.json` |
| `Amoeba128` | hash profile dec | `profile/kem-02/Amoeba128__dec.json` |
| `Amoeba128` | hash profile enc | `profile/kem-02/Amoeba128__enc.json` |
| `Amoeba128` | hash profile keygen | `profile/kem-02/Amoeba128__keygen.json` |
| `Amoeba192` | KAT log (sha256 `2673db5fc79cdc84…`) | `kat/kem-02/Amoeba192.log` |
| `Amoeba192` | timing dec | `records/kem-02/Amoeba192__dec.json` |
| `Amoeba192` | timing enc | `records/kem-02/Amoeba192__enc.json` |
| `Amoeba192` | timing keygen | `records/kem-02/Amoeba192__keygen.json` |
| `Amoeba192` | hash profile dec | `profile/kem-02/Amoeba192__dec.json` |
| `Amoeba192` | hash profile enc | `profile/kem-02/Amoeba192__enc.json` |
| `Amoeba192` | hash profile keygen | `profile/kem-02/Amoeba192__keygen.json` |
| `Amoeba256` | KAT log (sha256 `fad206a4adf6e8f7…`) | `kat/kem-02/Amoeba256.log` |
| `Amoeba256` | timing dec | `records/kem-02/Amoeba256__dec.json` |
| `Amoeba256` | timing enc | `records/kem-02/Amoeba256__enc.json` |
| `Amoeba256` | timing keygen | `records/kem-02/Amoeba256__keygen.json` |
| `Amoeba256` | hash profile dec | `profile/kem-02/Amoeba256__dec.json` |
| `Amoeba256` | hash profile enc | `profile/kem-02/Amoeba256__enc.json` |
| `Amoeba256` | hash profile keygen | `profile/kem-02/Amoeba256__keygen.json` |
| `Amoeba384` | KAT log (sha256 `8b62f2f400a11253…`) | `kat/kem-02/Amoeba384.log` |
| `Amoeba384` | timing dec | `records/kem-02/Amoeba384__dec.json` |
| `Amoeba384` | timing enc | `records/kem-02/Amoeba384__enc.json` |
| `Amoeba384` | timing keygen | `records/kem-02/Amoeba384__keygen.json` |
| `Amoeba384` | hash profile dec | `profile/kem-02/Amoeba384__dec.json` |
| `Amoeba384` | hash profile enc | `profile/kem-02/Amoeba384__enc.json` |
| `Amoeba384` | hash profile keygen | `profile/kem-02/Amoeba384__keygen.json` |
| `Amoeba512` | KAT log (sha256 `93f87a85af74a92b…`) | `kat/kem-02/Amoeba512.log` |
| `Amoeba512` | timing dec | `records/kem-02/Amoeba512__dec.json` |
| `Amoeba512` | timing enc | `records/kem-02/Amoeba512__enc.json` |
| `Amoeba512` | timing keygen | `records/kem-02/Amoeba512__keygen.json` |
| `Amoeba512` | hash profile dec | `profile/kem-02/Amoeba512__dec.json` |
| `Amoeba512` | hash profile enc | `profile/kem-02/Amoeba512__enc.json` |
| `Amoeba512` | hash profile keygen | `profile/kem-02/Amoeba512__keygen.json` |

Scripts: `performance/campaign.py`, `performance/ngcc_perf.c`, `performance/hashprof/` in the [harness](https://github.com/ngcc-dev/ngcc-harness).

