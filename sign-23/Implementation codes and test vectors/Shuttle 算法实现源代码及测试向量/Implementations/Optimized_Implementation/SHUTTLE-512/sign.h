/*
 * sign.h -- top-level KeyGen / Sign / Verify orchestration for SHUTTLE.
 *
 * These three functions are the spine of the scheme: every subroutine
 * is glued together here in the EXACT order the spec algorithm
 * blocks mandate (Description.tex KeyGen alg:keygen-internal, Sign
 * alg:sign-internal, Verify alg:verify-internal).  sign.c owns NO
 * subroutine math -- it is pure control flow + the two derived-matrix
 * builds (the Sign-side full A-hat with the 2*I_m block, the Verify-side
 * narrower A1-hat without it) + the kappa-counter timing.
 *
 * The prototypes below are IDENTICAL to the NIST/SUPERCOP entry points
 * already declared in api.h (mangled per SHUTTLE_MODE x backend through
 * namespace.h).  sign.h exists so a TU can pull in the top-level contract
 * without dragging in the rest of api.h; sign.c includes api.h (which is
 * the authoritative declaration) so the two can never drift.
 *
 *   crypto_sign_keypair    -- KeyGen(xi)        : 0 on success, <0 on
 * error crypto_sign_signature  -- Sign(sk, m)       : 0 on success, <0 on
 * error crypto_sign_verify     -- Verify(pk, sig, m): 0 valid, -1 invalid
 *
 * The NGCC sig_* adapter (SIG_AlgorithmInstance.c) draws the internal
 * randomness (xi for KeyGen, rnd for Sign) from the global drng_algorithm
 * and maps these return codes onto the NGCC contract (verify reject = -1).
 *
 * By default sign.c is compiled with -DSIG_RAW, which
 * selects the fixed-length RAW signature packing (pack_sig_raw /
 * unpack_sig_raw) over the rANS codec (pack_sig / unpack_sig).  RAW
 * packing NEVER fails, so the entire KeyGen->Sign->Verify pipeline can be
 * proven correct independently of the rANS size question.  Drop
 * -DSIG_RAW to exercise the real rANS wire format (it round-trips
 * identically; only the byte length differs).
 */
#ifndef SHUTTLE_SIGN_H
#define SHUTTLE_SIGN_H

#include <stddef.h>
#include <stdint.h>

#include "api.h" /* the authoritative crypto_sign_* declarations */

/* All three are declared (and mangled) in api.h; re-stated here only as a
 * documentation anchor.  Buffers are caller-allocated:
 *   pk  : CRYPTO_PUBLICKEYBYTES
 *   sk  : CRYPTO_SECRETKEYBYTES
 *   sig : >= CRYPTO_BYTES (rANS) or SIG_RAW_PACKED_BYTES (raw); *siglen
 * set to the realized length.
 */

#endif /* SHUTTLE_SIGN_H */
