Yes: generating the ephemeral key in every session, as Fig. 3 specifies, moves each composite public key from the long-term key into the messages and adds one EKeyGen to each party's session, at all three parameter sets.

Impact: protocol messages +106.5% at AFS-KEX-C256 (count); each party's session steps +26% to +33% at AFS-KEX-C256 and AFS-KEX-C512 (estimate)
Scope: Sub-algorithm (large), specification and code
Assessed: 2026-11-23

## Attacks considered

- kex-02-1: The ephemeral key is generated once as long-term state (Implementation, Confirmed)

## Remediation

- **kex-02-1.** The root cause is that `kex_init_self` draws `seed_e`, runs `EKeyGen` and stores `cpk` and `csk` in the long-term public and secret keys, so every session under one key reuses one ephemeral key. The costed repair is the protocol of Fig. 3 (physical p. 10): each party draws its seed and runs `EKeyGen` in its own first pass, sends `cpk` in that pass, keeps `csk` in the session state, and erases it afterwards. No cheaper repair removes the root cause, because the peer cannot hold a fresh `cpk` before the session, so it has to be sent. The four passes of the interface carry Fig. 3 as it is: `cpk_A`; `(cpk_B, ct_A)`; `(ct_B, c_seed^A)`; `c_seed^B`. The report records no proposed fix. Scope: Sub-algorithm, specification and code. The protocol wrapper `KEX_AlgorithmInstance.c` is rewritten (long-term key format, session state, the order of encapsulation and all four messages); the BW-KEM code is not touched. Fig. 3 stays, but the NGCC-interface paragraph of §4 (physical p. 26), which assumes that `cpk` is obtained before the handshake, and Table 6 with the text that describes it (physical p. 27) change. Impact: the messages of one session total `2|pk| + 2|ct| + 2λ/8` = 3,136, 6,080 and 12,288 bytes, against 1,568, 2,944 and 6,016 in Table 6: +100%, +106.5% and +104.3% (count). Each party's session work gains one `EKeyGen`: +22% to +33% at C128 and +26% to +33% at C256 and C512, by the count of polynomial operations (estimate). The long-term public key halves (1,568 to 784 bytes at C128) and key generation loses the same `EKeyGen`; these savings are not offset against the increases.

  Two other baselines give no increase and are recorded here. Table 4 Panel A (physical p. 19) already states 3,136, 6,080 and 12,288 bytes for pMAKE-BW as drawn in Fig. 3, so the repaired messages equal that table. Table 5 (physical p. 27) times one complete exchange including `kex_init`, and that total does not change, because the `EKeyGen` only moves from `kex_init` into a pass. The answer rests on Table 6 and on the per-session steps, which is what a party pays when a long-term key serves more than one session.

  Not checked: the AVX2 trees, where §4 says the protocol layer is kept identical, and the Cortex-M4 tree; the session-state size after the repair; identity strings, which Table 4 also omits (the interface names the peer by the static public key that it passes as an argument).

## Evidence

- Baseline sizes, Table 6 (physical p. 27): public key 1,568 / 3,136 / 6,272; passes 768 + 784 + 16 = 1,568, 1,440 + 1,472 + 32 = 2,944, 2,944 + 3,008 + 64 = 6,016. Table 1 (physical p. 17): `|pk|` 784 / 1,568 / 3,136, `|ct|` 768 / 1,440 / 2,944, λ/8 = 16 / 32 / 64, k = 2 / 4 / 4.
- Control: Table 6 is `2|pk|` for the public key and `2|ct| + 2λ/8` for the messages: 2·768 + 32 = 1,568, 2·1,440 + 64 = 2,944, 2·2,944 + 128 = 6,016. The source returns the same (`AFS_KEX_C128/KEX_AlgorithmInstance.c:48`, `:53`).
- Repaired messages: `2|pk| + 2|ct| + 2λ/8` = 2·784 + 2·768 + 32 = 3,136; 2·1,568 + 2·1,440 + 64 = 6,080; 2·3,136 + 2·2,944 + 128 = 12,288. These equal the pMAKE-BW column of Table 4 Panel A. Ratios to Table 6: 3,136/1,568 = 2.000, 6,080/2,944 = 2.065, 12,288/6,016 = 2.043. At C128 the four passes become 784, 1,552, 784 and 16 bytes (Table 6: 768, 784, 16, 0).
- Source of the defect: `AFS_KEX_C128/KEX_AlgorithmInstance.c:69–74` (seed and `indcpa_keypair_ekeygen` inside `kex_init_self`), `:98–103` (`cpk`, `csk` and the seed written to the long-term keys and state). The file is identical in the C256 and C512 trees.
- Work of one party in one session, submitted code: one `Encaps` (`indcpa_enc`), one `Decaps` (`indcpa_dec` and a re-encryption, `kem.c:155–162`) and one `EKeyGen` to check the peer (`KEX_AlgorithmInstance.c:154`, `:217`, `:240` for the initiator). The repair adds one `EKeyGen` (`indcpa.c:243–275`: one matrix expansion, 2k noise polynomials, 2k NTTs, k² products). A party's passes and derivation are counted together, because the repair also moves work between passes when the order of encapsulation changes.

  | per party and session | submitted | repaired | k = 2 | k = 4 |
  |---|---|---|---|---|
  | uniform polynomials expanded | 3k² | 4k² | +33.3% | +33.3% |
  | noise polynomials sampled | 6k + 2 | 8k + 2 | +28.6% | +30.8% |
  | polynomial products | 3k² + 3k | 4k² + 3k | +22.2% | +26.7% |
  | forward and inverse NTTs | 7k + 3 | 9k + 3 | +23.5% | +25.8% |

  The counts are read from `indcpa_enc` (`indcpa.c:293–332`), `indcpa_dec` (`:347–365`) and `indcpa_keypair_ekeygen` in the C128 tree; the C256 and C512 routines have the same shape. The FO hashes (`kem.c:93`, `:159`, `:167`), the session PRF and the Barnes–Wall codec do not change and are not in the table, so a party's steps as a whole rise somewhat less than the table shows. Nothing was measured.
- Specification claim that the submitted code departs from: fresh ephemeral KEM key material for each session, erased after use (physical pp. 7 and 23).
