No: the one Critical finding is repaired by having decapsulation compare and hash the received ciphertext bytes, which changes no size and adds no work in CMultiURAG-512 or in the other two sets.

Impact: none
Scope: Function (small), specification and code
Assessed: 2026-11-23

## Attacks considered

- kem-10-3: Ignored padding bits make CMultiURAG-512 ciphertexts malleable without changing the key (Implementation, Confirmed)

## Remediation

- **kem-10-3.** Root cause: at Level-512 the packed `U` and `V` each end with 5 unused bits that the parser drops (`rbc_mat.c:434-444`, `rbc_vec.c:789-809`), and decapsulation then compares and hashes re-serialized copies of the parsed `U` and `V` (`kem.c:235-246`, `kem.c:259-260`). The received bytes are never compared. The costed repair is the usual one for a non-canonical encoding, which is to reject it: `cmultiurag_decaps` compares the received bytes of `U` and `V` with the serialized re-encryption and hashes the received bytes into `K`. A ciphertext with a nonzero unused bit then takes the implicit-rejection branch. This is the comparison of Algorithm 9, line 4 (physical p. 13), applied to the byte string. The specification fixes the lengths at ⌈N2·n·m/8⌉ and ⌈N1·N2·m/8⌉ bytes (Table 3 note, physical p. 14) and has no rule for the unused bits, so it gains one encoding rule: the unused bits are zero and the comparison is on bytes. Scope: Function, specification and code; the code change is confined to `cmultiurag_decaps`, in a `kem.c` whose compared and hashed lines are the same in all six trees. The Level-128 and Level-256 encodings have no unused bits, so the change has no effect there. Impact: none. Honest ciphertexts are unchanged, because the serializer already writes zero in the unused bits (`rbc_vec.c:761`), so no transmitted object changes. Decapsulation compares the same number of bytes and hashes an input of the same length, and the four re-serializations of the parsed values (`kem.c:235-236`, `kem.c:259-260`) give way to the received bytes. The code did not skip specified work at this step: it performs the Algorithm 9 comparison and both hash calls, on the wrong bytes. The report records no proposed fix for this finding; the team response linked under kem-10-6 does not mention the encoding. Not checked: nothing was built or run; the optimized tree was read by source only.

## Evidence

- Baseline, Table 3 (physical p. 14): pks = ⌈m·n·N1/8⌉ + 64 and cts = ⌈N2·n·m/8⌉ + ⌈N1·N2·m/8⌉ + 64 bytes. With (m, n, N1, N2) from Table 2 (physical p. 14): Level-128 (79, 35, 11, 16), pk 3802 + 64 = 3866, ct 5530 + 1738 + 64 = 7332; Level-256 (127, 45, 15, 16), pk 10716 + 64 = 10780, ct 11430 + 3810 + 64 = 15304; Level-512 (181, 67, 19, 21), pk 28802 + 64 = 28866, ct 31834 + 9028 + 64 = 40926. These match Table 3 and `parameters.h:10,12,29,30` in each tree. The repair changes none of them.
- Unused bits at Level-512: `U` has 21·67·181 = 254667 bits in 31834 bytes, and 8·31834 − 254667 = 5; `V` has 19·21·181 = 72219 bits in 9028 bytes, and 8·9028 − 72219 = 5. At Level-128 and Level-256, N2 = 16 makes both bit counts multiples of 8 (44240 and 13904; 91440 and 30480).
- Source, under `Implementations/Reference_Implementation/CMultiURAG-512/src/`: `kem.c:203` parses the ciphertext; `kem.c:235-236` re-serialize the parsed `U` and `V`; `kem.c:244-246` compare them with the re-encryption; `kem.c:259-260` put the re-serialized values into the input of `H`; `rbc-181/rbc_mat.c:434-444` and `rbc-181/rbc_vec.c:789-809` are the parser and `rbc-181/rbc_vec.c:761` zeroes the output of the serializer. `kem.c` and `rbc-181/rbc_vec.c` are byte-identical between the reference and optimized trees (`cmp`).
- Team response read for proposed fixes: https://list.niccs.org.cn/archives/list/pkcforum@list.niccs.org.cn/message/USDY6YVIGB3YM3F4T2KRLUU4KTMCHAZR/ (linked from the report's kem-10-6 "Proposed fixes"). It covers length validation, loop bounds and malformed inputs.
