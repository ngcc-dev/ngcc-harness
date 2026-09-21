#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "loom_loc.h"
#include "api.h"
#include "prf_mac.h"
#include "params.h"
#include "../kem/kem.h"
#include "../sig/sign.h"

#ifndef LOOM_USE_SHAKE
// USE SM3:
#include "drng.h"
extern DRNG_ctx drng_algorithm;
int randombytes(unsigned char *x, unsigned long long xlen)
{
    return get_random_number(&drng_algorithm, x, xlen * 8);
}
#else
#include "rng.h"
#endif

/*************************************************
* Name:        build_DTBS
*
* Description: Build the Data-To-Be-Signed (DTBS) for authentication.
*
* Arguments:   - const LOOM_KEX_CTX *ctx: inner state
*              - int       sigmode:  1 for sign; 0 for verify
*              - uint8_t *dtbs_out:  output buffer (LOOM_DTBSBYTES)
**************************************************/
static size_t build_DTBS(const LOOM_KEX_CTX *ctx, int sigmode, uint8_t *dtbs_out)
{
    size_t offset = 0;
    memcpy(dtbs_out + offset, ctx->spi, LOOM_SPIBYTES);
    offset += LOOM_SPIBYTES;
    if(sigmode) {
        /* sigmode == 1: (Algorithm 4,5: AuthInit/AuthResp)
        *  Initiator build Data-To-Be-Signed_i: sig_msg = 0 | SPI | (ek, ect)
        *  Responder build Data-To-Be-Signed_r: sig_msg = 1 | SPI | (ect, ek)
        */
        dtbs_out[offset] = (uint8_t )(1-ctx->initiator);
        offset += 1;
        memcpy(dtbs_out + offset, ctx->self_exch, ctx->self_exchlen);
        offset += ctx->self_exchlen;
        memcpy(dtbs_out + offset, ctx->peer_exch, ctx->peer_exchlen);
        offset += ctx->peer_exchlen;
        return offset;
    }
    else { 
        /* sigmode == 0: (Algorithm 5,6: AuthResp/Confirm)
        *  Responder build Data-To-Be-Signed_i: sig_msg = 0 | SPI | (ek, ect)
        *  Initiator build Data-To-Be-Signed_r: sig_msg = 1 | SPI | (ect, ek)
        */
        dtbs_out[offset] = (uint8_t )ctx->initiator;
        offset += 1;
        memcpy(dtbs_out + offset, ctx->peer_exch, ctx->peer_exchlen);
        offset += ctx->peer_exchlen;
        memcpy(dtbs_out + offset, ctx->self_exch, ctx->self_exchlen);
        offset += ctx->self_exchlen;
        return offset;
    }
}

static int prf_derive_loc(LOOM_KEX_CTX *ctx) 
{
    int ret = 0;
    uint8_t *mki, *mkr;
    const uint8_t *ni, *nr;

    if(ctx->initiator == LOOM_INITIATOR) {
        mki = ctx->authenticator.mk_self;
        mkr = ctx->authenticator.mk_peer;
        ni = ctx->self_nonce;
        nr = ctx->peer_nonce;
    } else {
        mki = ctx->authenticator.mk_peer;
        mkr = ctx->authenticator.mk_self;
        ni = ctx->peer_nonce;
        nr = ctx->self_nonce;
    }

    ret = loom_prf_derive(ctx->secret.ss, mki, mkr, ctx->secret.keymat, ni, nr);
    return ret;
}


static int mac_compute_loc(LOOM_KEX_CTX *ctx, uint8_t* output) 
{ // len(output) == LOOM_MACBYTES.
    uint8_t buf[LOOM_MACINPUT_BUFLEN];
    size_t offset = 0;
    /* MAC Input Format: 
    *  For Initiator: 0 | SPI | ID_i
    *  For Responder: 1 | SPI | ID_r
    */
    buf[0] = (uint8_t)(1-ctx->initiator);
    offset += 1;
    memcpy(buf + offset, ctx->spi, LOOM_SPIBYTES);
    offset += LOOM_SPIBYTES;
    memcpy(buf + offset, ctx->authenticator.self_id, ctx->authenticator.self_id_len);
    offset += ctx->authenticator.self_id_len;

    return loom_mac_compute(output, ctx->authenticator.mk_self, buf, offset);
}

