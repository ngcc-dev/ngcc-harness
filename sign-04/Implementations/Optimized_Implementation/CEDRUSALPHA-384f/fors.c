#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "fors.h"
#include "utils.h"
#include "utilsx1.h"
#include "hash.h"
#include "thash.h"
#include "address.h"

static void fors_gen_sk(unsigned char *sk, const unsigned char *sk_seed,
                        uint32_t fors_leaf_addr[8])
{
    prf_addr(sk, sk_seed, fors_leaf_addr);
}

static void fors_next_leaf(unsigned char *leaf, const unsigned char *sk,
                            const unsigned char *pub_seed,
                            uint32_t fors_leaf_addr[8])
{
    thash(leaf, sk, 1, pub_seed, fors_leaf_addr);
}

static void fors_gen_node(unsigned char *out, const unsigned char *in, uint32_t start, uint32_t end, const unsigned char *pub_seed,
                          uint32_t fors_leaf_addr[8])
{

    memcpy(out, in, SPX_N);
    
    for (uint32_t i = start; i <= end; i++)
    {
        set_hash_addr(fors_leaf_addr, i);
        fors_next_leaf(out, out, pub_seed, fors_leaf_addr);
    }

}


struct fors_gen_leaf_info {
    uint32_t leaf_addrx[8];
};

static void fors_gen_leafx1(unsigned char *leaf,
                            const unsigned char *sk_seed,
                            const unsigned char *pub_seed,
                            uint32_t addr_idx, void *info)
{
    struct fors_gen_leaf_info *fors_info = info;
    uint32_t *fors_leaf_addr = fors_info->leaf_addrx;

    set_chain_addr(fors_leaf_addr, addr_idx);
    set_hash_addr(fors_leaf_addr, 0);
    set_type(fors_leaf_addr, SPX_ADDR_TYPE_FORSPRF);
    fors_gen_sk(leaf, sk_seed, fors_leaf_addr); 
    set_type(fors_leaf_addr, SPX_ADDR_TYPE_FORSCHAIN);
    fors_gen_node(leaf,leaf,1,SPX_FORS_W,pub_seed,fors_leaf_addr);
}

/**
 * Interprets m as SPX_FORS_HEIGHT-bit unsigned integers.
 * Assumes m contains at least SPX_FORS_HEIGHT * SPX_FORS_TREES bits.
 * Assumes indices has space for SPX_FORS_TREES integers.
 */
static void message_to_indices(uint32_t *indices, uint32_t *lengths, const unsigned char *m)
{
    unsigned int i, j;
    unsigned int offset = 0;

    for (i = 0; i < SPX_FORS_TREES; i++)
    {
        indices[i] = 0;
        lengths[i] = 0;
        for (j = 0; j < SPX_FORS_HEIGHT; j++)
        {
            indices[i] ^= ((m[offset >> 3] >> (offset & 0x7)) & 0x1) << j;
            offset++;
        }
        for (j = 0; j < SPX_FORS_LOGW; j++)
        {
            lengths[i] ^= ((m[offset >> 3] >> (offset & 0x7)) & 0x1) << j;
            offset++;
        }
    }
}

/**
 * Signs a message m, deriving the secret key from sk_seed and the FTS address.
 * Assumes m contains at least SPX_FORS_HEIGHT * SPX_FORS_TREES bits.
 */
