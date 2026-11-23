No: the one Critical finding (kem-31-2) is repaired by comparing and hashing the received ciphertext bytes in decapsulation, as Algorithm 15 specifies, which changes no size and adds no work in NGCC-1, NGCC-2 or NGCC-3.

Impact: none
Scope: Function (small), code
Assessed: 2026-11-23

## Attacks considered

- kem-31-2: Non-canonical field encodings make ciphertexts malleable without changing the key (Implementation, Confirmed)

## Remediation

- **kem-31-2.** Decapsulation decodes the ciphertext and then compares and hashes a fresh encoding of the decoded object (`KEM_AlgorithmInstance.c:245,252`, through `compare_ct_for_kdf` at `:612–633` and `derive_ss_from_m_and_ct` at `:588–605`). Byte strings that decode to the same object, such as `x + p` for a field element `x` or a changed unread byte of an NGCC-1 slot, are therefore treated alike. The costed repair makes the code follow Algorithm 15 (physical p. 39): compare the received byte string with the encoding of the re-encryption (line 7), and hash the received bytes in the valid and the rejection key (lines 4, 8 and 10). A non-canonical string then differs from the re-encryption and takes the rejection path. Rejecting field elements of `p` or more and nonzero unread bytes in the parser is an alternative of the same scope. Scope: Function, code. The specification stays as it is, because Algorithm 15 already compares and hashes the received `ct`; the report notes that it defines no parser for malformed encodings, and this repair needs none. Impact: none. The ciphertext keeps its length. The received string and its re-encoding have the same length (`KEM_AlgorithmInstance.c:39–40`), so the same number of bytes is compared and hashed, and two of the three encoding calls are no longer needed. The code skipped no specified work in this step.

## Evidence

- Baseline sizes, Table 7.1 (physical p. 57), compressed variant: public key 405 / 595 / 1201 bytes and ciphertext 602 / 882 / 1746 bytes for NGCC-1 / -2 / -3. Only the compressed variant is built.
- Control: the source's ciphertext length is 4 field elements of F_p^2, 2 scalars of ⌈a/8⌉ bytes, 4 hints of 4 bytes and the masked message (`pike_compressed.h:26–31`). With slot lengths 64 / 96 / 192 (physical p. 53), a = 162 / 260 / 514 (physical p. 68; `precomp/ref/NGCC_*/include/ec_params.h:6`) and message lengths 32 / 32 / 64 (`pike_compressed.h:19–21`; `pikex_compressed/NGCC_3/CMakeLists.txt:15`): 512 + 42 + 16 + 32 = 602, 768 + 66 + 16 + 32 = 882 and 1536 + 130 + 16 + 64 = 1746. These match Table 7.1.
- The built public key is shorter than stated, 389 / 592 / 1198 bytes, and the built secret key is 472 / 734 / 1497 against the stated 488 / 737 / 1500 (headers of the locally generated `kem-31/kat/KAT_KEM_NGCC-*.txt`; also recorded in `pseudocode.md`). The repair touches neither.
- Work: Table 7.2 (physical p. 58) gives compressed decapsulation as 98.02 / 360.88 / 2845.02 million cycles. The changed step compares and hashes one ciphertext of at most 1,746 bytes.
- Specification steps: Algorithm 15 (physical p. 39); the description of `Hct` as binding the full ciphertext (§3.3.1, physical p. 38); reduced field-element serialization (Appendix B.1, physical p. 81); the 64-byte slot for a 60-byte element at NGCC-1 (physical p. 53).
- Source paths are under `Implementations/Implementations/src/`: `ngcc/common/KEM_AlgorithmInstance.c` and `pike/ref/pikex_compressed/include/pike_compressed.h`. The optimized tree has the same two routines (`Implementations/Optimized_Implementation/src/ngcc/common/KEM_AlgorithmInstance.c:588–635`).
- Not checked: nothing was built or run. The KEM wrapper also departs from Algorithms 13 to 15 in other ways that `pseudocode.md` records (no `Hpk`, one XOF call in place of `Hct` and `KDF`). They are not part of this finding and were not costed; the specified calls would hash a public key and a ciphertext of at most a few kilobytes, beside operations of 61 million cycles and more (Table 7.2).