static int loom_id_check(const uint8_t *id, size_t idlen)
{
    if (!id || idlen == 0 || idlen > LOOM_MAX_IDBYTES)
        return -1;
    return 0;
}

static int loom_auth_parse(const uint8_t *msg,
                           const uint8_t **id,
                           size_t *idlen,
                           size_t *siglen,
                           const uint8_t **sigma,
                           const uint8_t **tau)
{
    size_t ilen, slen;

    ilen = (size_t)msg[0] | ((size_t)msg[1] << 8);
    if (ilen == 0 || ilen > LOOM_MAX_IDBYTES)
        return -1;

    slen = (size_t)msg[LOOM_IDLENBYTES + ilen] | ((size_t)msg[LOOM_IDLENBYTES + ilen + 1] << 8);
    if (slen > LOOM_SIG_MAXBYTES)
        return -1;

    if (LOOM_IDLENBYTES + ilen + LOOM_SIGLENBYTES + LOOM_SIG_MAXBYTES + LOOM_MACBYTES > LOOM_MSG3BYTES)
        return -1;

    *id = msg + LOOM_IDLENBYTES;
    *idlen = ilen;
    *siglen = slen;
    *sigma = msg + LOOM_IDLENBYTES + ilen + LOOM_SIGLENBYTES;
    *tau = msg + LOOM_IDLENBYTES + ilen + LOOM_SIGLENBYTES + slen;
    return 0;
}

static int loom_auth_write(uint8_t *msg,
                           const uint8_t *id,
                           size_t idlen,
                           size_t siglen,
                           const uint8_t *sig)
{
    if (loom_id_check(id, idlen))
        return -1;
    if (siglen > LOOM_SIG_MAXBYTES)
        return -1;

    memset(msg, 0, LOOM_MSG3BYTES);
    msg[0] = (uint8_t)(idlen & 0xff);
    msg[1] = (uint8_t)(idlen >> 8);
    memcpy(msg + LOOM_IDLENBYTES, id, idlen);
    msg[LOOM_IDLENBYTES + idlen] = (uint8_t)(siglen & 0xff);
    msg[LOOM_IDLENBYTES + idlen + 1] = (uint8_t)(siglen >> 8);
    memcpy(msg + LOOM_IDLENBYTES + idlen + LOOM_SIGLENBYTES, sig, siglen);
    return 0;
}

static int loom_auth_ver(LOOM_KEX_CTX *ctx,
                          uint8_t *sigbuf,
                          const unsigned char *pk_peer,
                          const unsigned char *id_peer,
                          size_t id_peer_len,
                          const unsigned char *msgauth) 
{ // msgauth = msg3 or msg4
    const uint8_t *idread;
    size_t idreadlen, siglen, sig_msglen;
    const uint8_t *sigma, *tau;
    int ret = 0;
    uint8_t buf[LOOM_MACINPUT_BUFLEN];
    size_t offset = 0;

    /* Peer ID verification */
    ret = loom_auth_parse(msgauth, &idread, &idreadlen, &siglen, &sigma, &tau);
    if (ret) {
        ret = -1;
        goto end;
    }
    if (idreadlen != id_peer_len || memcmp(idread, id_peer, id_peer_len) != 0) {
        ret = -4; // write failure
        goto end;
    }

    sig_msglen = build_DTBS(ctx, 0, sigbuf);
    if (crypto_sign_verify(sigma, siglen, sigbuf, sig_msglen, pk_peer)) {
        ret = -10; // sig ver failure
        goto end;
    }

    if(ctx->initiator != 1) { /* for responder */
        ret = prf_derive_loc(ctx);
        if (ret) {
            ret = -4; // write failure
            goto end;
        }
    }

    /* MAC Verification Format: 
    *  For Initiator: 1 | SPI | ID_expected
    *  For Responder: 0 | SPI | ID_expected
    */
    buf[0] = (uint8_t)ctx->initiator;
    offset += 1;
    memcpy(buf + offset, ctx->spi, LOOM_SPIBYTES);
    offset += LOOM_SPIBYTES;
    memcpy(buf + offset, idread, idreadlen);
    offset += idreadlen;

    ret = loom_mac_verify(tau, ctx->authenticator.mk_peer, buf, offset);
    if (ret) {
        ret = -11; // mac ver failure
        goto end;
    }
    ret = 0;
end:
    return ret;
}

