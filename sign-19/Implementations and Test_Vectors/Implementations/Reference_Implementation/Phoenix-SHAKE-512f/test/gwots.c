#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "params.h"
#include "merkle.h"
#include "gwots.h"
#include "utils.h"
#include "context.h"
#include "address.h"
#include "thash.h"

int main() {
    spx_ctx ctx;
    
    /* 1. Initialize Seeds and Context */
    /* In a real scenario, these are part of the SK/PK */
    memset(ctx.sk_seed, 0x01, SPX_N);
    memset(ctx.pub_seed, 0x02, SPX_N);

    /* 2. Setup Addresses */
    uint32_t gwots_addr[8] = {0};
    uint32_t tree_addr[8] = {0};
    uint32_t idx_leaf = 5; /* Test with leaf index 5 to ensure ADRS logic is correct */

    set_type(gwots_addr, SPX_ADDR_TYPE_GWOTS);
    set_keypair_addr(gwots_addr, idx_leaf);

    set_type(tree_addr, SPX_ADDR_TYPE_HASHTREE);
    /* Note: tree_addr index will be updated inside treehash/compute_root */

    /* 3. Prepare buffers */
    /* The message to sign is usually the root of a lower-level subtree */
    unsigned char message_to_sign[SPX_N]; 
    memset(message_to_sign, 0xFF, SPX_N);

    unsigned char expected_root[SPX_N];
    memcpy(expected_root, message_to_sign, SPX_N);
    /* Signature buffer: GWOTS signature + Merkle Auth Path */
    unsigned char sig[SPX_GWOTS_BYTES + SPX_TREE_HEIGHT * SPX_N + 64];
    
    unsigned char recovered_pk_full[SPX_GWOTS_PK_BYTES];
    unsigned char recovered_leaf[SPX_N];
    unsigned char computed_root[SPX_N];
    
    uint32_t counter;

    printf("--- Starting Optimized GWOTS+C Merkle Test ---\n");
    printf("Message to sign: ");
    for(int i=0; i<SPX_N; i++) printf("%02x", message_to_sign[i]);
    printf("\n\n");

    /* 4. SIGN: Generate GWOTS signature and Subtree Root */
    /* This will trigger the counter search loop in merkle_sign */
    /* We pass message_to_sign as the 'root' parameter because merkle_sign 
       signs the root of the tree below it. */
    merkle_sign(sig, expected_root, &ctx, gwots_addr, tree_addr, idx_leaf, &counter);

    printf("Step 1: merkle_sign completed.\n");
    printf("Found Counter: %u\n", counter);
    printf("Expected Root:  ");
    for(int i=0; i<SPX_N; i++) printf("%02x", expected_root[i]);
    printf("\n\n");

    /* 5. VERIFY - Phase A: Recover GWOTS Public Key */
    /* Use the counter found during signing to reconstruct the digest */
    gwots_pk_from_sig(recovered_pk_full, sig, message_to_sign, &ctx, gwots_addr, counter);

    /* Check if recovery failed (returned all zeros) */
    int all_zero = 1;
    for(int i=0; i<SPX_GWOTS_PK_BYTES; i++) {
        if(recovered_pk_full[i] != 0) {
            all_zero = 0;
            break;
        }
    }
    if(all_zero) {
        printf("FAILURE: gwots_pk_from_sig validation failed (Zero-bits or Range).\n");
        return -1;
    }
    printf("Step 2: GWOTS PK recovered and validated.\n");

    /* 6. VERIFY - Phase B: Convert GWOTS PK to Leaf */
    /* In SPHINCS+, the leaf is the thash of the GWOTS PK */
    uint32_t gwots_pk_addr[8] = {0};
    copy_keypair_addr(gwots_pk_addr, gwots_addr);
    set_type(gwots_pk_addr, SPX_ADDR_TYPE_GWOTSPK);
    thash(recovered_leaf, recovered_pk_full, SPX_GWOTS_LEN, &ctx, gwots_pk_addr);
    
    printf("Step 3: Leaf node computed.\n");

    /* 7. VERIFY - Phase C: Compute Root from Leaf and Auth Path */
    unsigned char *auth_path = sig + SPX_GWOTS_BYTES;
    
    /* Reset tree_addr type for root computation */
    set_type(tree_addr, SPX_ADDR_TYPE_HASHTREE);
    compute_root(computed_root, recovered_leaf, idx_leaf, 0, auth_path, SPX_TREE_HEIGHT, &ctx, tree_addr);

    printf("Step 4: Subtree Root computed from Auth Path.\n");
    printf("Computed Root:  ");
    for(int i=0; i<SPX_N; i++) printf("%02x", computed_root[i]);
    printf("\n\n");

    /* 8. Final Comparison */
    if (memcmp(expected_root, computed_root, SPX_N) == 0) {
        printf("*******************************************\n");
        printf("* SUCCESS: Recovered Root matches Merkle! *\n");
        printf("*******************************************\n");
    } else {
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        printf("! FAILURE: Root Mismatch. Check ADRS logic.!\n");
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    }

    return 0;
}