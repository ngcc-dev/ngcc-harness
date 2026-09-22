#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "utils.h"
#include "utilsx1.h"
#include "hash.h"
#include "thash.h"
#include "wots.h"
#include "wotsx1.h"
#include "address.h"
#include "params.h"

// TODO clarify address expectations, and make them more uniform.
// TODO i.e. do we expect types to be set already?
// TODO and do we expect modifications or copies?

/**
 * Computes the chaining function.
 * out and in have to be n-byte arrays.
 *
 * Interprets in as start-th value of the chain.
 * addr has to contain the address of the chain.
 */
const unsigned int wots_w[]    = SPX_WOTS_W_ARRAY;

#define WOTS_UINT_WORDS (SPX_N / 8)

typedef uint64_t wots_uint_t[WOTS_UINT_WORDS];

static int wots_uint_less(const wots_uint_t *x, const wots_uint_t *y)
{
    for (int32_t i = WOTS_UINT_WORDS - 1; i >= 0; i--)
    {
        if ((*x)[i] != (*y)[i])
        {
            return (*x)[i] < (*y)[i];
        }
    }
    return 0;
}

static void wots_uint_set0(wots_uint_t *z)
{
    memset(*z, 0, sizeof(*z));
}

static void wots_uint_set1(wots_uint_t *z)
{
    wots_uint_set0(z);
    (*z)[0] = 1;
}

static void wots_uint_add(wots_uint_t *z, const wots_uint_t *x, const wots_uint_t *y)
{
    uint64_t carry = 0;

    for (int32_t i = 0; i < WOTS_UINT_WORDS; i++)
    {
        uint64_t xi = (*x)[i];
        uint64_t yi = (*y)[i];
        uint64_t sum = xi + yi;
        uint64_t carry1 = sum < xi;
        uint64_t sum2 = sum + carry;
        uint64_t carry2 = sum2 < sum;

        (*z)[i] = sum2;
        carry = carry1 | carry2;
    }
}

static void wots_uint_sub(wots_uint_t *z, const wots_uint_t *x, const wots_uint_t *y)
{
    uint64_t borrow = 0;

    for (int32_t i = 0; i < WOTS_UINT_WORDS; i++)
    {
        uint64_t xi = (*x)[i];
        uint64_t yi = (*y)[i];
        uint64_t diff = xi - yi;
        uint64_t borrow1 = xi < yi;
        uint64_t diff2 = diff - borrow;
        uint64_t borrow2 = diff < borrow;

        (*z)[i] = diff2;
        borrow = borrow1 | borrow2;
    }
}

static void gen_chain(unsigned char *out, const unsigned char *in,
                      unsigned int start, unsigned int steps,
                      const unsigned char *pub_seed, uint32_t addr[8],uint32_t chain_idx)
{
    uint32_t i;

    /* Initialize out with the value at position 'start'. */
    memcpy(out, in, SPX_N);

    /* Iterate 'steps' calls to the hash function. */
    for (i = start; i < (start + steps) && i < wots_w[SPX_WOTS_LEN-1-chain_idx]; i++)
    {
        set_hash_addr(addr, i);
        thash(out, out, 1, pub_seed, addr);
    }
}

void encode(unsigned int *out, wots_uint_t *x, int l)
{

    static int done = 0;
    static wots_uint_t dp[SPX_WOTS_LEN + 1][SPX_WOTS_S + 1];
    int s = SPX_WOTS_S;


    if (!done)
    {

        wots_uint_set1(&dp[0][0]);
        for (int i = 1; i <= l; i++)
            for (int j = 0; j <= s; j++)
            {
                wots_uint_set0(&dp[i][j]);
                int w = (int)wots_w[i-1];
                for (int k = 0; k < w && k <= j; k++)
                    wots_uint_add(&dp[i][j], (const wots_uint_t *)&dp[i][j], (const wots_uint_t *)&dp[i - 1][j - k]);
            }

        done = 1;
    }

    for (int i = l - 1; i >= 0; i--)
    {
        int t = -1;
        for (int j = 0; j < (int)wots_w[i] && j <= s; j++)
        {
            if (!wots_uint_less((const wots_uint_t *)x, (const wots_uint_t *)&dp[i][s - j]))
            {
                wots_uint_sub(x, (const wots_uint_t *)x, (const wots_uint_t *)&dp[i][s - j]);
            }
            else
            {
                t = j;
                break;
            }
        }
        assert(t!=-1);
        
        out[l-1-i] = t;
        s -= t;
    }
}

/* Takes a message and derives the matching chain lengths. */
void chain_lengths(unsigned int *lengths, const unsigned char *msg)
{
    wots_uint_t m;

    wots_uint_set0(&m);
    for (int i = 0; i < SPX_N/8; i++)
    {
        m[i] = bytes_to_ull(msg+i*8,8);
    }

    encode(lengths, &m, SPX_WOTS_LEN);

}

/**
 * Takes a WOTS signature and an n-byte message, computes a WOTS public key.
 *
 * Writes the computed public key to 'pk'.
 */
void wots_pk_from_sig(unsigned char *pk,
                      const unsigned char *sig, const unsigned char *msg,
                      const unsigned char *pub_seed, uint32_t addr[8])
{
    unsigned int lengths[SPX_WOTS_LEN];
    uint32_t i;

    chain_lengths(lengths, msg);

    for (i = 0; i < SPX_WOTS_LEN; i++)
    {
        set_chain_addr(addr, i);
        gen_chain(pk + i * SPX_N, sig + i * SPX_N,
                  lengths[i], wots_w[SPX_WOTS_LEN-1-i] - 1 - lengths[i], pub_seed, addr,i);
    }
}