LOOM_KEX_CTX * create_ctx(void) {
    LOOM_KEX_CTX* ctx = (LOOM_KEX_CTX*)malloc(sizeof(LOOM_KEX_CTX));
    if (ctx) {
        memset(ctx, 0, sizeof(LOOM_KEX_CTX));
    }
    return ctx;
}

int destroy_ctx(LOOM_KEX_CTX *ctx) {
    free(ctx->authenticator.self_id);
    free(ctx);
    return 0;
}

/*************************************************
* Name:        crypto_loom_keypair
*
* Description: AKE.KeyGen (PDF Algorithm 1): long-term SIG keys.
*
* Arguments:   - uint8_t *pk: output signature public key
*                            (LOOM_PKBYTES)
*              - uint8_t *sk: output signature secret key
*                            (LOOM_SKBYTES)
*
* Returns 0 on success; negative on SIG.KGen failure.
**************************************************/
int crypto_loom_initialize_state(LOOM_KEX_CTX *ctx, unsigned char *pk, unsigned char *sk, int initiator, const uint8_t* idself, size_t idlen)
{
    unsigned char xi[SEEDBYTES];
    int ret = -1;
    if (!ctx || !pk || !sk)
    {
        goto end;
    }
    if (!idself || idlen == 0 || idlen > LOOM_MAX_IDBYTES) {
        goto end;
    }
    memset(ctx, 0, sizeof(LOOM_KEX_CTX));

    ctx->initiator = initiator;
    ctx->authenticator.self_id = malloc(idlen);
    if (!ctx->authenticator.self_id)
    {
        goto end;
    }

    memcpy(ctx->authenticator.self_id, idself, idlen);
    ctx->authenticator.self_id_len = idlen;

    randombytes(xi, SEEDBYTES);
    ret = crypto_sign_keypair_xi(pk, sk, xi);
    if (ret)
    {
        goto end;
    }
    ctx->authenticator.self_pubkey = pk;
    ctx->authenticator.self_privkey = sk;
    ctx->state = -1; // initialized
    ret = 0;
end:
    return ret;
}

/*************************************************
* Name:        1 crypto_loom_init
*
* Description: AKE.Init (PDF Algorithm 2): initiator samples NI,
*              runs KEM.KGen, sends msg1 = (spi_i,0) || pkkem || NI.
*
* Arguments:   - LOOM_KEX_CTX *ctx: output state (NI, skkem)
*              - uint8_t *msg1: output message (LOOM_MSG1BYTES)
*
* Returns 0 on success.
**************************************************/
int crypto_loom_init(LOOM_KEX_CTX *ctx, unsigned char *msg1)
{
    int ret = -1;

    if (!ctx || !msg1) {
        goto end;
    }
    if (ctx->state != -1 || ctx->initiator != 1) {
        ret = -2;
        goto end;
    }
    randombytes(ctx->spi, LOOM_SPIHALFBYTES); // (spi_i || 0)
    randombytes(ctx->self_nonce, LOOM_NONCEBYTES);
    ret = crypto_kem_keypair(ctx->self_exch, ctx->secret.skkem);
    if (ret) {
        goto end;
    }
    ctx->self_exchlen = LOOM_KEM_PKBYTES;
    memcpy(msg1, ctx->spi, LOOM_SPIBYTES);
    memcpy(msg1 + LOOM_SPIBYTES, ctx->self_exch, ctx->self_exchlen);
    memcpy(msg1 + LOOM_SPIBYTES + ctx->self_exchlen, ctx->self_nonce, LOOM_NONCEBYTES);
    ctx->state = 1; // sent msg1
    ret = 0;
end:
    return ret;
}