void fors_sign(unsigned char *sig, unsigned char *pk,
               const unsigned char *m,
               const unsigned char *sk_seed, const unsigned char *pub_seed,
               const uint32_t fors_addr[8])
{
    uint32_t indices[SPX_FORS_TREES];
    uint32_t lengths[SPX_FORS_TREES];
    unsigned char roots[SPX_FORS_TREES * SPX_N];
    uint32_t fors_tree_addr[8] = {0};
    struct fors_gen_leaf_info fors_info = {0};
    uint32_t *fors_leaf_addr = fors_info.leaf_addrx;
    uint32_t fors_pk_addr[8] = {0};
    uint32_t idx_offset;
    unsigned int i;

    copy_keypair_addr(fors_tree_addr, fors_addr);
    copy_keypair_addr(fors_pk_addr, fors_addr);
    copy_keypair_addr(fors_leaf_addr, fors_addr);

    set_type(fors_tree_addr, SPX_ADDR_TYPE_FORSTREE);
    set_type(fors_pk_addr, SPX_ADDR_TYPE_FORSPK);

    message_to_indices(indices, lengths, m);

    for (i = 0; i < SPX_FORS_TREES; i++) {
        idx_offset = i * (1 << SPX_FORS_HEIGHT);

        set_tree_height(fors_tree_addr, 0);
        set_tree_index(fors_tree_addr, indices[i] + idx_offset);

        set_hash_addr(fors_leaf_addr, 0);
        set_chain_addr(fors_leaf_addr, indices[i] + idx_offset);//???

        /* Include the secret key part that produces the selected leaf node. */
        set_type(fors_leaf_addr, SPX_ADDR_TYPE_FORSPRF); 
        fors_gen_sk(sig, sk_seed, fors_leaf_addr);
        set_type(fors_leaf_addr, SPX_ADDR_TYPE_FORSCHAIN); 
        fors_gen_node(sig,sig,1,lengths[i],pub_seed,fors_leaf_addr);
        
        sig += SPX_N;

        /* Compute the authentication path for this leaf node. */
        treehashx1(roots + i*SPX_N, sig, sk_seed, pub_seed,
                 indices[i], idx_offset, SPX_FORS_HEIGHT, fors_gen_leafx1,
                 fors_tree_addr, &fors_info);

        sig += SPX_N * SPX_FORS_HEIGHT;
    }

    /* Hash horizontally across all tree roots to derive the public key. */
    thash(pk, roots, SPX_FORS_TREES, pub_seed, fors_pk_addr);
}

/**
 * Derives the FORS public key from a signature.
 * This can be used for verification by comparing to a known public key, or to
 * subsequently verify a signature on the derived public key. The latter is the
 * typical use-case when used as an FTS below an OTS in a hypertree.
 * Assumes m contains at least SPX_FORS_HEIGHT * SPX_FORS_TREES bits.
 */
void fors_pk_from_sig(unsigned char *pk,
                      const unsigned char *sig, const unsigned char *m,
                      const unsigned char *pub_seed,
                      const uint32_t fors_addr[8])
{
    uint32_t indices[SPX_FORS_TREES];
    uint32_t lengths[SPX_FORS_TREES];
    unsigned char roots[SPX_FORS_TREES * SPX_N];
    unsigned char leaf[SPX_N];
    uint32_t fors_tree_addr[8] = {0};
    uint32_t fors_pk_addr[8] = {0};
    uint32_t fors_leaf_addr[8] ={0};
    uint32_t idx_offset;
    unsigned int i;

    copy_keypair_addr(fors_tree_addr, fors_addr);
    copy_keypair_addr(fors_pk_addr, fors_addr);
    copy_keypair_addr(fors_leaf_addr, fors_addr);

    set_type(fors_tree_addr, SPX_ADDR_TYPE_FORSTREE);
    set_type(fors_pk_addr, SPX_ADDR_TYPE_FORSPK);
    set_type(fors_leaf_addr, SPX_ADDR_TYPE_FORSCHAIN); 

    message_to_indices(indices, lengths, m);

    for (i = 0; i < SPX_FORS_TREES; i++) {
        idx_offset = i * (1 << SPX_FORS_HEIGHT);

        set_tree_height(fors_tree_addr, 0);
        set_tree_index(fors_tree_addr, indices[i] + idx_offset);

        set_chain_addr(fors_leaf_addr, indices[i] + idx_offset);

        /* Derive the leaf from the included secret key part. */
        fors_gen_node(leaf,sig,1+lengths[i],SPX_FORS_W,pub_seed,fors_leaf_addr); 

        sig += SPX_N;

        /* Derive the corresponding root node of this tree. */
        compute_root(roots + i*SPX_N, leaf, indices[i], idx_offset,
                     sig, SPX_FORS_HEIGHT, pub_seed, fors_tree_addr);
        sig += SPX_N * SPX_FORS_HEIGHT;
    }

    /* Hash horizontally across all tree roots to derive the public key. */
    thash(pk, roots, SPX_FORS_TREES, pub_seed, fors_pk_addr);
}
