No: the one Critical finding is repaired by having decapsulation compare and hash the received ciphertext bytes, which changes no size and adds no work in any BRQC parameter set.

Impact: none
Scope: Function (small), specification and code
Assessed: 2026-11-23

## Attacks considered

- kem-07-2: Ignored padding bits make ciphertexts malleable without changing the key (Implementation, Confirmed)

## Remediation

- **kem-07-2.** Root cause: each packed vector ends with unused bits that the parser drops (`rbc_vec.c:789-809`), and decapsulation then compares and hashes re-serialized copies of the parsed `u` and `v` (`kem.c:241-252`, `kem.c:265-266`). The received bytes are never compared. The costed repair is the usual one for a non-canonical encoding, which is to reject it: `brqc_decaps` compares the received bytes of `u` and `v` with the serialized re-encryption and hashes the received bytes into `K`. A ciphertext with a nonzero unused bit then takes the implicit-rejection branch. This is the comparison of Algorithm 9, line 4 (physical p. 14), applied to the byte string. The specification fixes the vector length at ⌈mn/8⌉ bytes (Table 3 note, physical p. 15) and has no rule for the unused bits, so it gains one encoding rule: the unused bits are zero and the comparison is on bytes. Scope: Function, specification and code; the code change is confined to `brqc_decaps`, on the same lines in all six trees. Impact: none. Honest ciphertexts are unchanged, because the serializer already writes zero in the unused bits (`rbc_vec.c:761`), so no transmitted object changes. Decapsulation compares the same 2·⌈mn/8⌉ bytes and hashes an input of the same length, and the four re-serializations of the parsed values (`kem.c:241-242`, `kem.c:265-266`) give way to the received bytes. The code did not skip specified work at this step: it performs the Algorithm 9 comparison and both hash calls, on the wrong bytes. The report records no proposed fix for this finding; the team's plan recorded under kem-07-4 covers length validation, degree handling, the allocation check and error handling, not the encoding. Not checked: nothing was built or run; the optimized trees were read by source only; the team response itself was not read.

## Evidence

- Baseline, Table 3 (physical p. 15): pks = ⌈mn/8⌉ + 64 and cts = 2⌈mn/8⌉ + 64 bytes. With (m, n) from Table 2 (physical p. 15): BRQC-128 (127, 119), mn = 15113 bits, 1890 bytes per vector, pk 1954, ct 3844; BRQC-256 (163, 161), 26243 bits, 3281 bytes, pk 3345, ct 6626; BRQC-512 (229, 227), 51983 bits, 6498 bytes, pk 6562, ct 13060. These match Table 3 and `parameters.h:10,12,26` in each tree. The repair changes none of them.
- Unused bits per vector: 8·1890 − 15113 = 7, 8·3281 − 26243 = 5 and 8·6498 − 51983 = 1, as in the report.
- Source, under `Implementations/Reference_Implementation/BRQC-128/src/`: `kem.c:208` parses the ciphertext; `kem.c:241-242` re-serialize the parsed `u` and `v`; `kem.c:250-252` compare them with the re-encryption; `kem.c:265-266` put the re-serialized values into the input of `H`; `rbc-127/rbc_vec.c:789-809` is the parser and `rbc-127/rbc_vec.c:761` zeroes the output of the serializer. The same lines hold in BRQC-256 (`rbc-163`) and BRQC-512 (`rbc-229`). In each set `kem.c` and `rbc_vec.c` are byte-identical between the reference and optimized trees (`cmp`).