/*************************************************
* Name:        2 crypto_loom_respond
*
* Description: AKE.Respond (PDF Algorithm 3): parse msg1, sample spi_r, NR,
*              KEM.Encap(pkkem), send msg2 = SPI || ctkem || NR.
*
* Arguments:   - LOOM_KEX_CTX *ctx: output state
*              - const uint8_t *msg1: pass1 initiator message
*              - uint8_t *msg2: output message (LOOM_MSG2BYTES)
*
* Returns 0 on success.
**************************************************/
int crypto_loom_respond(LOOM_KEX_CTX *ctx,
                        const unsigned char *msg1,
                        unsigned char *msg2)
{
    int ret = -1;
    if (!ctx || !msg1 || !msg2)
    {
        goto end;
    }
    if (ctx->state != -1 || ctx->initiator != 0)
    {
        ret = -2;
        goto end;
    }
    ctx->peer_exchlen = LOOM_KEM_PKBYTES;
    memcpy(ctx->spi, msg1, LOOM_SPIBYTES);
    memcpy(ctx->peer_exch, msg1 + LOOM_SPIBYTES, ctx->peer_exchlen);
    memcpy(ctx->peer_nonce, msg1 + LOOM_SPIBYTES + ctx->peer_exchlen, LOOM_NONCEBYTES);

    randombytes(ctx->spi + LOOM_SPIHALFBYTES, LOOM_SPIHALFBYTES); // (spi_i || spi_r)
    randombytes(ctx->self_nonce, LOOM_NONCEBYTES);                // NR
    ret = crypto_kem_enc(ctx->self_exch, ctx->secret.keymat, ctx->peer_exch);
    if (ret)
    {
        ret = -3; // kem failure
        goto end;
    }
    ctx->self_exchlen = LOOM_KEM_CTBYTES;
    memcpy(msg2, ctx->spi, LOOM_SPIBYTES);
    memcpy(msg2 + LOOM_SPIBYTES, ctx->self_exch, ctx->self_exchlen);
    memcpy(msg2 + LOOM_SPIBYTES + ctx->self_exchlen, ctx->self_nonce, LOOM_NONCEBYTES);
    ctx->state = 2; // sent msg2
    ret = 0;
end:
    return ret;
}

/*************************************************
* Name:       3  crypto_loom_auth_init
*
* Description: AKE.AuthInit (PDF Algorithm 4): rigid KEM.Decap;
*              PRF(K', NI||NR) -> SS, Kmac; sign/MAC for initiator.
*              Wire msg3: idlen || IDI || siglen || sigma_I || tau_I.
*
*
* Returns 0 on success, -1 on failure.
**************************************************/
int crypto_loom_auth_init(LOOM_KEX_CTX *ctx,
                          const unsigned char *msg2,
                          unsigned char *msg3)
{
    unsigned char rnd[RNDBYTES];
    uint8_t sig_msg[LOOM_DTBS_BUFLEN];
    uint8_t sig[LOOM_SIG_MAXBYTES];
    size_t siglen = 0, sig_msglen = 0;
    int ret = -1;

    if (!ctx || !msg2 || !msg3)
    {
        goto end;
    }
    if (ctx->state != 1 || ctx->initiator != 1)
    { // Initiator must have sent msg1 and be in state 1
        ret = -2;
        goto end;
    }
    if (memcmp(ctx->spi, msg2, LOOM_SPIHALFBYTES) != 0)
    { // check: received prev(spi) == spi_i
        ret = -10; // auth failure
        goto end;
    }
    else
    { // reset spi to (spi_i || spi_r)
        memcpy(ctx->spi, msg2, LOOM_SPIBYTES);
    }

    ctx->peer_exchlen = LOOM_KEM_CTBYTES;
    memcpy(ctx->peer_exch, msg2 + LOOM_SPIBYTES, ctx->peer_exchlen);
    memcpy(ctx->peer_nonce, msg2 + LOOM_SPIBYTES + ctx->peer_exchlen, LOOM_NONCEBYTES);

    ret = crypto_kem_dec_rigid(ctx->secret.keymat, ctx->peer_exch, ctx->secret.skkem);
    if (ret) {
        ret = -3; // kem failure
        goto end;
    }

    ret = prf_derive_loc(ctx); /* for initiator */
    if (ret) {
        ret = -4; // write failure
        goto end;
    }

    // build Data-To-Be-Signed_i: sig_msg = SPI || (ek, NI, NR)
    sig_msglen = build_DTBS(ctx, 1, sig_msg);
    randombytes(rnd, RNDBYTES);
    ret = crypto_sign_signature_rnd(sig, &siglen, sig_msg, sig_msglen, ctx->authenticator.self_privkey, rnd);
    if (ret) {
        ret = -4; // write failure
        goto end;
    }

    if (loom_auth_write(msg3, ctx->authenticator.self_id, ctx->authenticator.self_id_len, siglen, sig))
    {
        ret = -4; // write failure
        goto end;
    }

    ret = mac_compute_loc(ctx, (msg3 + LOOM_MSG3BYTES - (LOOM_MAX_IDBYTES - ctx->authenticator.self_id_len) - LOOM_MACBYTES));
    if(ret) {
        ret = -4; // write failure
        goto end;
    }
    ctx->state = 3; // sent msg3
    ret = 0;
end:
    return ret;
}

