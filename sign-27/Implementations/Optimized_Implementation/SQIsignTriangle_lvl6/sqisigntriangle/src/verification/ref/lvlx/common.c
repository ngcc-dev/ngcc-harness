#include <fips202.h>
#include <intbig.h>
#include <tutil.h>
#include <mp.h>
#include <encoded_sizes.h>
#include <ec_params.h>
#include <verification.h>

void
public_key_init(public_key_t *pk)
{
    ec_curve_init(&pk->curve);
}

void
public_key_finalize(public_key_t *pk)
{
}


void
hash_to_challenge_prime_with_pk_j(ibz_t *c1,
                                  ibz_t *c2,
                                  const fp2_t *pk_j_inv,
                                  const ec_curve_t *com_curve,
                                  const unsigned char *message,
                                  size_t length)
{
    unsigned char buf[2 * FP2_ENCODED_BYTES];
    {
        fp2_t com_j_inv;
        fp2_encode(buf, pk_j_inv);
        ec_j_inv(&com_j_inv, com_curve);
        fp2_encode(buf + FP2_ENCODED_BYTES, &com_j_inv);
    }

    // Level 1: SECURITY_BITS=128 -> byte_len=8  (64 bits)
    // Level 3: SECURITY_BITS=192 -> byte_len=12 (96 bits)
    // Level 5: SECURITY_BITS=256 -> byte_len=16 (128 bits)
    size_t byte_len = SECURITY_BITS / 16;
    size_t digest_size = byte_len * 2;
    uint8_t chl[2 * (SECURITY_BITS / 16)]; 

    shake256incctx ctx;
    shake256_inc_init(&ctx);

    shake256_inc_absorb(&ctx, buf, 2 * FP2_ENCODED_BYTES);
    shake256_inc_absorb(&ctx, message, length);
    shake256_inc_finalize(&ctx);

    const int reps = 40; // Number of rounds for the Miller-Rabin primality test

    while (1) {
        // Squeeze the required number of bytes each time; 
        shake256_inc_squeeze(chl, digest_size, &ctx);

        mpz_import(*c1, byte_len, 1, 1, 1, 0, chl);
        mpz_import(*c2, byte_len, 1, 1, 1, 0, chl + byte_len);

        // Modular test and primality test
        if (mpz_fdiv_ui(*c1, 4) == 3) {
            if (mpz_probab_prime_p(*c1, reps)) {
                break;
            }
        }
        if (mpz_fdiv_ui(*c2, 4) == 3) {
            if (mpz_probab_prime_p(*c2, reps)) {
                ibz_swap(c1, c2); 
                break;
            }
        }
    }

    shake256_inc_ctx_release(&ctx);
}
