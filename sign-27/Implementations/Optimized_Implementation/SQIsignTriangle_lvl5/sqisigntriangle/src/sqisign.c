#include <sig.h>
#include <string.h>
#include <encoded_sizes.h>
#include <verification.h>
#if defined(ENABLE_SIGN)
#include <signature.h>
#endif

#if defined(ENABLE_SIGN)
SQISIGN_API
int
sqisign_keypair(unsigned char *pk, unsigned char *sk)
{
    int ret = 0;
    secret_key_t skt;
    public_key_t pkt = { 0 };
    secret_key_init(&skt);

    ret = !protocols_keygen(&pkt, &skt);

    secret_key_to_bytes(sk, &skt, &pkt);
    public_key_to_bytes(pk, &pkt);
    secret_key_finalize(&skt);
    return ret;
}

SQISIGN_API
int
sqisign_sign(unsigned char *sm,
             unsigned long long *smlen,
             const unsigned char *m,
             unsigned long long mlen,
             const unsigned char *sk)
{
    int ret = 0;
    secret_key_t skt;
    public_key_t pkt = { 0 };
    new_signature_t sigt;
    secret_key_init(&skt);
    secret_key_from_bytes(&skt, &pkt, sk);

    ret = !new_protocols_sign(&sigt, &pkt, &skt, m, mlen);
    if (ret != 0) {
        *smlen = 0;
        goto err;
    }

    memmove(sm + NEW_SIGNATURE_BYTES, m, mlen);
    new_signature_to_bytes(sm, &sigt);
    *smlen = NEW_SIGNATURE_BYTES + mlen;

err:
    secret_key_finalize(&skt);
    return ret;
}
#endif

SQISIGN_API
int
sqisign_open(unsigned char *m,
             unsigned long long *mlen,
             const unsigned char *sm,
             unsigned long long smlen,
             const unsigned char *pk)
{
    int ret = 0;
    public_key_t pkt = { 0 };
    new_signature_t sigt;

    if (smlen < NEW_SIGNATURE_BYTES) {
        *mlen = 0;
        return 1;
    }

    public_key_from_bytes(&pkt, pk);
    new_signature_from_bytes(&sigt, sm);

    ret = !new_protocols_verify(&sigt, &pkt, sm + NEW_SIGNATURE_BYTES, smlen - NEW_SIGNATURE_BYTES);

    if (!ret) {
        *mlen = smlen - NEW_SIGNATURE_BYTES;
        memmove(m, sm + NEW_SIGNATURE_BYTES, *mlen);
    } else {
        *mlen = 0;
        memset(m, 0, smlen - NEW_SIGNATURE_BYTES);
    }

    return ret;
}

SQISIGN_API
int
sqisign_verify(const unsigned char *m,
               unsigned long long mlen,
               const unsigned char *sig,
               unsigned long long siglen,
               const unsigned char *pk)
{

    int ret = 0;
    public_key_t pkt = { 0 };
    new_signature_t sigt;

    if (siglen != NEW_SIGNATURE_BYTES)
        return 1;

    public_key_from_bytes(&pkt, pk);
    new_signature_from_bytes(&sigt, sig);

    ret = !new_protocols_verify(&sigt, &pkt, m, mlen);

    return ret;
}
