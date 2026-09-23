# Aigis-Sig+ writes past a stack object during signing

This package reproduces an out-of-bounds stack write in the Aigis-Sig+ reference implementation, parameter set I (Aigis-sig1). A valid signing request reaches an extra `polyz_unpack` call in `polyvecl_uniform_gamma1`. The call writes one polynomial past the end of the `polyvecl` object.

`polyvecl` contains exactly `PARAM_L` polynomials. The loop fills indices `0` through `PARAM_L - 1`. After the loop, the index equals `PARAM_L`, but the function calls `polyz_unpack` again with that index:

```c
for (i = 0; i < PARAM_L; ++i) {
    polyz_unpack(v->vec + i, outbuf);
    nonce++;
}
polyz_unpack(v->vec + i, outbuf);  /* Here, i equals PARAM_L. */
```

`polyz_unpack` writes all 512 coefficients of a polynomial. The final call therefore writes beyond the array. The signer calls this helper for the valid signing mask `y`; the input key and message do not need to be malformed.

## Run

From the NGCC harness root, run:

```sh
make -C sign-01/repro-signing-stack-overflow test
```

The target builds a normal library for the control and an AddressSanitizer library for the reproduction. The candidate build uses `-O1 -g`; the sanitizer finding is from that build. This package uses the shared API and source files in this checkout. Copying only this package directory will not build. It requires Linux, GCC, GNU make, AddressSanitizer, and POSIX process and dynamic-library functions. The package does not invoke the submitted implementation's build system.

## What the driver does

1. It loads the normal and AddressSanitizer libraries and checks that their metadata identifies `sign-01`.
2. With the normal library, it generates a key pair, signs a fixed message, and verifies the signature. This confirms that the valid input path works without the sanitizer build.
3. In a child process, it signs the same message with the same valid secret key using the AddressSanitizer library.
4. It captures the sanitizer output and counts the result as a reproduction only when the child exits with the expected sanitizer status and the report contains both `stack-buffer-overflow` and `polyvecl_uniform_gamma1`.

Expected output on the vulnerable build:

```text
signing-stack-overflow full-round-trip=passed asan-sign=asan-diagnostic
```

The driver exits successfully only for that matched diagnostic. `accepted` means the call returned without an AddressSanitizer report. A rejected signing call, timeout, unrelated sanitizer report, failed control, or child/setup failure does not confirm this defect.

## Evidence and limits

`polyvecl` contains exactly `PARAM_L` polynomials. The loop in `polyvecl_uniform_gamma1` fills indices `0` through `PARAM_L - 1`; the additional unpack uses index `PARAM_L`. `polyz_unpack` writes a full polynomial, so this is a stack out-of-bounds write during an ordinary valid signing request.

The report identifies an out-of-bounds stack write in the named helper during valid signing. A normal build can still complete the round trip because the effect depends on stack layout and compiler behavior. This reproduction does not show controlled corruption, a canary failure, code execution, or key disclosure. An attacker would need a way to cause an application to sign, and this package does not establish a remotely reachable signing service.

The driver checks the sanitizer report text and child exit status. It does not infer exploitability from the finding.

## Relevant source

- `Implementations/Implementations/Reference_Implementation/Aigis-Sig+-I/polyvec.h` — `polyvecl` array size.
- `Implementations/Implementations/Reference_Implementation/Aigis-Sig+-I/polyvec.c` — `polyvecl_uniform_gamma1` loop and extra unpack.
- `Implementations/Implementations/Reference_Implementation/Aigis-Sig+-I/poly.c` — `polyz_unpack` writes polynomial coefficients.
- `reproduce.c` — control and sanitizer driver.
- `Makefile` — builds the control and AddressSanitizer libraries.

Detected and reproduced autonomously by Askus Operator using Luna High. Verified by human review.
