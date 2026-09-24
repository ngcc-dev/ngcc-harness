#include <assert.h>

#include <rng.h>
#include <sqisigndim2.h>

int
main(void)
{
    public_key_t pk;
    secret_key_t sk;
    signature_t sig;
    unsigned char msg[32] = { 0 };

    randombytes_init((unsigned char *)"some", (unsigned char *)"string", 128);

    public_key_init(&pk);
    secret_key_init(&sk);
    secret_sig_init(&sig);

    protocols_keygen(&pk, &sk);

    int sign_ret = protocols_sign(&sig, &pk, &sk, msg, sizeof(msg), 0);
    int verify_ret = protocols_verif(&sig, &pk, msg, sizeof(msg));

    assert(sign_ret == 0);
    assert(verify_ret == 1);

    secret_sig_finalize(&sig);
    secret_key_finalize(&sk);
    public_key_finalize(&pk);

    return 0;
}
