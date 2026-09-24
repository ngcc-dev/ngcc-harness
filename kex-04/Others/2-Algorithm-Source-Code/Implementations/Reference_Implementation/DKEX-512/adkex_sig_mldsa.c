/*
ADKEX (KEX + SIG) — ML-DSA adapter (pq-crystals reference, FIPS 204).

Thin shim from the backend-agnostic `adkex_sig_*` API into the
upstream `crypto_sign_*` API. Selected by `-DADKEX_SIG_BACKEND_MLDSA
-DADKEX_SIG_MLDSA_LEVEL={2,3,5}` at compile time; the level macro is
forwarded to the upstream source via `-DDILITHIUM_MODE=<level>`.

Determinism strategy:
  - Keygen randomness:   coins -> randombytes() via the hook below.
  - Signing randomness:  zero RND, called through crypto_sign_signature_internal.
  - Verify:              pure function.
*/

#include <string.h>
#include <stddef.h>
#include <stdint.h>

#include "adkex_sig.h"

#include "dilithium/sign.h"
#include "dilithium/params.h"

/* ---------- Coin injection ---------- */
/* randombytes.c forwards every request here. We return 0 if we satisfied
   the request from injected coins; -1 makes randombytes.c fall back to
   the global DRNG. */

static const uint8_t *g_coin_buf = NULL;
static unsigned long long g_coin_remaining = 0;

int adkex_sig_random_hook(uint8_t *out, unsigned long long outlen)
{
    if (g_coin_buf == NULL || outlen > g_coin_remaining) return -1;
    memcpy(out, g_coin_buf, (size_t)outlen);
    g_coin_buf += outlen;
    g_coin_remaining -= outlen;
    return 0;
}

static void coins_inject(const uint8_t *buf, unsigned long long len)
{
    g_coin_buf = buf;
    g_coin_remaining = len;
}

static void coins_clear(void)
{
    g_coin_buf = NULL;
    g_coin_remaining = 0;
}

/* ---------- adkex_sig API ---------- */

void adkex_sig_keygen(
    uint8_t       pk   [ADKEX_SIG_PKBITS   / 8],
    uint8_t       sk   [ADKEX_SIG_SKBITS   / 8],
    const uint8_t coins[ADKEX_SIG_COINBITS / 8])
{
    coins_inject(coins, ADKEX_SIG_COINBITS / 8);
    (void)crypto_sign_keypair(pk, sk);
    coins_clear();
}

void adkex_sig_sign(
    uint8_t       sigma[ADKEX_SIG_SNBITS / 8],
    const uint8_t sk   [ADKEX_SIG_SKBITS / 8],
    const uint8_t *msg, unsigned long long msg_bits)
{
    /* Deterministic sign via the _internal entrypoint with rnd = 0.
       The prefix encodes the (empty) context per FIPS 204 §5.4. */
    uint8_t pre[2] = { 0, 0 };           /* domain-byte 0, ctxlen 0 */
    uint8_t rnd[RNDBYTES];
    size_t siglen = 0;
    memset(rnd, 0, sizeof rnd);

    (void)crypto_sign_signature_internal(
        sigma, &siglen,
        msg, (size_t)(msg_bits / 8),
        pre, sizeof pre,
        rnd,
        sk);

    /* If the underlying scheme produced a shorter signature than our
       constant SNBYTES (ML-DSA does not; siglen == CRYPTO_BYTES), pad
       the rest with zeros so the wire format is fixed. */
    if (siglen < (ADKEX_SIG_SNBITS / 8)) {
        memset(sigma + siglen, 0, (ADKEX_SIG_SNBITS / 8) - siglen);
    }
}

int adkex_sig_verify(
    const uint8_t pk   [ADKEX_SIG_PKBITS / 8],
    const uint8_t sigma[ADKEX_SIG_SNBITS / 8],
    const uint8_t *msg, unsigned long long msg_bits)
{
    uint8_t pre[2] = { 0, 0 };
    int rc = crypto_sign_verify_internal(
        sigma, (size_t)(ADKEX_SIG_SNBITS / 8),
        msg, (size_t)(msg_bits / 8),
        pre, sizeof pre,
        pk);
    return (rc == 0) ? 1 : 0;
}
