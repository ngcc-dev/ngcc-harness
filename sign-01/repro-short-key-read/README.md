# Aigis-Sig+ reads beyond short key buffers

This package reproduces an out-of-bounds read in the Aigis-Sig+ reference implementation, parameter set I (Aigis-sig1). The ICCS entry points receive public-key and secret-key lengths, but the wrapper does not check those lengths before it unpacks the keys.

For Aigis-sig1, the implementation advertises a 928-byte public key and a 2800-byte secret key. The wrapper passes the key pointer to the signing or verification routine without passing its length:

```c
int sig_sign(unsigned char *sk, unsigned long long sk_len_bytes,
             unsigned char *m, unsigned long long m_len_bytes,
             unsigned char *sn, unsigned long long *sn_len_bytes)
{
    return msig_sign(sk, m, m_len_bytes, sn, sn_len_bytes);
}

int sig_verify(unsigned char *pk, unsigned long long pk_len_bytes,
               unsigned char *sn, unsigned long long sn_len_bytes,
               unsigned char *m, unsigned long long m_len_bytes)
{
    return msig_verf(pk, sn, sn_len_bytes, m, m_len_bytes);
}
```

The `sk_len_bytes` and `pk_len_bytes` parameters are unused. The underlying routines unpack fixed-size key fields from the supplied pointers.

## Run

From the NGCC harness root, run:

```sh
make -C sign-01/repro-short-key-read test
```

The target builds the reproducer and the Aigis-sig1 harness library, then runs the driver. It uses the shared API and source files in this checkout. Copying only this package directory will not build. It requires Linux, GCC, GNU make, AddressSanitizer for the driver, and POSIX `mmap`, `mprotect`, `fork`, and `waitpid` support. The library itself is built without AddressSanitizer.

## What the driver does

1. It loads the harness library and checks its signature metadata.
2. It generates a valid key pair, signs a fixed message, and verifies the signature. This is the full-size control.
3. In separate child processes, it calls `sig_sign` with a one-byte secret key and `sk_len_bytes == 0`, then `sig_verify` with a one-byte public key and `pk_len_bytes == 0`.
4. For each call, it places the byte at the end of a readable memory page and protects the next page. A read beyond that byte reaches the protected page and produces a guard fault. Separate children let the driver report both outcomes.

Expected output on the vulnerable build:

```text
short-key-read full=accepted short_sign=guard-fault short_verify=guard-fault
CONFIRMED: truncated key buffers are unpacked
```

The driver exits successfully only when the full-size control passes and both short-key calls fault. `rejected` means the call returned an error without a guard fault; `accepted` means it returned success. `setup-failed` or a failed control is inconclusive. A fixed implementation should reject short keys before reading beyond their buffers.

## Evidence and limits

The guard-page result demonstrates that each call accesses memory beyond the one-byte key object. The source shows why: both entry points ignore the supplied key length, and the unpack routines read the fixed-format key.

This test does not show that an attacker can disclose adjacent memory, control execution, or reach a network service. A crash is possible when an application passes an attacker-controlled short key to these entry points. The test does not establish that such an application exists.

The reproducer uses a protected page instead of AddressSanitizer for the library read. The valid signing control cannot use an AddressSanitizer library because the separate signing defect in `polyvecl_uniform_gamma1` triggers during normal signing.

## Relevant source

- `Implementations/Implementations/Reference_Implementation/Aigis-Sig+-I/SIG_AlgorithmInstance.c` — ICCS wrappers and key lengths.
- `Implementations/Implementations/Reference_Implementation/Aigis-Sig+-I/sign.c` and `packing.c` — fixed-size key unpacking.
- `reproduce.c` — guard-page driver.
- `Makefile` — harness build and run commands.

Detected and reproduced autonomously by Askus Operator using Luna High. Verified by human review.
