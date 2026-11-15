# UOV-7 — Low: crypto_sign_open loops forever on a rejected input or a message of 2^32 bytes or more

Severity: Low
Status: Confirmed
Layer: Implementation
Affected: crypto_sign_open in all 60 reference and optimized directories (identical sign.c; reference uov-Ip run)
Discovery: Trivial
Exploitation: Denial of service through the submitted API: a rejected input of 2^32 bytes, or a valid signed message of 2^32 message bytes, makes the call run without end; no memory corruption
Remediation: Local
Credit: Markku-Juhani O. Saarinen <markku-juhani.saarinen@tuni.fi>, with AI assistance
Date: 2026-11-15

## Defect

`crypto_sign_open` copies and clears the message with loops whose index is a
32-bit `unsigned`, compared against the 64-bit lengths `mlen` and `smlen`
(`sign.c:137` (`UOV/submission/UOV-R3/Reference_Implementation/Ip/sign.c#L137`), with the loops at lines 147 and 157). When a
length reaches `2^32` the index wraps to 0 before the comparison fails, so
the loop never ends. The same `sign.c` is in all 60 reference and optimized
directories.

## Evidence

The reproducer builds reference uov-Ip with gcc `-O3` and runs each call in a
child process with a timeout of at least 60 s and four times the matching
control's time.

- Reject path: an all-zero input of `2^32 − 1` bytes is rejected in about
  15 s, while the same call on `2^32` bytes had not returned after 60 s and
  kept rewriting the first output byte.
- Accept path: a valid signed message of `2^32 − 1` message bytes opens
  correctly in about 14 s; one of `2^32` message bytes had not returned after
  60 s. A valid input hangs only when its message part reaches `2^32` bytes,
  because that loop runs to the message length; a rejected input hangs when
  the whole input does.
- Controls: `crypto_sign_verify` alone rejects the `2^32` byte input, and a
  temporary copy with a 64-bit index rejects it in about 14 s.

## Classification and limits

Low / Confirmed / Implementation: denial of service through the submitted API
for a rejected input of `2^32` bytes or more, or a valid signed message whose
message part is `2^32` bytes or more. There is no out-of-bounds access, because
the index stays below `2^32`. "Never returns" is observed up to the 60 s
timeout and supported by the repeated rewrites. Only reference uov-Ip with
gcc was built.

Declare the index as `unsigned long long` or `size_t`.

## Reproduction

```sh
python3 UOV/reproduce_uov_7.py
```

The reproducer needs about 8 GiB of free memory and runs for about
3.5 minutes.

## Harness reproduction

From the `ngcc-harness` checkout, run the pinned source script through the harness-side dispatcher:

```sh
python3 nist/reproduce.py UOV-7
```