/*************************************************
* Name:        4 crypto_loom_auth_resp
*
* Description: AKE.AuthResp (PDF Algorithm 5). Wire msg4 same as msg3.
*              id_init/id_init_len must match initiator ID parsed from msg3.
**************************************************/
int crypto_loom_auth_resp(LOOM_KEX_CTX *ctx,
                          const unsigned char *pk_peer,
                          const unsigned char *id_peer,
                          size_t id_peer_len,
                          const unsigned char *msg3,
                          unsigned char *msg4)
{
    unsigned char rnd[RNDBYTES];
    size_t sig_msglen, siglen_out;
    uint8_t sig_msg[LOOM_DTBS_BUFLEN];
    uint8_t sig[LOOM_SIG_MAXBYTES];
    int ret = -1;
    if (!ctx || !pk_peer || !id_peer || !msg3 || !msg4)
    {
        goto end;
    }
    if (ctx->state != 2 || ctx->initiator != 0)
    { // Responder must have sent msg2 and be in state 2
        ret = -2;
        goto end;
    }

    ret = loom_auth_ver(ctx, sig_msg, pk_peer, id_peer, id_peer_len, msg3);
    if(ret) {
        goto end;
    }
    
    // build Data-To-Be-Signed_r: sig_msg = SPI || (ct, NR, NI)
    sig_msglen = build_DTBS(ctx, 1, sig_msg);
    randombytes(rnd, RNDBYTES);
    ret = crypto_sign_signature_rnd(sig, &siglen_out, sig_msg, sig_msglen, ctx->authenticator.self_privkey, rnd);
    if (ret) {
        ret = -4; // write failure
        goto end;
    }

    if (loom_auth_write(msg4, ctx->authenticator.self_id, ctx->authenticator.self_id_len, siglen_out, sig)) {
        ret = -4; // write failure
        goto end;
    }

    ret = mac_compute_loc(ctx, (msg4 + LOOM_MSG4BYTES - (LOOM_MAX_IDBYTES - ctx->authenticator.self_id_len) - LOOM_MACBYTES));
    if (ret) {
        ret = -4; // write failure
        goto end;
    }

    ctx->state = 4; // ready to finalize & derive session key
    ret = 0;
end:
    return ret;
}

/*************************************************
* Name:        crypto_loom_last_init
*
* Description: AKE.Confirm (PDF Algorithm 6). idb/idblen must match
*              responder ID parsed from msg4.
**************************************************/
int crypto_loom_last_init(LOOM_KEX_CTX *ctx,
                          const unsigned char *pk_peer,
                          const unsigned char *id_peer,
                          size_t id_peer_len,
                          const unsigned char *msg4)
{
    uint8_t sig_msg[LOOM_DTBS_BUFLEN];
    int ret = -1;

    if (!ctx || !pk_peer || !id_peer || !msg4) {
        goto end;
    }

    if (ctx->state != 3 || ctx->initiator != 1)
    { // Initiator must have sent msg3 and be in state 3
        ret = -2;
        goto end;
    }

    ret = loom_auth_ver(ctx, sig_msg, pk_peer, id_peer, id_peer_len, msg4);
    if(ret) {
        goto end;
    }

    ctx->state = 4; // ready to finalize & derive session key
    ret = 0;
end:
    return ret;
}

int crypto_loom_finalize(const LOOM_KEX_CTX *ctx,
                         unsigned char *ss, size_t *ss_len)
{
    int ret = -1;
    if(!ctx || !ss || !ss_len) {
        goto end;
    }
    if(ctx->state != 4) {
        ret = -2;
        goto end;
    }
    memcpy(ss, ctx->secret.ss, LOOM_KEM_SSBYTES);
    *ss_len = LOOM_KEM_SSBYTES;
    // call destroy_ctx to free the ctx.
    ret = 0;
end:
    return ret;
}