/* Reproduce Shiyuan's CEDRUS-alpha WOTS and FORC-address findings against
 * the submitted 160s source.  The test links the real chain-length encoder,
 * address implementation, and SM3 PRF.  thash is a deterministic stub only
 * because the WOTS check compares the control flow induced by chain_lengths.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "address.h"
#include "hash.h"
#include "params.h"
#include "wots.h"

void thash(unsigned char *out, const unsigned char *in, unsigned int inblocks,
           const unsigned char *pub_seed, uint32_t addr[8])
{
    (void)inblocks;
    (void)pub_seed;
    for (int i = 0; i < SPX_N; i++)
        out[i] = in[i] ^ ((unsigned char *)addr)[31] ^ 0x5a;
}

static int test_wots(void)
{
    unsigned char a[SPX_N], b[SPX_N];
    for (int i = 0; i < SPX_N; i++) a[i] = b[i] = (unsigned char)i;
    memset(a + 16, 0x00, 4);
    memset(b + 16, 0xff, 4);

    unsigned int la[SPX_WOTS_LEN], lb[SPX_WOTS_LEN];
    chain_lengths(la, a);
    chain_lengths(lb, b);

    unsigned char sig[SPX_WOTS_BYTES], seed[SPX_N];
    unsigned char pka[SPX_WOTS_PK_BYTES], pkb[SPX_WOTS_PK_BYTES];
    uint32_t adra[8] = {0}, adrb[8] = {0};
    memset(sig, 0x42, sizeof(sig));
    memset(seed, 0x13, sizeof(seed));
    wots_pk_from_sig(pka, sig, a, seed, adra);
    wots_pk_from_sig(pkb, sig, b, seed, adrb);

    int ok = memcmp(la, lb, sizeof(la)) == 0 &&
             memcmp(pka, pkb, sizeof(pka)) == 0;
    printf("WOTS trailing-32-bit control: %s\n", ok ? "COLLISION" : "distinct");
    return ok;
}

static int test_forc_address(void)
{
    uint32_t a[8] = {0}, b[8] = {0};
    set_layer_addr(a, 1);
    set_layer_addr(b, 1);
    set_tree_addr(a, UINT64_C(0x12345678));
    set_tree_addr(b, UINT64_C(0x12345678));
    set_keypair_addr(a, 2);
    set_keypair_addr(b, 2);
    set_type(a, SPX_ADDR_TYPE_FORSPRF);
    set_type(b, SPX_ADDR_TYPE_FORSPRF);
    set_hash_addr(a, 0);
    set_hash_addr(b, 0);
    set_chain_addr(a, 0);
    set_chain_addr(b, 256);

    unsigned char seed[SPX_N], ska[SPX_N], skb[SPX_N];
    for (int i = 0; i < SPX_N; i++) seed[i] = (unsigned char)(0x33 + i);
    prf_addr(ska, seed, a);
    prf_addr(skb, seed, b);

    int addr_ok = memcmp(a, b, sizeof(a)) == 0;
    int sk_ok = memcmp(ska, skb, sizeof(ska)) == 0;
    printf("FORC indices 0 and 256: address=%s, PRF output=%s\n",
           addr_ok ? "COLLISION" : "distinct",
           sk_ok ? "COLLISION" : "distinct");
    return addr_ok && sk_ok;
}

int main(void)
{
    int wots = test_wots();
    int forc = test_forc_address();
    printf("result: %s\n", wots && forc
           ? "CEDRUS_ALPHA_FINDINGS_CONFIRMED" : "NOT_CONFIRMED");
    return wots && forc ? 0 : 1;
}
