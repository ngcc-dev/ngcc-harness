#include <assert.h>

#include <rng.h>
#include <curve_extras.h>
#include <sqisigndim2.h>

int
main(void)
{
    public_key_t pk;
    public_key_t pk_from_sk;
    public_key_t pk_dec;
    secret_key_t sk;
    signature_t sig;
    signature_t sig_dec;
    unsigned char pk_bytes[PUBLICKEY_BYTES];
    unsigned char sig_bytes[SIGNATURE_LEN];
    unsigned char msg[32] = { 0 };

    randombytes_init((unsigned char *)"some", (unsigned char *)"string", 128);

    public_key_init(&pk);
    public_key_init(&pk_from_sk);
    public_key_init(&pk_dec);
    secret_key_init(&sk);
    secret_sig_init(&sig);
    secret_sig_init(&sig_dec);

    protocols_keygen(&pk, &sk);
    copy_curve(&(pk_from_sk.curve), &(sk.curve));
    assert(protocols_sign(&sig, &pk_from_sk, &sk, msg, sizeof(msg), 0) == 0);

    assert(protocols_verif(&sig, &pk_from_sk, msg, sizeof(msg)) == 1);

    signature_encode(sig_bytes, &sig);
    signature_decode(&sig_dec, sig_bytes);
    assert(protocols_verif(&sig_dec, &pk_from_sk, msg, sizeof(msg)) == 1);

    public_key_encode(pk_bytes, &pk);
    public_key_decode(&pk_dec, pk_bytes);
    assert(protocols_verif(&sig, &pk_dec, msg, sizeof(msg)) == 1);

    assert(protocols_verif(&sig_dec, &pk_dec, msg, sizeof(msg)) == 1);

    secret_sig_finalize(&sig_dec);
    secret_sig_finalize(&sig);
    secret_key_finalize(&sk);
    public_key_finalize(&pk_dec);
    public_key_finalize(&pk_from_sk);
    public_key_finalize(&pk);

    return 0;
}
